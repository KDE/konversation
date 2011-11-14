/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005-2009 Eike Hein <hein@kde.org>
*/

#include "outputfilter.h"
#include "application.h"
#include "mainwindow.h"
#include "channel.h"
#include "awaymanager.h"
#include "ignore.h"
#include "server.h"
#include "scriptlauncher.h"
#include "irccharsets.h"
#include "linkaddressbook/addressbook.h"
#include "query.h"
#include "viewcontainer.h"
#include "outputfilterresolvejob.h"

#ifdef HAVE_QCA2
#include "cipher.h"
#endif

#include <QStringList>
#include <QFile>
#include <QRegExp>
#include <QTextCodec>
#include <QByteArray>
#include <QTextStream>

#include <KPasswordDialog>
#include <KMessageBox>
#include <KAboutData>

#include <QTextDocument>
#include <QTextBlock>
#include <kdebug.h>
#include "ircview.h"

QDebug operator<<(QDebug d, QTextDocument* document);

namespace Konversation
{
    QSet<QString> OutputFilter::m_commands;

    void OutputFilter::fillCommandList()
    {
        if (m_commands.size() > 0)
            return;

        QString methodSignature;

        for (int i = OutputFilter::staticMetaObject.methodOffset();
            i < OutputFilter::staticMetaObject.methodCount(); ++i)
        {
            methodSignature = QString::fromLatin1(OutputFilter::staticMetaObject.method(i).signature());

            if (methodSignature.startsWith(QLatin1String("command_")))
                m_commands << methodSignature.mid(8).section('(', 0, 0).toLower();
        }
    }

    OutputFilter::OutputFilter(Server* server)
        : QObject(server)
    {
        m_server = server;

        fillCommandList();
    }

    OutputFilter::~OutputFilter()
    {
    }

    // replace all aliases in the string and return true if anything got replaced at all
    bool OutputFilter::replaceAliases(QString& line)
    {
        QStringList aliasList=Preferences::self()->aliasList();
        QString cc(Preferences::self()->commandChar());
        // check if the line starts with a defined alias
        for(int index=0;index<aliasList.count();index++)
        {
            // cut alias pattern from definition
            QString aliasPattern(aliasList[index].section(' ',0,0));
            // cut first word from command line, so we do not wrongly find an alias
            // that starts with the same letters, like /m would override /me
            QString lineStart=line.section(' ',0,0);

            // pattern found?
            // TODO: cc may be a regexp character here ... we should escape it then
            if (lineStart==cc+aliasPattern)
            {
                QString aliasReplace;

                // cut alias replacement from definition
                if ( aliasList[index].contains("%p") )
                    aliasReplace = aliasList[index].section(' ',1);
                else
                    aliasReplace = aliasList[index].section(' ',1 )+' '+line.section(' ',1 );

                // protect "%%"
                aliasReplace.replace("%%","%\x01");
                // replace %p placeholder with rest of line
                aliasReplace.replace("%p",line.section(' ',1));
                // restore "%<1>" as "%%"
                aliasReplace.replace("%\x01","%%");
                // modify line
                line=aliasReplace;
                // return "replaced"
                return true;
            }
        }                                         // for

        return false;
    }

    QStringList OutputFilter::splitForEncoding(const QString& destination, const QString& inputLine,
                                               int max, int segments)
    {
        int sublen = 0; //The encoded length since the last split
        int charLength = 0; //the length of this char
        int lastBreakPoint = 0;

        //FIXME should we run this through the encoder first, checking with "canEncode"?
        QString text = inputLine; // the text we'll send, currently in Unicode
        QStringList finals; // The strings we're going to output

        QString channelCodecName = Preferences::channelEncoding(m_server->getDisplayName(), destination);
        //Get the codec we're supposed to use. This must not fail. (not verified)
        QTextCodec* codec;

        // I copied this bit straight out of Server::send
        if (channelCodecName.isEmpty())
        {
            codec = m_server->getIdentity()->getCodec();
        }
        else
        {
            codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);
        }

        Q_ASSERT(codec);
        int index = 0;

        while (index < text.length() && (segments == -1 || finals.size() < segments-1))
        {
            // The most important bit - turn the current char into a QCString so we can measure it
            QByteArray ch = codec->fromUnicode(QString(text[index]));
            charLength = ch.length();

            // If adding this char puts us over the limit:
            if (charLength + sublen > max)
            {
                if (lastBreakPoint != 0)
                {
                    finals.append(text.left(lastBreakPoint + 1));
                    text = text.mid(lastBreakPoint + 1);
                }
                else
                {
                    finals.append(text.left(index));
                    text = text.mid(index);
                }

                lastBreakPoint = 0;
                sublen = 0;
                index = 0;
            }
            else if (text[index].isSpace() || text[index].isPunct())
            {
                lastBreakPoint = index;
            }

            ++index;
            sublen += charLength;
        }

        if (!text.isEmpty())
        {
            finals.append(text);
        }

        return finals;
    }

    bool OutputFilter::checkForEncodingConflict(QString *line, const QString& target)
    {
        QTextCodec* codec = 0;
        QString encoding;
        QString oldLine(*line);
        if(m_server->getServerGroup())
            encoding = Preferences::channelEncoding(m_server->getServerGroup()->id(), target);
        else
            encoding = Preferences::channelEncoding(m_server->getDisplayName(), target);

        if(encoding.isEmpty())
            codec = m_server->getIdentity()->getCodec();
        else
            codec = Konversation::IRCCharsets::self()->codecForName(encoding);

        QTextCodec::ConverterState state;

        QString newLine;
        if(codec)
            newLine = codec->fromUnicode(oldLine.constData(),oldLine.length(),&state);

        if(!newLine.isEmpty() && state.invalidChars)
        {
            int ret = KMessageBox::Continue;

            ret = KMessageBox::warningContinueCancel(m_server->getViewContainer()->getWindow(),
            i18n("The message you are sending includes characters "
                 "that do not exist in your current encoding. If "
                 "you choose to continue anyway those characters "
                 "will be replaced by a '?'."),
            i18n("Encoding Conflict Warning"),
            KStandardGuiItem::cont(),
            KStandardGuiItem::cancel(),
            "WarnEncodingConflict");

            *line = newLine; //set so we show it as it's sent
            if (ret != KMessageBox::Continue) return true;
        }

        return false;
    }

    OutputFilterResult OutputFilter::parse(const QString& myNick, const QString& originalLine,
        const QString& destination, ChatWindow* inputContext)
    {
        OutputFilterInput input;
        input.myNick = myNick;
        input.destination = destination;
        input.context = inputContext;

        OutputFilterResult result;

        QString inputLine(originalLine);

        if (inputLine.isEmpty() || inputLine == "\n" || checkForEncodingConflict(&inputLine, destination))
            return result;

        //Protect against nickserv auth being sent as a message on the off chance
        // someone didn't notice leading spaces
        {
            QString testNickServ(inputLine.trimmed());
            if(testNickServ.startsWith(Preferences::self()->commandChar() + "nickserv", Qt::CaseInsensitive)
              || testNickServ.startsWith(Preferences::self()->commandChar() + "ns", Qt::CaseInsensitive))
            {
                    inputLine = testNickServ;
            }
        }

        //perform variable expansion according to prefs
        inputLine = Konversation::doVarExpansion(inputLine);

        QString line = inputLine.toLower();

        // Convert double command chars at the beginning to single ones
        if(line.startsWith(Preferences::self()->commandChar() + Preferences::self()->commandChar()) && !destination.isEmpty())
        {
            inputLine = inputLine.mid(1);
            goto BYPASS_COMMAND_PARSING;
        }
        // Server command?
        else if (line.startsWith(Preferences::self()->commandChar()))
        {
            QString command = inputLine.section(' ', 0, 0).mid(1).toLower();
            input.parameter = inputLine.section(' ', 1);

            if (command != "topic")
                input.parameter = input.parameter.trimmed();

            if (supportedCommands().contains(command))
            {
                QString methodSignature("command_" + command);

                QMetaObject::invokeMethod(this,
                    methodSignature.toLatin1().constData(),
                    Qt::DirectConnection,
                    Q_RETURN_ARG(OutputFilterResult, result),
                    Q_ARG(OutputFilterInput, input));
            }
            // Forward unknown commands to server
            else
            {
                result.toServer = inputLine.mid(1);
                result.type = Message;
            }
        }
        // Ordinary message to channel/query?
        else if (!destination.isEmpty())
        {
            BYPASS_COMMAND_PARSING:

            QStringList outputList = splitForEncoding(destination, inputLine,
                                                      m_server->getPreLength("PRIVMSG", destination));

            if (outputList.count() > 1)
            {
                result.output.clear();
                result.outputList = outputList;
                for (QStringList::ConstIterator it = outputList.constBegin(); it != outputList.constEnd(); ++it)
                {
                    result.toServerList += "PRIVMSG " + destination + " :" + *it;
                }
            }
            else
            {
                result.output = inputLine;
                result.toServer = "PRIVMSG " + destination + " :" + inputLine;
            }

            result.type = Message;
        }
        // Eveything else goes to the server unchanged
        else
        {
            result.toServer = inputLine;
            result.output = inputLine;
            result.typeString = i18n("Raw");
            result.type = Program;
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_op(const OutputFilterInput& input)
    {
        return changeMode(input.parameter, input.destination, 'o', '+');
    }

    OutputFilterResult OutputFilter::command_deop(const OutputFilterInput& input)
    {
        return changeMode(addNickToEmptyNickList(input.myNick, input.parameter),
                          input.destination, 'o', '-');
    }

    OutputFilterResult OutputFilter::command_hop(const OutputFilterInput& input)
    {
        return changeMode(input.parameter, input.destination, 'h', '+');
    }

    OutputFilterResult OutputFilter::command_dehop(const OutputFilterInput& input)
    {
        return changeMode(addNickToEmptyNickList(input.myNick, input.parameter), input.destination,
                          'h', '-');
    }

    OutputFilterResult OutputFilter::command_voice(const OutputFilterInput& input)
    {
        return changeMode(input.parameter, input.destination, 'v', '+');
    }

    OutputFilterResult OutputFilter::command_unvoice(const OutputFilterInput& input)
    {
        return changeMode(addNickToEmptyNickList(input.myNick, input.parameter), input.destination,
                          'v', '-');
    }

    OutputFilterResult OutputFilter::command_devoice(const OutputFilterInput& input)
    {
        return command_unvoice(input);
    }

    OutputFilterResult OutputFilter::command_join(const OutputFilterInput& _input)
    {
        OutputFilterInput input(_input);
        OutputFilterResult result;

        if (input.parameter.contains(",")) // Protect against #foo,0 tricks
            input.parameter = input.parameter.remove(",0");
        //else if(channelName == "0") // FIXME IRC RFC 2812 section 3.2.1

        if (input.parameter.isEmpty())
        {
            if (input.destination.isEmpty() || !isAChannel(input.destination))
                return usage(i18n("Usage: %1JOIN <channel> [password]",
                                  Preferences::self()->commandChar()));

            input.parameter = input.destination;
        }
        else if (!isAChannel(input.parameter))
            input.parameter = '#' + input.parameter.trimmed();

        Channel* channel = m_server->getChannelByName(input.parameter);

        if (channel)
        {
            // Note that this relies on the channels-flush-nicklists-on-disconnect behavior.
            if (!channel->numberOfNicks())
                result.toServer = "JOIN " + input.parameter;

            if (channel->joined()) emit showView(channel);
        }
        else
            result.toServer = "JOIN " + input.parameter;

        return result;
    }

    OutputFilterResult OutputFilter::command_j(const OutputFilterInput& input)
    {
        return command_join(input);
    }

    OutputFilterResult OutputFilter::command_kick(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (isAChannel(input.destination))
        {
            // get nick to kick
            QString victim = input.parameter.left(input.parameter.indexOf(' '));

            if (victim.isEmpty())
                result = usage(i18n("Usage: %1KICK <nick> [reason]", Preferences::self()->commandChar()));
            else
            {
                // get kick reason (if any)
                QString reason = input.parameter.mid(victim.length() + 1);

                // if no reason given, take default reason
                if (reason.isEmpty())
                    reason = m_server->getIdentity()->getKickReason();

                result.toServer = "KICK " + input.destination + ' ' + victim + " :" + reason;
            }
        }
        else
            result = error(i18n("%1KICK only works from within channels.",
                                Preferences::self()->commandChar()));

        return result;
    }

    OutputFilterResult OutputFilter::command_part(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        // No parameter, try default part message
        if (input.parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if (isAChannel(input.destination))
                result.toServer = "PART " + input.destination + " :" +
                                  m_server->getIdentity()->getPartReason();
            else if (m_server->getQueryByName(input.destination))
                m_server->closeQuery(input.destination);
            else
                result = error(i18n("%1PART and %1LEAVE without parameters only work from within a "
                                    "channel or a query.", Preferences::self()->commandChar()));
        }
        else
        {
            // part a given channel
            if (isAChannel(input.parameter))
            {
                // get channel name
                QString channel = input.parameter.left(input.parameter.indexOf(' '));
                // get part reason (if any)
                QString reason = input.parameter.mid(channel.length() + 1);

                // if no reason given, take default reason
                if (reason.isEmpty())
                    reason = m_server->getIdentity()->getPartReason();

                result.toServer = "PART " + channel + " :" + reason;
            }
            // part this channel with a given reason
            else
            {
                if (isAChannel(input.destination))
                    result.toServer = "PART " + input.destination + " :" + input.parameter;
                else
                    result = error(i18n("%1PART without channel name only works from within a channel.",
                                        Preferences::self()->commandChar()));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_leave(const OutputFilterInput& input)
    {
        return command_part(input);
    }

    OutputFilterResult OutputFilter::command_topic(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        // No parameter, try to get current topic
        if (input.parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if (isAChannel(input.destination))
                result.toServer = "TOPIC " + input.destination;
            else
                result = error(i18n("%1TOPIC without parameters only works from within a channel.",
                                    Preferences::self()->commandChar()));
        }
        else
        {
            // retrieve or set topic of a given channel
            if (isAChannel(input.parameter))
            {
                // get channel name
                QString channel = input.parameter.left(input.parameter.indexOf(' '));
                // get topic (if any)
                QString topic = input.parameter.mid(channel.length()+1);
                // if no topic given, retrieve topic
                if (topic.isEmpty())
                    m_server->requestTopic(channel);

                // otherwise set topic there
                else
                {
                    result.toServer = "TOPIC " + channel + " :";
                    //If we get passed a ^A as a topic its a sign we should clear the topic.
                    //Used to be a \n, but those get smashed by QStringList::split and readded later
                    //Now is not the time to fight with that. FIXME
                    //If anyone out there *can* actually set the topic to a single ^A, now they have to
                    //specify it twice to get one.
                    if (topic =="\x01\x01")
                        result.toServer += '\x01';
                    else if (topic!="\x01")
                        result.toServer += topic;
                }
            }
            // set this channel's topic
            else
            {
                if (isAChannel(input.destination))
                    result.toServer = "TOPIC " + input.destination + " :" + input.parameter;
                else
                    result = error(i18n("%1TOPIC without channel name only works from within a channel.",
                                        Preferences::self()->commandChar()));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_away(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty() && m_server->isAway())
            m_server->requestUnaway();
        else
            m_server->requestAway(input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_unaway(const OutputFilterInput& /* input */)
    {
        m_server->requestUnaway();

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_back(const OutputFilterInput& input)
    {
        return command_unaway(input);
    }

    OutputFilterResult OutputFilter::command_aaway(const OutputFilterInput& input)
    {
        Application::instance()->getAwayManager()->requestAllAway(input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_aunaway(const OutputFilterInput& /* input */)
    {
        Application::instance()->getAwayManager()->requestAllUnaway();

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_aback(const OutputFilterInput& input)
    {
        return command_aunaway(input);
    }

    OutputFilterResult OutputFilter::command_names(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        result.toServer = "NAMES ";

        if (input.parameter.isNull())
            result = error(i18n("%1NAMES with no target may disconnect you from the server. Specify "
                                "'*' if you really want this.", Preferences::self()->commandChar()));
        else if (input.parameter != QChar('*'))
            result.toServer.append(input.parameter);

        return result;
    }

    OutputFilterResult OutputFilter::command_close(const OutputFilterInput& _input)
    {
        OutputFilterInput input(_input);
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            input.parameter = input.destination;

        if (isAChannel(input.parameter) && m_server->getChannelByName(input.parameter))
            m_server->getChannelByName(input.parameter)->closeYourself(false);
        else if (m_server->getQueryByName(input.parameter))
            m_server->getQueryByName(input.parameter)->closeYourself(false);
        else if (input.parameter.isEmpty()) // this can only mean one thing.. we're in the Server tab
            m_server->closeYourself(false);
        else
            result = usage(i18n("Usage: %1close [window] closes the named channel or query tab, "
                                "or the current tab if none specified.",
                                Preferences::self()->commandChar()));

        return result;
    }

    OutputFilterResult OutputFilter::command_reconnect(const OutputFilterInput& input)
    {
        emit reconnectServer(input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_disconnect(const OutputFilterInput& input)
    {
        emit disconnectServer(input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_quit(const OutputFilterInput& input)
    {
        emit disconnectServer(input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_notice(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        QString recipient = input.parameter.left(input.parameter.indexOf(' '));
        QString message = input.parameter.mid(recipient.length()+1);

        if (input.parameter.isEmpty() || message.isEmpty())
            result = usage(i18n("Usage: %1NOTICE <recipient> <message>",
                                Preferences::self()->commandChar()));
        else
        {
            result.typeString = i18n("Notice");
            result.toServer = "NOTICE " + recipient + " :" + message;
            result.output=i18nc("%1 is the message, %2 the recipient nickname",
                                "Sending notice \"%1\" to %2.", message, recipient);
            result.type = Program;
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_me(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.destination.isEmpty())
            return error(i18n("%1ME only works from a channel, query or DCC chat tab.", Preferences::self()->commandChar()));
        else if (input.parameter.isEmpty())
            return usage(i18n("Usage: %1ME text", Preferences::self()->commandChar()));

        QString command("PRIVMSGACTION \x01\x01");

        QStringList outputList = splitForEncoding(input.destination, input.parameter,
                                                  m_server->getPreLength(command, input.destination), 2);

        if (outputList.count() > 1)
        {
            command = "PRIVMSG";

            outputList += splitForEncoding(input.destination, outputList.at(1),
                                           m_server->getPreLength(command, input.destination));

            outputList.removeAt(1);

            result.output.clear();
            result.outputList = outputList;

            for (int i = 0; i < outputList.count(); ++i)
            {
                if (i == 0)
                    result.toServerList += "PRIVMSG " + input.destination + " :" + '\x01' +
                                           "ACTION " + outputList.at(i) + '\x01';
                else
                    result.toServerList += "PRIVMSG " + input.destination + " :" + outputList.at(i);
            }
        }
        else
        {
            result.output = input.parameter;
            result.toServer = "PRIVMSG " + input.destination + " :" + '\x01' + "ACTION " +
                              input.parameter + '\x01';
        }

        result.type = Action;

        return result;
    }

    OutputFilterResult OutputFilter::command_msg(const OutputFilterInput& input)
    {
        return handleMsg(input.parameter, false);
    }

    OutputFilterResult OutputFilter::command_m(const OutputFilterInput& input)
    {
        return command_msg(input);
    }

    OutputFilterResult OutputFilter::command_query(const OutputFilterInput& input)
    {
        return handleMsg(input.parameter, true);
    }

    OutputFilterResult OutputFilter::handleMsg(const QString& parameter, bool commandIsQuery)
    {
        OutputFilterResult result;

        QString recipient = parameter.section(' ', 0, 0, QString::SectionSkipEmpty);
        QString message = parameter.section(' ', 1);
        QString output;

        bool recipientIsAChannel = false;

        if (recipient.isEmpty())
            return error(i18n("You need to specify a recipient."));
        else
            recipientIsAChannel = m_server->isAChannel(recipient);

        if (commandIsQuery && recipientIsAChannel)
            return error(i18n("You cannot open queries to channels."));

        if (message.trimmed().isEmpty())
        {
            // Empty result - we don't want to send any message to the server.
            if (!commandIsQuery)
                return error(i18n("You need to specify a message."));
        }
        else
        {
            output = message;

            if (message.startsWith(Preferences::self()->commandChar() + "me"))
                result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " +
                                   message.mid(4) + '\x01';
            else
                result.toServer = "PRIVMSG " + recipient + " :" + message;
        }

        // If this is a /query, always open a query window.
        // Treat "/msg nick" as "/query nick".
        if (commandIsQuery || output.isEmpty())
        {
            // Note we have to be a bit careful here.
            // We only want to create ('obtain') a new nickinfo if we have done /query
            // or "/msg nick".  Not "/msg nick message".
            NickInfoPtr nickInfo = m_server->obtainNickInfo(recipient);
            ::Query* query = m_server->addQuery(nickInfo, true /*we initiated*/);

            // Force focus if the user did not specify any message.
            if (output.isEmpty()) emit showView(query);
        }

        // Result should be completely empty;
        if (output.isEmpty()) return result;

        //FIXME: Don't do below line if query is focused.
        result.output = output;
        result.typeString = recipient;
        result.type = PrivateMessage;

        return result;
    }

    OutputFilterResult OutputFilter::command_smsg(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        QString recipient = input.parameter.left(input.parameter.indexOf(' '));
        QString message = input.parameter.mid(recipient.length() + 1);

        if (message.startsWith(Preferences::self()->commandChar() + "me"))
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " +
                              message.mid(4) + '\x01';
        else
            result.toServer = "PRIVMSG " + recipient + " :" + message;

        return result;
    }

    OutputFilterResult OutputFilter::command_ping(const OutputFilterInput& input)
    {
        return handleCtcp(input.parameter.section(' ', 0, 0) + " ping");
    }

    OutputFilterResult OutputFilter::command_ctcp(const OutputFilterInput& input)
    {
        return handleCtcp(input.parameter);
    }

    OutputFilterResult OutputFilter::handleCtcp(const QString& parameter)
    {
        OutputFilterResult result;
                                                  // who is the recipient?
        QString recipient = parameter.section(' ', 0, 0);
                                                  // what is the first word of the ctcp?
        QString request = parameter.section(' ', 1, 1, QString::SectionSkipEmpty).toUpper();
                                                  // what is the complete ctcp command?
        QString message = parameter.section(' ', 2, 0xffffff, QString::SectionSkipEmpty);

        QString out = request;

        if (!message.isEmpty())
            out+= ' ' + message;

        if (request == "PING")
        {
            unsigned int timeT = QDateTime::currentDateTime().toTime_t();
            result.toServer = QString("PRIVMSG %1 :\x01PING %2\x01").arg(recipient).arg(timeT);
            result.output = i18n("Sending CTCP-%1 request to %2.", QString::fromLatin1("PING"), recipient);
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + out + '\x01';
            result.output = i18n("Sending CTCP-%1 request to %2.", out, recipient);
        }

        result.typeString = i18n("CTCP");
        result.type = Program;

        return result;
    }

    OutputFilterResult OutputFilter::command_ame(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
            return usage(i18n("Usage: %1AME [-LOCAL] text", Preferences::self()->commandChar()));

        if (isParameter("local", input.parameter.section(' ', 0, 0)))
            m_server->sendToAllChannelsAndQueries(Preferences::self()->commandChar() + "me " + input.parameter.section(' ', 1));
        else
            emit multiServerCommand("me", input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_amsg(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
            return usage(i18n("Usage: %1AMSG [-LOCAL] text", Preferences::self()->commandChar()));

        if (isParameter("local", input.parameter.section(' ', 0, 0)))
            m_server->sendToAllChannelsAndQueries(input.parameter.section(' ', 1));
        else
            emit multiServerCommand("msg", input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_omsg(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
            return usage(i18n("Usage: %1OMSG text", Preferences::self()->commandChar()));

        OutputFilterResult result;

        result.toServer = "PRIVMSG @" + input.destination + " :" + input.parameter;

        return result;
    }

    OutputFilterResult OutputFilter::command_onotice(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (!input.parameter.isEmpty())
        {
            result.toServer = "NOTICE @" + input.destination + " :" + input.parameter;
            result.typeString = i18n("Notice");
            result.type = Program;
            result.output = i18n("Sending notice \"%1\" to %2.", input.parameter, input.destination);
        }
        else
            result = usage(i18n("Usage: %1ONOTICE text", Preferences::self()->commandChar()));

        return result;
    }

    OutputFilterResult OutputFilter::command_quote(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            result = usage(i18n("Usage: %1QUOTE command list", Preferences::self()->commandChar()));
        else
            result.toServer = input.parameter;

        return result;
    }

    OutputFilterResult OutputFilter::command_say(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            result = usage(i18n("Usage: %1SAY text", Preferences::self()->commandChar()));
        else
        {
            result.toServer = "PRIVMSG " + input.destination + " :" + input.parameter;
            result.output = input.parameter;
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_dcc(const OutputFilterInput& _input)
    {
        OutputFilterInput input(_input);
        OutputFilterResult result;

        kDebug() << input.parameter;
        // No parameter, just open DCC panel
        if (input.parameter.isEmpty())
        {
            emit addDccPanel();
        }
        else
        {
            QStringList parameterList = input.parameter.replace("\\ ", "%20").split(' ');

            QString dccType = parameterList[0].toLower();

            //TODO close should not just refer to the gui-panel, let it close connections
            if (dccType == "close")
            {
                emit closeDccPanel();
            }
            else if (dccType == "send")
            {
                if (parameterList.count() == 1) // DCC SEND
                {
                    emit requestDccSend();
                }
                else if (parameterList.count() == 2) // DCC SEND <nickname>
                {
                    emit requestDccSend(parameterList[1]);
                }
                else if (parameterList.count() > 2) // DCC SEND <nickname> <file> [file] ...
                {
                    // TODO: make sure this will work:
                    //output=i18n("Usage: %1DCC SEND nickname [filename] [filename] ...").arg(commandChar);
                    KUrl fileURL(parameterList[2]);

                    //We could easily check if the remote file exists, but then we might
                    //end up asking for creditionals twice, so settle for only checking locally
                    if (!fileURL.isLocalFile() || QFile::exists(fileURL.path()))
                    {
                        emit openDccSend(parameterList[1],fileURL);
                    }
                    else
                    {
                        result = error(i18n("File \"%1\" does not exist.", parameterList[2]));
                    }
                }
                else                              // Don't know how this should happen, but ...
                    result = usage(i18n("Usage: %1DCC [SEND [nickname [filename]]]",
                                        Preferences::self()->commandChar()));
            }
            else if (dccType == "get")
            {
                //dcc get [nick [file]]
                switch (parameterList.count())
                {
                    case 1:
                        emit acceptDccGet("","");
                        break;
                    case 2:
                        emit acceptDccGet(parameterList.at(1),"");
                        break;
                    case 3:
                        emit acceptDccGet(parameterList.at(1),parameterList.at(2));
                        break;
                    default:
                        result = usage(i18n("Usage: %1DCC [GET [nickname [filename]]]",
                                            Preferences::self()->commandChar()));
                }
            }
            else if (dccType == "chat")
            {
                //dcc chat nick
                switch (parameterList.count())
                {
                    case 1:
                        emit openDccChat("");
                        break;
                    case 2:
                        emit openDccChat(parameterList[1]);
                        break;
                    default:
                        result = usage(i18n("Usage: %1DCC [CHAT [nickname]]",
                                            Preferences::self()->commandChar()));
                }
            }
            else if (dccType == "whiteboard")
            {
                //dcc whiteboard nick
                switch (parameterList.count())
                {
                    case 1:
                        emit openDccWBoard("");
                        break;
                    case 2:
                        emit openDccWBoard(parameterList[1]);
                        break;
                    default:
                        result = usage(i18n("Usage: %1DCC [WHITEBOARD [nickname]]",
                                            Preferences::self()->commandChar()));
                }
            }
            else
                result = error(i18n("Unrecognized command %1DCC %2. Possible commands are SEND, "
                                    "CHAT, CLOSE, GET, WHITEBOARD.",
                                    Preferences::self()->commandChar(), parameterList[0]));
        }

        return result;
    }

    OutputFilterResult OutputFilter::sendRequest(const QString &recipient, const QString &fileName,
                                                 const QString &address, quint16 port, quint64 size)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + ' ' + QString::number(port) + ' ' + QString::number(size) + '\x01';

        return result;
    }

    OutputFilterResult OutputFilter::passiveSendRequest(const QString& recipient,
                                                        const QString &fileName,
                                                        const QString &address,
                                                        quint64 size, const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + " 0 " + QString::number(size) + ' ' + token + '\x01';

        return result;
    }

    // Accepting Resume Request
    OutputFilterResult OutputFilter::acceptResumeRequest(const QString &recipient,
                                                         const QString &fileName,
                                                         quint16 port, quint64 startAt)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + ' ' +
                          QString::number(port) + ' ' + QString::number(startAt) + '\x01';

        return result;
    }

    // Accepting Passive Resume Request
    OutputFilterResult OutputFilter::acceptPassiveResumeRequest(const QString &recipient,
                                                                const QString &fileName,
                                                                quint16 port, quint64 startAt,
                                                                const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + ' ' +
                          QString::number(port) + ' ' + QString::number(startAt) + ' ' + token +'\x01';

        return result;
    }

    // Requesting Resume Request
    OutputFilterResult OutputFilter::resumeRequest(const QString &sender, const QString &fileName,
                                                   quint16 port, KIO::filesize_t startAt)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + sender + " :" + '\x01' + "DCC RESUME " + fileName + ' ' +
                          QString::number(port) + ' ' + QString::number(startAt) + '\x01';

        return result;
    }

    // Requesting Passive Resume Request
    OutputFilterResult OutputFilter::resumePassiveRequest(const QString &sender,
                                                          const QString &fileName,
                                                          quint16 port, KIO::filesize_t startAt,
                                                          const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + sender + " :" + '\x01' + "DCC RESUME " + fileName + ' ' +
                          QString::number(port) + ' ' + QString::number(startAt) + ' ' + token + '\x01';

        return result;
    }

    // Accept Passive Send Request, there active doesn't need that
    OutputFilterResult OutputFilter::acceptPassiveSendRequest(const QString& recipient,
                                                              const QString &fileName,
                                                              const QString &address,
                                                              quint16 port, quint64 size,
                                                              const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + ' ' + QString::number(port) + ' ' + QString::number(size) + ' ' + token + '\x01';

        return result;
    }

    // Rejecting quened dcc receive, is not send when abort an active send
    OutputFilterResult OutputFilter::rejectDccSend(const QString& partnerNick, const QString & fileName)
    {
        OutputFilterResult result;
        result.toServer = "NOTICE " + partnerNick + " :" + '\x01' + "DCC REJECT SEND "
                          + fileName + '\x01';

        return result;
    }

    OutputFilterResult OutputFilter::rejectDccChat(const QString & partnerNick, const QString& extension)
    {
        OutputFilterResult result;
        result.toServer = "NOTICE " + partnerNick + " :" + '\x01' + "DCC REJECT CHAT " + extension.toUpper() + '\x01';

        return result;
    }

    OutputFilterResult OutputFilter::requestDccChat(const QString& partnerNick, const QString& extension, const QString& numericalOwnIp, quint16 ownPort)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + partnerNick + " :" + '\x01' + "DCC CHAT " +
                          extension.toUpper() + ' ' + numericalOwnIp + ' ' + QString::number(ownPort) + '\x01';
        return result;
    }

    OutputFilterResult OutputFilter::passiveChatRequest(const QString& recipient, const QString& extension, const QString& address, const QString& token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC CHAT " +
                          extension.toUpper() + ' ' + address + " 0 " + token + '\x01';
        return result;
    }

    OutputFilterResult OutputFilter::acceptPassiveChatRequest(const QString& recipient, const QString& extension, const QString& numericalOwnIp, quint16 ownPort, const QString& token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC CHAT " +
                          extension.toUpper() + ' ' + numericalOwnIp + ' ' + QString::number(ownPort) + ' ' + token + '\x01';
        return result;
    }

    OutputFilterResult OutputFilter::command_invite(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            result = usage(i18n("Usage: %1INVITE <nick> [channel]", Preferences::self()->commandChar()));
        else
        {
            QString nick = input.parameter.section(' ', 0, 0, QString::SectionSkipEmpty);
            QString channel = input.parameter.section(' ', 1, 1, QString::SectionSkipEmpty);

            if (channel.isEmpty())
            {
                if (isAChannel(input.destination))
                    channel = input.destination;
                else
                    result = error(i18n("%1INVITE without channel name works only from within channels.",
                                        Preferences::self()->commandChar()));
            }

            if (!channel.isEmpty())
            {
                if (isAChannel(channel))
                    result.toServer = "INVITE " + nick + ' ' + channel;
                else
                    result = error(i18n("%1 is not a channel.", channel));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_exec(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            result = usage(i18n("Usage: %1EXEC [-SHOWPATH] <script> [parameter list]",
                Preferences::self()->commandChar()));
        else
        {
            QStringList parameterList = input.parameter.split(' ');

            if (isParameter("showpath", parameterList[0]) && !parameterList[1].isEmpty())
            {
                result = info(i18nc("%2 is a filesystem path to the script file",
                    "The script file '%1' was found at: %2",
                    parameterList[1],
                    ScriptLauncher::scriptPath(parameterList[1])));
            }
            else if (!parameterList[0].contains("../"))
                emit launchScript(m_server->connectionId(), input.destination, input.parameter);
            else
                result = error(i18n("The script name may not contain \"../\"."));
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_raw(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty() || input.parameter == "open")
            emit openRawLog(true);
        else if (input.parameter == "close")
            emit closeRawLog();
        else
            result = usage(i18n("Usage: %1RAW [OPEN | CLOSE]", Preferences::self()->commandChar()));

        return result;
    }

    OutputFilterResult OutputFilter::command_notify(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        int serverGroupId = -1;

        if (m_server->getServerGroup())
            serverGroupId = m_server->getServerGroup()->id();

        QStringList added;
        QStringList removed;

        if (!input.parameter.isEmpty() && serverGroupId != -1)
        {
            QStringList list = input.parameter.split(' ', QString::SkipEmptyParts);


            for (int index = 0; index < list.count(); ++index)
            {
                // Try to remove current pattern.
                if (!Preferences::removeNotify(serverGroupId, list[index]))
                {
                    // If remove failed, try to add it instead.
                    if (Preferences::addNotify(serverGroupId, list[index]))
                        added << list[index];
                }
                else
                    removed << list[index];
            }
        }

        QString list = Preferences::notifyListByGroupId(serverGroupId).join(", ");

        if (list.isEmpty())
        {
            if (removed.count())
                result.output = i18nc("%1 = Comma-separated list of nicknames",
                                      "The notify list has been emptied (removed %1).",
                                      removed.join(", "));
            else
                result.output = i18n("The notify list is currently empty.");
        }
        else
        {
            if (added.count() && !removed.count())
                result.output = i18nc("%1 and %2 = Comma-separated lists of nicknames",
                                      "Current notify list: %1 (added %2).",
                                      list, added.join(", "));
            else if (removed.count() && !added.count())
                result.output = i18nc("%1 and %2 = Comma-separated lists of nicknames",
                                      "Current notify list: %1 (removed %2).",
                                      list, removed.join(", "));
            else if (added.count() && removed.count())
                result.output = i18nc("%1, %2 and %3 = Comma-separated lists of nicknames",
                                      "Current notify list: %1 (added %2, removed %3).",
                                      list, added.join(", "), removed.join(", "));
            else
                result.output = i18nc("%1 = Comma-separated list of nicknames",
                                      "Current notify list: %1.",
                                      list);
        }

        result.type = Program;
        result.typeString = i18n("Notify");

        return result;
    }

    OutputFilterResult OutputFilter::command_oper(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        QStringList parameterList = input.parameter.split(' ');

        if (input.parameter.isEmpty() || parameterList.count() == 1)
        {
            QString nick((parameterList.count() == 1) ? parameterList[0] : input.myNick);

            QPointer<KPasswordDialog> dialog = new KPasswordDialog(0, KPasswordDialog::ShowUsernameLine);
            dialog->setPrompt(i18n("Enter username and password for IRC operator privileges:"));
            dialog->setUsername(nick);
            dialog->setCaption(i18n("IRC Operator Password"));
            if (dialog->exec()) {
                result.toServer = "OPER " + dialog->username() + ' ' + dialog->password();
            }

            delete dialog;
        }
        else
            result.toServer = "OPER " + input.parameter;

        return result;
    }

    OutputFilterResult OutputFilter::command_ban(const OutputFilterInput& input)
    {
        return handleBan(input, false);
    }

    OutputFilterResult OutputFilter::command_kickban(const OutputFilterInput& input)
    {
        return handleBan(input, true);
    }

    OutputFilterResult OutputFilter::handleBan(const OutputFilterInput& input, bool kick)
    {
        OutputFilterResult result;

        // assume incorrect syntax first
        bool showUsage = true;

        if (!input.parameter.isEmpty())
        {
            QStringList parameterList = input.parameter.split(' ');
            QString channel;
            QString option;
            // check for option
            bool host = isParameter("host", parameterList[0]);
            bool domain = isParameter("domain", parameterList[0]);
            bool uhost = isParameter("userhost", parameterList[0]);
            bool udomain = isParameter("userdomain", parameterList[0]);

            // remove possible option
            if (host || domain || uhost || udomain)
            {
                option = parameterList[0].mid(1);
                parameterList.pop_front();
            }

            // look for channel / ban mask
            if (parameterList.count())
            {
                // user specified channel
                if (isAChannel(parameterList[0]))
                {
                    channel = parameterList[0];
                    parameterList.pop_front();
                }
                // no channel, so assume current destination as channel
                else if (isAChannel(input.destination))
                    channel = input.destination;
                else
                {
                    // destination is no channel => error
                    if (!kick)
                        result = error(i18n("%1BAN without channel name works only from inside a channel.",
                                            Preferences::self()->commandChar()));
                    else
                        result = error(i18n("%1KICKBAN without channel name works only from inside a channel.",
                                            Preferences::self()->commandChar()));

                    // no usage information after error
                    showUsage = false;
                }
                // signal server to ban this user if all went fine
                if (!channel.isEmpty() && !parameterList.isEmpty())
                {
                    if (kick)
                    {
                        QString victim = parameterList[0];
                        parameterList.pop_front();

                        QString reason = parameterList.join(" ");

                        if (reason.isEmpty())
                            reason = m_server->getIdentity()->getKickReason();

                        result.toServer = "KICK " + channel + ' ' + victim + " :" + reason;

                        emit banUsers(QStringList(victim), channel, option);
                    }
                    else
                        emit banUsers(parameterList, channel, option);

                    // syntax was correct, so reset flag
                    showUsage = false;
                }
            }
        }

        if (showUsage)
        {
            if (!kick)
                result = usage(i18n("Usage: %1BAN [-HOST | -DOMAIN | -USERHOST | -USERDOMAIN] "
                                    "[channel] <nickname | mask>", Preferences::self()->commandChar()));
            else
                result = usage(i18n("Usage: %1KICKBAN [-HOST | -DOMAIN | -USERHOST | -USERDOMAIN] "
                                    "[channel] <nickname | mask> [reason]",
                                    Preferences::self()->commandChar()));
        }

        return result;
    }

    // finally set the ban
    OutputFilterResult OutputFilter::execBan(const QString& mask, const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " +b " + mask;
        return result;
    }

    OutputFilterResult OutputFilter::command_unban(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        // assume incorrect syntax first
        bool showUsage = true;

        if (!input.parameter.isEmpty())
        {
            QStringList parameterList = input.parameter.split(' ');
            QString channel;
            QString mask;

            // if the user specified a channel
            if (isAChannel(parameterList[0]))
            {
                // get channel
                channel = parameterList[0];
                // remove channel from parameter list
                parameterList.pop_front();
            }
            // otherwise the current destination must be a channel
            else if (isAChannel(input.destination))
                channel = input.destination;
            else
            {
                // destination is no channel => error
                result = error(i18n("%1UNBAN without channel name works only from inside a channel.",
                                    Preferences::self()->commandChar()));
                // no usage information after error
                showUsage = false;
            }
            // if all went good, signal server to unban this mask
            if (!channel.isEmpty())
            {
                emit unbanUsers(parameterList[0], channel);
                // syntax was correct, so reset flag
                showUsage = false;
            }
        }

        if (showUsage)
            result = usage(i18n("Usage: %1UNBAN [channel] pattern", Preferences::self()->commandChar()));

        return result;
    }

    OutputFilterResult OutputFilter::execUnban(const QString& mask, const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " -b " + mask;
        return result;
    }

    OutputFilterResult OutputFilter::command_ignore(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        // assume incorrect syntax first
        bool showUsage = true;

        // did the user give parameters at all?
        if (!input.parameter.isEmpty())
        {
            QStringList parameterList = input.parameter.split(' ');

            // if nothing else said, only ignore channels and queries
            int value = Ignore::Channel | Ignore::Query;

            // user specified -all option
            if (isParameter("all", parameterList[0]))
            {
                // ignore everything
                value = Ignore::All;
                parameterList.pop_front();
            }

            // were there enough parameters?
            if (parameterList.count() >= 1)
            {
                for (int index=0;index<parameterList.count();index++)
                {
                    if (!parameterList[index].contains('!'))
                        parameterList[index] += "!*";

                    Preferences::addIgnore(parameterList[index] + ',' + QString::number(value));
                }

                result.output = i18n("Added %1 to your ignore list.", parameterList.join(", "));
                result.typeString = i18n("Ignore");
                result.type = Program;

                // all went fine, so show no error message
                showUsage = false;
            }
        }

        if (showUsage)
            result = usage(i18n("Usage: %1IGNORE [ -ALL ] <user 1> <user 2> ... <user n>",
                                Preferences::self()->commandChar()));

        return result;
    }

    OutputFilterResult OutputFilter::command_unignore(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            result = usage(i18n("Usage: %1UNIGNORE <user 1> <user 2> ... <user n>",
                                Preferences::self()->commandChar()));
        else
        {
            QString unignore = input.parameter.simplified();
            QStringList unignoreList = unignore.split(' ');

            QStringList succeeded;
            QStringList failed;

            // Iterate over potential unignores
            foreach (const QString &uign, unignoreList)
            {
                // If pattern looks incomplete, try to complete it
                if (!uign.contains('!'))
                {
                    QString fixedPattern = uign;
                    fixedPattern += "!*";

                    bool success = false;

                    // Try to remove completed pattern
                    if (Preferences::removeIgnore(fixedPattern))
                    {
                        succeeded.append(fixedPattern);
                        success = true;
                    }

                    // Try to remove the incomplete version too, in case it was added via the GUI ...
                    // FIXME: Validate patterns in GUI?
                    if (Preferences::removeIgnore(uign))
                    {
                        succeeded.append(uign);
                        success = true;
                    }

                    if (!success)
                        failed.append(uign + "[!*]");
                }
                // Try to remove seemingly complete pattern
                else if (Preferences::removeIgnore(uign))
                    succeeded.append(uign);
                // Failed to remove given complete pattern
                else
                    failed.append(uign);
            }

            // Print all successful unignores, in case there were any
            if (succeeded.count() >= 1)
            {
                result.output = i18n("Removed %1 from your ignore list.", succeeded.join(", "));
                result.typeString = i18n("Ignore");
                result.type = Program;
            }

            // Any failed unignores
            if (failed.count() >= 1)
                result = error(i18np("No such ignore: %2", "No such ignores: %2", failed.count(), failed.join(", ")));
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_server(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty() && !m_server->isConnected() && !m_server->isConnecting())
            emit reconnectServer(QString());
        else
        {
            QStringList parameterList = input.parameter.split(' ');
            QString host = parameterList[0];
            QString password;

            if (parameterList.count() == 3)
                emit connectTo(Konversation::CreateNewConnection, parameterList[0], parameterList[1], parameterList[2]);
            else if (parameterList.count() == 2)
            {
                if (parameterList[0].contains(QRegExp(":[0-9]+$")))
                    emit connectTo(Konversation::CreateNewConnection, parameterList[0], 0, parameterList[1]);
                else
                    emit connectTo(Konversation::CreateNewConnection, parameterList[0], parameterList[1]);
            }
            else
                emit connectTo(Konversation::CreateNewConnection, parameterList[0]);
        }

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_charset(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
            return info(i18n("Current encoding is: %1",
                             QString(m_server->getIdentity()->getCodec()->name())));

        OutputFilterResult result;

        QString shortName = Konversation::IRCCharsets::self()->ambiguousNameToShortName(input.parameter);

        if (!shortName.isEmpty())
        {
            m_server->getIdentity()->setCodecName(shortName);
            emit encodingChanged();
            result = info (i18n("Switched to %1 encoding.", shortName));
        }
        else
            result = error(i18n("%1 is not a valid encoding.", input.parameter));

        return result;
    }

    OutputFilterResult OutputFilter::command_encoding(const OutputFilterInput& input)
    {
        return command_charset(input);
    }

    OutputFilterResult OutputFilter::command_setkey(const OutputFilterInput& input)
    {
        #ifdef HAVE_QCA2
        QStringList parms = input.parameter.split(' ', QString::SkipEmptyParts);

        if (parms.count() == 1 && !input.destination.isEmpty())
            parms.prepend(input.destination);
        else if (parms.count() < 2)
            return usage(i18n("Usage: %1setkey <nick|channel> <key> sets the encryption key "
                              "for nick or channel. %1setkey <key> when in a channel or query "
                              "tab sets the key for it. The key field recognizes \"cbc:\" and "
                              "\"ecb:\" prefixes to set the block cipher mode of operation to "
                              "either Cipher-Block Chaining or Electronic Codebook. The mode it "
                              "defaults to when no prefix is given can be changed in the "
                              "configuration dialog under Behavior -> Connection -> Encryption "
                              "-> Default Encryption Type, with the default for that setting being "
                              "Electronic Codebook (ECB).",
                              Preferences::self()->commandChar()));

        if (!Cipher::isFeatureAvailable(Cipher::Blowfish))
            return error(i18n("Unable to set an encryption key for %1.", parms[0]) + ' ' + Cipher::runtimeError());

        m_server->setKeyForRecipient(parms[0], QStringList(parms.mid(1)).join(" ").toLocal8Bit());

        if (isAChannel(parms[0]) && m_server->getChannelByName(parms[0]))
            m_server->getChannelByName(parms[0])->setEncryptedOutput(true);
        else if (m_server->getQueryByName(parms[0]))
            m_server->getQueryByName(parms[0])->setEncryptedOutput(true);

        return info(i18n("The key for %1 has been set.", parms[0]));
        #else
        Q_UNUSED(input);
        return error(i18n("Setting an encryption key requires Konversation to have been built "
                          "with support for the Qt Cryptographic Architecture (QCA) library. "
                          "Contact your distributor about a Konversation package with QCA "
                          "support, or rebuild Konversation with QCA present."));
        #endif
    }

    OutputFilterResult OutputFilter::command_keyx(const OutputFilterInput& input)
    {
        #ifdef HAVE_QCA2
        QStringList parms = input.parameter.split(' ', QString::SkipEmptyParts);

        if (parms.count() == 0 && !input.destination.isEmpty())
            parms.prepend(input.destination);
        else if (parms.count() !=1)
            return usage(i18n("Usage: %1keyx <nick|channel> triggers DH1080 key exchange with the target.",
                              Preferences::self()->commandChar()));
        if (!Cipher::isFeatureAvailable(Cipher::DH))
            return error(i18n("Unable to request a key exchange from %1.", parms[0]) + ' ' + Cipher::runtimeError());

        m_server->initKeyExchange(parms[0]);

        return info(i18n("Beginning DH1080 key exchange with %1.", parms[0]));
        #else
        Q_UNUSED(input);
        return error(i18n("Setting an encryption key requires Konversation to have been built "
                          "with support for the Qt Cryptographic Architecture (QCA) library. "
                          "Contact your distributor about a Konversation package with QCA "
                          "support, or rebuild Konversation with QCA present."));
        #endif
    }

    OutputFilterResult OutputFilter::command_delkey(const OutputFilterInput& _input)
    {
        OutputFilterInput input(_input);
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            input.parameter = input.destination;

        if (input.parameter.isEmpty() || input.parameter.contains(' '))
            return usage(i18n("Usage: %1delkey <nick> or <channel> deletes the encryption key for "
                              "nick or channel", Preferences::self()->commandChar()));

        if (!m_server->getKeyForRecipient(input.parameter).isEmpty())
        {
            m_server->setKeyForRecipient(input.parameter, "");

            if (isAChannel(input.parameter) && m_server->getChannelByName(input.parameter))
                m_server->getChannelByName(input.parameter)->setEncryptedOutput(false);
            else if (m_server->getQueryByName(input.parameter))
                m_server->getQueryByName(input.parameter)->setEncryptedOutput(false);

            result = info(i18n("The key for %1 has been deleted.", input.parameter));
        }
        else
            result = error(i18n("No key has been set for %1.", input.parameter));

        return result;
    }

    OutputFilterResult OutputFilter::command_showkey(const OutputFilterInput& _input)
    {
        OutputFilterInput input(_input);

        if (input.parameter.isEmpty())
            input.parameter = input.destination;

        QString key(m_server->getKeyForRecipient(input.parameter));

        QWidget* mw = Application::instance()->getMainWindow();

        if (!key.isEmpty())
            KMessageBox::information(mw, i18n("The key for %1 is \"%2\".", input.parameter, key),
                                     i18n("Blowfish"));
        else
            KMessageBox::information(mw, i18n("No key has been set for %1.", input.parameter));

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_kill(const OutputFilterInput& input)
    {
        OutputFilterResult result;

        if (input.parameter.isEmpty())
            result = usage(i18n("Usage: %1KILL <nick> [comment]", Preferences::self()->commandChar()));
        else
        {
            QString victim = input.parameter.section(' ', 0, 0);

            result.toServer = "KILL " + victim + " :" + input.parameter.mid(victim.length() + 1);
        }

        return result;
    }

    OutputFilterResult OutputFilter::command_dns(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
            return usage(i18n("Usage: %1DNS <nick>", Preferences::self()->commandChar()));
        else
            new OutputFilterResolveJob(input);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_list(const OutputFilterInput& input)
    {
        emit openChannelList(input.parameter);

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_konsole(const OutputFilterInput& /* input */)
    {
        emit openKonsolePanel();

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_queuetuner(const OutputFilterInput& input)
    {
        Application *konvApp = static_cast<Application*>(KApplication::kApplication());

        if (input.parameter.isEmpty() || input.parameter == "on")
            konvApp->showQueueTuner(true);
        else if (input.parameter == "off")
            konvApp->showQueueTuner(false);
        else
            return usage(i18n("Usage: %1queuetuner [on | off]", Preferences::self()->commandChar()));

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_sayversion(const OutputFilterInput& input)
    {
        OutputFilterResult result;
        result.output = input.parameter;

        QTextStream serverOut(&result.toServer);
        QTextStream myOut(&result.output); //<--because seek is unimplemented in QTextStreamPrivate::write(const QString &data)
        myOut
            << "Konversation: " << KGlobal::mainComponent().aboutData()->version()
            << ", Qt " << QString::fromLatin1(qVersion())
            << ", KDE SC " << KDE::versionString()
            ;
        if (QString::fromLatin1(KDE_VERSION_STRING) != KDE::versionString())
            myOut << ", KDE DP " << QString::fromLatin1(KDE_VERSION_STRING);
        if (qgetenv("KDE_FULL_SESSION").isEmpty())
            myOut << ", no KDE";

        serverOut << "PRIVMSG " << input.destination << " :" << result.output;
        return result;
    }

    OutputFilterResult OutputFilter::command_cycle(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
        {
            if (input.context)
                input.context->cycle();
            else
                kDebug() << "Parameter-less /cycle without an input context can't work.";
        }
        else
        {
            if (isParameter("app", input.parameter))
            {
                Application *konvApp = static_cast<Application*>(KApplication::kApplication());

                konvApp->restart();
            }
            else if (isParameter("server", input.parameter))
            {
                if (m_server)
                    m_server->cycle();
                else
                    kDebug() << "Told to cycle the server, but current context doesn't have one.";
            }
            else if (m_server)
            {
                if (isAChannel(input.parameter))
                {
                    Channel* channel = m_server->getChannelByName(input.parameter);

                    if (channel) channel->cycle();
                }
                else if (m_server->getQueryByName(input.parameter))
                    m_server->getQueryByName(input.parameter)->cycle();
                else
                    return usage(i18n("%1CYCLE [-APP | -SERVER] [channel | nickname]", Preferences::self()->commandChar()));
            }
        }

        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::command_clear(const OutputFilterInput& input)
    {
        if (input.parameter.isEmpty())
        {
            if (input.context)
                input.context->clear();
            else
                kDebug() << "Parameter-less /clear without an input context can't work.";
        }
        else if (m_server)
        {
            if (isParameter("all", input.parameter))
                m_server->getViewContainer()->clearAllViews();
            else
            {
                if (isAChannel(input.parameter))
                {
                    Channel* channel = m_server->getChannelByName(input.parameter);

                    if (channel) channel->clear();
                }
                else if (m_server->getQueryByName(input.parameter))
                    m_server->getQueryByName(input.parameter)->clear();
                else
                    return usage(i18n("%1CLEAR [-ALL] [channel | nickname]", Preferences::self()->commandChar()));
            }
        }

        return OutputFilterResult();
    }


    OutputFilterResult OutputFilter::command_dumpdoc(const OutputFilterInput& input)
    {
        if (input.context && input.context->getTextView())
        {
#if KDE_IS_VERSION(4,6,0)
            KDebug::Block myBlock(qPrintable(QString::number((ulong)(input.context->getTextView()), 16)));
#else
            kDebug() << "view =" << qPrintable(QString::number((ulong)(input.context->getTextView()), 16));
#endif
            kDebug() << input.context->getTextView()->document();
        }
        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::changeMode(const QString &parameter, const QString& destination,
                                                char mode, char giveTake)
    {
        OutputFilterResult result;
        // TODO: Make sure this works with +l <limit> and +k <password> also!
        QString token;
        QString tmpToken;
        QStringList nickList = parameter.split(' ');

        if(nickList.count())
        {
            // Check if the user specified a channel
            if(isAChannel(nickList[0]))
            {
                token = "MODE " + nickList[0];
                // remove the first element
                nickList.removeFirst();
            }
            // Add default destination if it is a channel
            else if(isAChannel(destination))
            {
                token = "MODE " + destination;
            }

            // Only continue if there was no error
            if(token.length())
            {
                QString modeToken;
                QString nickToken;

                modeToken = QString(" ") + QChar(giveTake);

                tmpToken = token;

                for(int index = 0; index < nickList.count(); index++)
                {
                    if(index != 0 && (index % 3) == 0)
                    {
                        token += modeToken + nickToken;
                        result.toServerList.append(token);
                        token = tmpToken;
                        nickToken.clear();
                        modeToken = QString(" ") + QChar(giveTake);
                    }
                    nickToken += ' ' + nickList[index];
                    modeToken += mode;
                }

                if(!nickToken.isEmpty())
                {
                    token += modeToken + nickToken;
                    result.toServerList.append(token);
                }
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::usage(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Usage");
        result.output = string;
        result.type = Program;
        return result;
    }

    OutputFilterResult OutputFilter::info(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Info");
        result.output = string;
        result.type = Program;
        return result;
    }

    OutputFilterResult OutputFilter::error(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Error");
        result.output = string;
        result.type = Program;
        return result;
    }

    QString OutputFilter::addNickToEmptyNickList(const QString& nick, const QString& parameter)
    {
        QStringList nickList = parameter.split(' ');
        QString newNickList;

        if (nickList.count() == 0)
        {
            newNickList = nick;
        }
        // check if list contains only target channel
        else if (nickList.count() == 1 && isAChannel(nickList[0]))
        {
            newNickList = nickList[0] + ' ' + nick;
        }
        // list contains at least one nick
        else
        {
            newNickList = parameter;
        }

        return newNickList;
    }

    // # & + and ! are *often*, but not necessarily, channel identifiers. + and ! are non-RFC,
    // so if a server doesn't offer 005 and supports + and ! channels, I think thats broken
    // behaviour on their part - not ours.
    bool OutputFilter::isAChannel(const QString &check)
    {
        if (check.isEmpty())
            return false;
        Q_ASSERT(m_server);
                                                  // XXX if we ever see the assert, we need the ternary
        return m_server? m_server->isAChannel(check) : bool(QString("#&").contains(check.at(0)));
    }

    bool OutputFilter::isParameter(const QString& parameter, const QString& string)
    {
        QRegExp rx(QString("^[\\-]{1,2}%1$").arg(parameter), Qt::CaseInsensitive);

        return rx.exactMatch(string);
    }
}
#include "outputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
