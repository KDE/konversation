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
#include "awaymanager.h"
#include "ignore.h"
#include "server.h"
#include "irccharsets.h"
#include "linkaddressbook/addressbook.h"
#include "query.h"
#include "viewcontainer.h"
#include <config-konversation.h>

#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTextCodec>
#include <QByteArray>

#include <KIO/PasswordDialog>
#include <KMessageBox>


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

    QStringList OutputFilter::splitForEncoding(const QString& inputLine, int max, int segments)
    {
        int sublen = 0; //The encoded length since the last split
        int charLength = 0; //the length of this char
        int lastBreakPoint = 0;

        //FIXME should we run this through the encoder first, checking with "canEncode"?
        QString text = inputLine; // the text we'll send, currently in Unicode
        QStringList finals; // The strings we're going to output

        QString channelCodecName=Preferences::channelEncoding(m_server->getDisplayName(), m_destination);
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

        while(text.length() > max && (segments == -1 || finals.size() < segments-1))
        {
            // The most important bit - turn the current char into a QCString so we can measure it
            QByteArray ch = codec->fromUnicode(QString(text[index]));
            charLength = ch.length();

            // If adding this char puts us over the limit:
            if (charLength + sublen > max)
            {
                if(lastBreakPoint != 0)
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
        QTextCodec* codec;
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
        QString newLine = codec->fromUnicode(oldLine.constData(),oldLine.length(),&state);
        if(state.invalidChars)
        {
            int ret = KMessageBox::Continue;

            ret = KMessageBox::warningContinueCancel(m_server->getViewContainer()->getWindow(),
            i18n("The message you're sending includes characters "
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

    OutputFilterResult OutputFilter::parse(const QString& myNick, const QString& originalLine, const QString& name)
    {
        m_result = OutputFilterResult();

        m_commandChar = Preferences::self()->commandChar();
        m_myNick = myNick;
        m_destination = name;
        m_parameter.clear();

        QString inputLine(originalLine);

        if (inputLine.isEmpty() || inputLine == "\n" || checkForEncodingConflict(&inputLine,name))
            return m_result;

        //Protect against nickserv auth being sent as a message on the off chance
        // someone didn't notice leading spaces
        {
            QString testNickServ(inputLine.trimmed());
            if(testNickServ.startsWith(m_commandChar + "nickserv", Qt::CaseInsensitive)
              || testNickServ.startsWith(m_commandChar + "ns", Qt::CaseInsensitive))
            {
                    inputLine = testNickServ;
            }
        }

        //perform variable expansion according to prefs
        inputLine = Konversation::doVarExpansion(inputLine);

        QString line = inputLine.toLower();

        // Convert double command chars at the beginning to single ones
        if(line.startsWith(m_commandChar + m_commandChar) && !m_destination.isEmpty())
        {
            inputLine = inputLine.mid(1);
            goto BYPASS_COMMAND_PARSING;
        }
        // Server command?
        else if(line.startsWith(m_commandChar))
        {
            QString command = inputLine.section(' ', 0, 0).mid(1).toLower();
            m_parameter = inputLine.section(' ', 1);

            if (command !="topic")
                m_parameter = m_parameter.trimmed();

            if (supportedCommands().contains(command))
            {
                QString methodSignature("command_" + command);

                QMetaObject::invokeMethod(this, methodSignature.toLatin1().constData(), Qt::DirectConnection);
            }
            // Forward unknown commands to server
            else
            {
                m_result.toServer = inputLine.mid(1);
                m_result.type = Message;
            }
        }
        // Ordinary message to channel/query?
        else if(!m_destination.isEmpty())
        {
            BYPASS_COMMAND_PARSING:

            QStringList outputList = splitForEncoding(inputLine, m_server->getPreLength("PRIVMSG", m_destination));
            if (outputList.count() > 1)
            {
                m_result.output.clear();
                m_result.outputList = outputList;
                for ( QStringList::ConstIterator it = outputList.constBegin(); it != outputList.constEnd(); ++it )
                {
                    m_result.toServerList += "PRIVMSG " + m_destination + " :" + *it;
                }
            }
            else
            {
                m_result.output = inputLine;
                m_result.toServer = "PRIVMSG " + m_destination + " :" + inputLine;
            }

            m_result.type = Message;
        }
        // Eveything else goes to the server unchanged
        else
        {
            m_result.toServer = inputLine;
            m_result.output = inputLine;
            m_result.typeString = i18n("Raw");
            m_result.type = Program;
        }

        return m_result;
    }

    void OutputFilter::command_op()
    {
        m_result = changeMode(m_parameter,'o','+');
    }

    void OutputFilter::command_deop()
    {
        m_result = changeMode(addNickToEmptyNickList(m_myNick, m_parameter),'o','-');
    }

    void OutputFilter::command_hop()
    {
        m_result = changeMode(m_parameter, 'h', '+');
    }

    void OutputFilter::command_dehop()
    {
        m_result = changeMode(addNickToEmptyNickList(m_myNick, m_parameter), 'h', '-');
    }

    void OutputFilter::command_voice()
    {
        m_result = changeMode(m_parameter,'v','+');
    }

    void OutputFilter::command_unvoice()
    {
        m_result = changeMode(addNickToEmptyNickList(m_myNick, m_parameter),'v','-');
    }

    void OutputFilter::command_devoice()
    {
        command_unvoice();
    }

    void OutputFilter::command_join()
    {
        if(m_parameter.contains(",")) // Protect against #foo,0 tricks
            m_parameter = m_parameter.remove(",0");
        //else if(channelName == "0") // FIXME IRC RFC 2812 section 3.2.1

        if (m_parameter.isEmpty())
        {
            if (m_destination.isEmpty() || !isAChannel(m_destination))
            {
                m_result = usage(i18n("Usage: %1JOIN <channel> [password]", m_commandChar));
                return;
            }
            m_parameter = m_destination;
        }
        else if (!isAChannel(m_parameter))
            m_parameter = '#' + m_parameter.trimmed();

        Channel* channel = m_server->getChannelByName(m_parameter);

        if (channel)
        {
            // Note that this relies on the channels-flush-nicklists-on-disconnect behavior.
            if (!channel->numberOfNicks())
                m_result.toServer = "JOIN " + m_parameter;

            if (channel->joined()) emit showView (channel);
        }
        else
            m_result.toServer = "JOIN " + m_parameter;
    }

    void OutputFilter::command_j()
    {
        command_join();
    }

    void OutputFilter::command_kick()
    {
        if (isAChannel(m_destination))
        {
            // get nick to kick
            QString victim = m_parameter.left(m_parameter.indexOf(' '));

            if (victim.isEmpty())
                m_result = usage(i18n("Usage: %1KICK <nick> [reason]", m_commandChar));
            else
            {
                // get kick reason (if any)
                QString reason = m_parameter.mid(victim.length() + 1);

                // if no reason given, take default reason
                if (reason.isEmpty())
                    reason = m_server->getIdentity()->getKickReason();

                m_result.toServer = "KICK " + m_destination + ' ' + victim + " :" + reason;
            }
        }
        else
            m_result = error(i18n("%1KICK only works from within channels.", m_commandChar));
    }

    void OutputFilter::command_part()
    {
        // No parameter, try default part message
        if (m_parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if (isAChannel(m_destination))
                m_result.toServer = "PART " + m_destination + " :" + m_server->getIdentity()->getPartReason();
            else if (m_server->getQueryByName(m_destination))
                m_server->closeQuery(m_destination);
            else
                m_result = error(i18n("%1PART and %1LEAVE without parameters only work from within a channel or a query.", m_commandChar));
        }
        else
        {
            // part a given channel
            if (isAChannel(m_parameter))
            {
                // get channel name
                QString channel = m_parameter.left(m_parameter.indexOf(' '));
                // get part reason (if any)
                QString reason = m_parameter.mid(channel.length() + 1);

                // if no reason given, take default reason
                if (reason.isEmpty())
                    reason = m_server->getIdentity()->getPartReason();

                m_result.toServer = "PART " + channel + " :" + reason;
            }
            // part this channel with a given reason
            else
            {
                if (isAChannel(m_destination))
                    m_result.toServer = "PART " + m_destination + " :" + m_parameter;
                else
                    m_result = error(i18n("%1PART without channel name only works from within a channel.", m_commandChar));
            }
        }
    }

    void OutputFilter::command_leave()
    {
        command_part();
    }

    void OutputFilter::command_topic()
    {
        // No parameter, try to get current topic
        if (m_parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if (isAChannel(m_destination))
                m_result.toServer = "TOPIC " + m_destination;
            else
                m_result = error(i18n("%1TOPIC without parameters only works from within a channel.", m_commandChar));
        }
        else
        {
            // retrieve or set topic of a given channel
            if (isAChannel(m_parameter))
            {
                // get channel name
                QString channel = m_parameter.left(m_parameter.indexOf(' '));
                // get topic (if any)
                QString topic = m_parameter.mid(channel.length()+1);
                // if no topic given, retrieve topic
                if (topic.isEmpty())
                    m_server->requestTopic(channel);

                // otherwise set topic there
                else
                {
                    m_result.toServer = "TOPIC " + channel + " :";
                    //If we get passed a ^A as a topic its a sign we should clear the topic.
                    //Used to be a \n, but those get smashed by QStringList::split and readded later
                    //Now is not the time to fight with that. FIXME
                    //If anyone out there *can* actually set the topic to a single ^A, now they have to
                    //specify it twice to get one.
                    if (topic =="\x01\x01")
                        m_result.toServer += '\x01';
                    else if (topic!="\x01")
                        m_result.toServer += topic;
                }
            }
            // set this channel's topic
            else
            {
                if (isAChannel(m_destination))
                    m_result.toServer = "TOPIC " + m_destination + " :" + m_parameter;
                else
                    m_result = error(i18n("%1TOPIC without channel name only works from within a channel.", m_commandChar));
            }
        }
    }

    void OutputFilter::command_away()
    {
        if (m_parameter.isEmpty() && m_server->isAway())
            m_server->requestUnaway();
        else
            m_server->requestAway(m_parameter);
    }

    void OutputFilter::command_unaway()
    {
        m_server->requestUnaway();
    }

    void OutputFilter::command_back()
    {
        command_unaway();
    }

    void OutputFilter::command_aaway()
    {
        Application::instance()->getAwayManager()->requestAllAway(m_parameter);
    }

    void OutputFilter::command_aunaway()
    {
        Application::instance()->getAwayManager()->requestAllUnaway();
    }

    void OutputFilter::command_aback()
    {
        command_aunaway();
    }

    void OutputFilter::command_names()
    {
        m_result.toServer = "NAMES ";

        if (m_parameter.isNull())
            m_result = error(i18n("%1NAMES with no target may disconnect you from the server. Specify '*' if you really want this.", m_commandChar));
        else if (m_parameter != QChar('*'))
            m_result.toServer.append(m_parameter);
    }

    void OutputFilter::command_close()
    {
        if (m_parameter.isEmpty())
            m_parameter = m_destination;

        if (isAChannel(m_parameter) && m_server->getChannelByName(m_parameter))
            m_server->getChannelByName(m_parameter)->closeYourself(false);
        else if (m_server->getQueryByName(m_parameter))
            m_server->getQueryByName(m_parameter)->closeYourself(false);
        else if (m_parameter.isEmpty()) // this can only mean one thing.. we're in the Server tab
            m_server->closeYourself(false);
        else
            m_result = usage(i18n("Usage: %1close [window] closes the named channel or query tab, or the current tab if none specified.", m_commandChar));
    }

    void OutputFilter::command_quit()
    {
        m_result.toServer = "QUIT :";
        // if no reason given, take default reason
        if (m_parameter.isEmpty())
            m_result.toServer += m_server->getIdentity()->getQuitReason();
        else
            m_result.toServer += m_parameter;
    }

    void OutputFilter::command_notice()
    {
        QString recipient = m_parameter.left(m_parameter.indexOf(' '));
        QString message = m_parameter.mid(recipient.length()+1);

        if (m_parameter.isEmpty() || message.isEmpty())
            m_result = usage(i18n("Usage: %1NOTICE <recipient> <message>", m_commandChar));
        else
        {
            m_result.typeString = i18n("Notice");
            m_result.toServer = "NOTICE " + recipient + " :" + message;
            m_result.output=i18nc("%1 is the message, %2 the recipient nickname","Sending notice \"%1\" to %2.", message, recipient);
            m_result.type = Program;
        }
    }

    void OutputFilter::command_me()
    {
        if (m_destination.isEmpty() || m_parameter.isEmpty())
        {
            m_result = usage(i18n("Usage: %1ME text", m_commandChar));

            return;
        }

        QString command("PRIVMSGACTION \x01\x01");

        QStringList outputList = splitForEncoding(m_parameter, m_server->getPreLength(command, m_destination), 2);

        if (outputList.count() > 1)
        {
            command = "PRIVMSG";

            outputList += splitForEncoding(outputList.at(1), m_server->getPreLength(command, m_destination));

            outputList.removeAt(1);

            m_result.output.clear();
            m_result.outputList = outputList;

            for (int i = 0; i < outputList.count(); ++i)
            {
                if (i == 0)
                    m_result.toServerList += "PRIVMSG " + m_destination + " :" + '\x01' + "ACTION " + outputList.at(i) + '\x01';
                else
                    m_result.toServerList += "PRIVMSG " + m_destination + " :" + outputList.at(i);
            }
        }
        else
        {
            m_result.output = m_parameter;
            m_result.toServer = "PRIVMSG " + m_destination + " :" + '\x01' + "ACTION " + m_parameter + '\x01';
        }

        m_result.type = Action;
    }

    void OutputFilter::command_msg()
    {
        handleMsg(false);
    }

    void OutputFilter::command_m()
    {
        command_msg();
    }

    void OutputFilter::command_query()
    {
        handleMsg(true);
    }

    void OutputFilter::handleMsg(bool commandIsQuery)
    {
        QString recipient = m_parameter.section(' ', 0, 0, QString::SectionSkipEmpty);
        QString message = m_parameter.section(' ', 1);
        QString output;

        bool recipientIsAChannel = false;

        if (recipient.isEmpty())
        {
            m_result = error(i18n("Error: You need to specify a recipient."));

            return;
        }
        else
            recipientIsAChannel = m_server->isAChannel(recipient);

        if (commandIsQuery && recipientIsAChannel)
        {
            m_result = error(i18n("Error: You cannot open queries to channels."));

            return;
        }

        if (message.trimmed().isEmpty())
        {
            // Empty result - we don't want to send any message to the server.
            if (!commandIsQuery)
            {
                m_result = error(i18n("Error: You need to specify a message."));

                return;
            }
        }
        else
        {
            output = message;

            if (message.startsWith(m_commandChar + "me"))
                m_result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
            else
                m_result.toServer = "PRIVMSG " + recipient + " :" + message;
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
        if (output.isEmpty()) return;

        //FIXME: Don't do below line if query is focused.
        m_result.output = output;
        m_result.typeString = recipient;
        m_result.type = PrivateMessage;
    }

    void OutputFilter::command_smsg()
    {
        QString recipient = m_parameter.left(m_parameter.indexOf(' '));
        QString message = m_parameter.mid(recipient.length() + 1);

        if (message.startsWith(m_commandChar + "me"))
            m_result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
        else
            m_result.toServer = "PRIVMSG " + recipient + " :" + message;
    }

    void OutputFilter::command_ping()
    {
        handleCtcp(m_parameter.section(' ', 0, 0) + " ping");
    }

    void OutputFilter::command_ctcp()
    {
        handleCtcp(m_parameter);
    }

    void OutputFilter::handleCtcp(const QString& parameter)
    {
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
            unsigned int time_t = QDateTime::currentDateTime().toTime_t();
            m_result.toServer = QString("PRIVMSG %1 :\x01PING %2\x01").arg(recipient).arg(time_t);
            m_result.output = i18n("Sending CTCP-%1 request to %2.", QString::fromLatin1("PING"), recipient);
        }
        else
        {
            m_result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + out + '\x01';
            m_result.output = i18n("Sending CTCP-%1 request to %2.", out, recipient);
        }

        m_result.typeString = i18n("CTCP");
        m_result.type = Program;
    }

    void OutputFilter::command_ame()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1AME text", m_commandChar));

        emit multiServerCommand("me", m_parameter);
    }

    void OutputFilter::command_amsg()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1AMSG text", m_commandChar));

        emit multiServerCommand("msg", m_parameter);
    }

    void OutputFilter::command_omsg()
    {
        if (!m_parameter.isEmpty())
            m_result.toServer = "PRIVMSG @"+m_destination+" :"+m_parameter;
        else
            m_result = usage(i18n("Usage: %1OMSG text", m_commandChar));
    }

    void OutputFilter::command_onotice()
    {
        if (!m_parameter.isEmpty())
        {
            m_result.toServer = "NOTICE @"+m_destination+" :"+m_parameter;
            m_result.typeString = i18n("Notice");
            m_result.type = Program;
            m_result.output = i18n("Sending notice \"%1\" to %2.", m_parameter, m_destination);
        }
        else
            m_result = usage(i18n("Usage: %1ONOTICE text", m_commandChar));
    }

    void OutputFilter::command_quote()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1QUOTE command list", m_commandChar));
        else
            m_result.toServer = m_parameter;
    }

    void OutputFilter::command_say()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1SAY text", m_commandChar));
        else
        {
            m_result.toServer = "PRIVMSG " + m_destination + " :" + m_parameter;
            m_result.output = m_parameter;
        }
    }

    void OutputFilter::command_dcc()
    {
        kDebug() << m_parameter;
        // No parameter, just open DCC panel
        if (m_parameter.isEmpty())
        {
            emit addDccPanel();
        }
        else
        {
            QStringList parameterList = m_parameter.replace("\\ ", "%20").split(' ');

            QString dccType = parameterList[0].toLower();

            //TODO close should not just refer to the gui-panel, let it close connections
            if (dccType == "close")
            {
                emit closeDccPanel();
            }
            else if (dccType == "send")
            {
                if (parameterList.count() == 1) // DCC SEND
                    emit requestDccSend();
                else if (parameterList.count() == 2) // DCC SEND <nickname>
                    emit requestDccSend(parameterList[1]);
                else if (parameterList.count() > 2) // DCC SEND <nickname> <file> [file] ...
                {
                    // TODO: make sure this will work:
                    //output=i18n("Usage: %1DCC SEND nickname [filename] [filename] ...").arg(commandChar);
                    KUrl fileURL(parameterList[2]);

                    //We could easily check if the remote file exists, but then we might
                    //end up asking for creditionals twice, so settle for only checking locally
                    if (!fileURL.isLocalFile() || QFile::exists(fileURL.path()))
                        emit openDccSend(parameterList[1],fileURL);
                    else
                        m_result = error(i18n("File \"%1\" does not exist.", parameterList[2]));
                }
                else                              // Don't know how this should happen, but ...
                    m_result = usage(i18n("Usage: %1DCC [SEND nickname filename]", m_commandChar));
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
                        m_result = usage(i18n("Usage: %1DCC [GET [nickname [filename]]]", m_commandChar));
                }
            }
            // TODO: DCC Chat etc. comes here
            else if (dccType == "chat")
            {
                if (parameterList.count() == 2)
                    emit openDccChat(parameterList[1]);
                else
                    m_result = usage(i18n("Usage: %1DCC [CHAT nickname]", m_commandChar));
            }
            else
                m_result = error(i18n("Unrecognized command %1DCC %2. Possible commands are SEND, CHAT, CLOSE.", m_commandChar, parameterList[0]));
        }
    }

    OutputFilterResult OutputFilter::sendRequest(const QString &recipient,const QString &fileName,const QString &address,uint port,quint64 size)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + ' ' + QString::number(port) + ' ' + QString::number(size) + '\x01';

        return result;
    }

    OutputFilterResult OutputFilter::passiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,quint64 size,const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + " 0 " + QString::number(size) + ' ' + token + '\x01';

        return result;
    }

    // Accepting Resume Request
    OutputFilterResult OutputFilter::acceptResumeRequest(const QString &recipient,const QString &fileName,uint port,quint64 startAt)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + ' ' + QString::number(port)
            + ' ' + QString::number(startAt) + '\x01';

        return result;
    }

    // Accepting Passive Resume Request
    OutputFilterResult OutputFilter::acceptPassiveResumeRequest(const QString &recipient,const QString &fileName,uint port,quint64 startAt,const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + ' ' + QString::number(port)
            + ' ' + QString::number(startAt) + ' ' + token +'\x01';

        return result;
    }

    // Requesting Resume Request
    OutputFilterResult OutputFilter::resumeRequest(const QString &sender,const QString &fileName,uint port,KIO::filesize_t startAt)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + sender + " :" + '\x01' + "DCC RESUME " + fileName + ' ' + QString::number(port) + ' '
            + QString::number(startAt) + '\x01';

        return result;
    }

    // Requesting Passive Resume Request
    OutputFilterResult OutputFilter::resumePassiveRequest(const QString &sender,const QString &fileName,uint port,KIO::filesize_t startAt,const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + sender + " :" + '\x01' + "DCC RESUME " + fileName + ' ' + QString::number(port) + ' '
            + QString::number(startAt) + ' ' + token + '\x01';

        return result;
    }

    // Accept Passive Send Request, there active doesn't need that
    OutputFilterResult OutputFilter::acceptPassiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,uint port,quint64 size,const QString &token)
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

    OutputFilterResult OutputFilter::rejectDccChat(const QString & partnerNick)
    {
        OutputFilterResult result;
        result.toServer = "NOTICE " + partnerNick + " :" + '\x01' + "DCC REJECT CHAT chat" + '\x01';

        return result;
    }


    void OutputFilter::command_invite()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1INVITE <nick> [channel]", m_commandChar));
        else
        {
            QString nick = m_parameter.section(' ', 0, 0, QString::SectionSkipEmpty);
            QString channel = m_parameter.section(' ', 1, 1, QString::SectionSkipEmpty);

            if (channel.isEmpty())
            {
                if (isAChannel(m_destination))
                    channel = m_destination;
                else
                    m_result = error(i18n("%1INVITE without channel name works only from within channels.", m_commandChar));
            }

            if (!channel.isEmpty())
            {
                if (isAChannel(channel))
                    m_result.toServer = "INVITE " + nick + ' ' + channel;
                else
                    m_result = error(i18n("%1 is not a channel.", channel));
            }
        }
    }

    void OutputFilter::command_exec()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1EXEC <script> [parameter list]", m_commandChar));
        else
        {
            QStringList parameterList = m_parameter.split(' ');

            if (!parameterList[0].contains("../"))
                emit launchScript(m_destination, m_parameter);
            else
                m_result = error(i18n("Script name may not contain \"../\"."));
        }
    }

    void OutputFilter::command_raw()
    {
        if (m_parameter.isEmpty() || m_parameter == "open")
            emit openRawLog(true);
        else if (m_parameter == "close")
            emit closeRawLog();
        else
            m_result = usage(i18n("Usage: %1RAW [OPEN | CLOSE]", m_commandChar));
    }

    void OutputFilter::command_notify()
    {
        int serverGroupId = -1;

        if (m_server->getServerGroup())
            serverGroupId = m_server->getServerGroup()->id();

        if (!m_parameter.isEmpty() && serverGroupId != -1)
        {
            QStringList list = m_parameter.split(' ');

            for (int index = 0; index < list.count(); index++)
            {
                // Try to remove current pattern
                if (!Preferences::removeNotify(serverGroupId, list[index]))
                {
                    // If remove failed, try to add it instead
                    if (!Preferences::addNotify(serverGroupId, list[index]))
                        kDebug() << "Adding failed!";
                }
            }
        }

        // show (new) notify list to user
        //TODO FIXME uh, wtf? my brain has no room in it for this kind of fucking shit

        QString list = Preferences::notifyStringByGroupId(serverGroupId) + ' ' + Konversation::Addressbook::self()->allContactsNicksForServer(m_server->getServerName(), m_server->getDisplayName()).join(" ");

        m_result.typeString = i18n("Notify");

        if (list.isEmpty())
            m_result.output = i18n("Current notify list is empty.");
        else
            m_result.output = i18n("Current notify list: %1", list);

        m_result.type = Program;
    }

    void OutputFilter::command_oper()
    {
        QStringList parameterList = m_parameter.split(' ');

        if (m_parameter.isEmpty() || parameterList.count() == 1)
        {
            QString nick((parameterList.count() == 1) ? parameterList[0] : m_myNick);
            QString password;
            bool keep = false;

            int ret = KIO::PasswordDialog::getNameAndPassword
                (
                nick,
                password,
                &keep,
                i18n("Enter username and password for IRC operator privileges:"),
                false,
                i18n("IRC Operator Password")
                );

            if (ret == KIO::PasswordDialog::Accepted)
                m_result.toServer = "OPER " + nick + ' ' + password;
        }
        else
            m_result.toServer = "OPER " + m_parameter;
    }

    void OutputFilter::command_ban()
    {
        handleBan(false);
    }

    void OutputFilter::command_kickban()
    {
        handleBan(true);
    }

    void OutputFilter::handleBan(bool kick)
    {
        // assume incorrect syntax first
        bool showUsage = true;

        if (!m_parameter.isEmpty())
        {
            QStringList parameterList = m_parameter.split(' ');
            QString channel;
            QString option;
            // check for option
            QString lowerParameter = parameterList[0].toLower();
            bool host = (lowerParameter == "-host");
            bool domain = (lowerParameter == "-domain");
            bool uhost = (lowerParameter == "-userhost");
            bool udomain = (lowerParameter == "-userdomain");

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
                else if (isAChannel(m_destination))
                    channel = m_destination;
                else
                {
                    // destination is no channel => error
                    if (!kick)
                        m_result = error(i18n("%1BAN without channel name works only from inside a channel.", m_commandChar));
                    else
                        m_result = error(i18n("%1KICKBAN without channel name works only from inside a channel.", m_commandChar));

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

                        m_result.toServer = "KICK " + channel + ' ' + victim + " :" + reason;

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
                m_result = usage(i18n("Usage: %1BAN [-HOST | -DOMAIN | -USERHOST | -USERDOMAIN] [channel] <user|mask>", m_commandChar));
            else
                m_result = usage(i18n("Usage: %1KICKBAN [-HOST | -DOMAIN | -USERHOST | -USERDOMAIN] [channel] <user|mask> [reason]", m_commandChar));
        }
    }

    // finally set the ban
    OutputFilterResult OutputFilter::execBan(const QString& mask, const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " +b " + mask;
        return result;
    }

    void OutputFilter::command_unban()
    {
        // assume incorrect syntax first
        bool showUsage=true;

        if (!m_parameter.isEmpty())
        {
            QStringList parameterList = m_parameter.split(' ');
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
            else if (isAChannel(m_destination))
                channel = m_destination;
            else
            {
                // destination is no channel => error
                m_result = error(i18n("%1UNBAN without channel name works only from inside a channel.", m_commandChar));
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
            m_result = usage(i18n("Usage: %1UNBAN [channel] pattern", m_commandChar));
    }

    OutputFilterResult OutputFilter::execUnban(const QString& mask, const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " -b " + mask;
        return result;
    }

    void OutputFilter::command_ignore()
    {
        // assume incorrect syntax first
        bool showUsage = true;

        // did the user give parameters at all?
        if (!m_parameter.isEmpty())
        {
            QStringList parameterList = m_parameter.split(' ');

            // if nothing else said, only ignore channels and queries
            int value = Ignore::Channel | Ignore::Query;

            // user specified -all option
            if (parameterList[0].toLower() == "-all")
            {
                // ignore everything
                value = Ignore::All;
                parameterList.pop_front();
            }

            // were there enough parameters?
            if (parameterList.count() >= 1)
            {
                for(int index=0;index<parameterList.count();index++)
                {
                    if (!parameterList[index].contains('!'))
                        parameterList[index] += "!*";

                    Preferences::addIgnore(parameterList[index] + ',' + QString::number(value));
                }

                m_result.output = i18n("Added %1 to your ignore list.", parameterList.join(", "));
                m_result.typeString = i18n("Ignore");
                m_result.type = Program;

                // all went fine, so show no error message
                showUsage = false;
            }
        }

        if (showUsage)
            m_result = usage(i18n("Usage: %1IGNORE [ -ALL ] <user 1> <user 2> ... <user n>", m_commandChar));
    }

    void OutputFilter::command_unignore()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1UNIGNORE <user 1> <user 2> ... <user n>", m_commandChar));
        else
        {
            QString unignore = m_parameter.simplified();
            QStringList unignoreList = unignore.split(' ');

            QStringList succeeded;
            QStringList failed;

            // Iterate over potential unignores
            for (QStringList::ConstIterator it = unignoreList.constBegin(); it != unignoreList.constEnd(); ++it)
            {
                // If pattern looks incomplete, try to complete it
                if (!(*it).contains('!'))
                {
                    QString fixedPattern = (*it);
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
                    if (Preferences::removeIgnore((*it)))
                    {
                        succeeded.append((*it));
                        success = true;
                    }

                    if (!success)
                        failed.append((*it) + "[!*]");
                }
                // Try to remove seemingly complete pattern
                else if (Preferences::removeIgnore((*it)))
                    succeeded.append((*it));
                // Failed to remove given complete pattern
                else
                    failed.append((*it));
            }

            // Print all successful unignores, in case there were any
            if (succeeded.count() >= 1)
            {
                //FIXME Why is this not just using the OutputFilterResult?
                m_server->appendMessageToFrontmost(i18n("Ignore"),i18n("Removed %1 from your ignore list.", succeeded.join(", ")));
            }

            // Any failed unignores
            if (failed.count() >= 1)
            {
                //FIXME Why is this not just using the OutputFilterResult?
                m_server->appendMessageToFrontmost(i18n("Error"),i18np("No such ignore: %2", "No such ignores: %2", failed.count(), failed.join(", ")));
            }
        }
    }

    void OutputFilter::command_server()
    {
        if (m_parameter.isEmpty() && !m_server->isConnected() && !m_server->isConnecting())
            emit reconnectServer();
        else
        {
            QStringList splitted = m_parameter.split(' ');
            QString host = splitted[0];
            QString password;

            if (splitted.count() == 3)
                emit connectTo(Konversation::CreateNewConnection, splitted[0], splitted[1], splitted[2]);
            else if (splitted.count() == 2)
            {
                if (splitted[0].contains(QRegExp(":[0-9]+$")))
                    emit connectTo(Konversation::CreateNewConnection, splitted[0], 0, splitted[1]);
                else
                    emit connectTo(Konversation::CreateNewConnection, splitted[0], splitted[1]);
            }
            else
                emit connectTo(Konversation::CreateNewConnection, splitted[0]);
        }
    }

    void OutputFilter::command_reconnect()
    {
        emit reconnectServer();
    }

    void OutputFilter::command_disconnect()
    {
        emit disconnectServer();
    }

    void OutputFilter::command_charset()
    {
        if (m_parameter.isEmpty())
        {
            m_result = info (i18n("Current encoding is: %1",
                                QString(m_server->getIdentity()->getCodec()->name())));
            return;
        }

        QString shortName = Konversation::IRCCharsets::self()->ambiguousNameToShortName(m_parameter);

        if (!shortName.isEmpty())
        {
            m_server->getIdentity()->setCodecName(shortName);
            emit encodingChanged();
            m_result = info (i18n("Switched to %1 encoding.", shortName));
        }
        else
            m_result = error(i18n("%1 is not a valid encoding.", m_parameter));
    }

    void OutputFilter::command_encoding()
    {
        command_charset();
    }

    void OutputFilter::command_setkey()
    {
        QStringList parms = m_parameter.split(' ', QString::SkipEmptyParts);

        #ifdef HAVE_QCA2
        if (parms.count() == 1 && !m_destination.isEmpty())
            parms.prepend(m_destination);
        else if (parms.count() != 2)
        {
            m_result = usage(i18n("Usage: %1setkey <nick|channel> <key> sets the encryption key for nick or channel. %1setkey <key> when in a channel or query tab sets the key for it. The key field recognizes \"cbc:\" and \"ecb:\" prefixes to set the block cipher mode of operation to either Cipher-Block Chaining or Electronic Codebook. The mode it defaults to when no prefix is given can be changed in the configuration dialog under Behavior -> Connection -> Encryption -> Default Encryption Type, with the default for that setting being Electronic Codebook (ECB).", m_commandChar));

            return;
        }

        m_server->setKeyForRecipient(parms[0], parms[1].toLocal8Bit());

        if (isAChannel(parms[0]) && m_server->getChannelByName(parms[0]))
            m_server->getChannelByName(parms[0])->setEncryptedOutput(true);
        else if (m_server->getQueryByName(parms[0]))
            m_server->getQueryByName(parms[0])->setEncryptedOutput(true);

        m_result = info(i18n("The key for %1 has been set.", parms[0]));
        #else
        m_result = error(i18n("Setting an encryption key requires Konversation to have been built with support for the Qt Cryptographic Architecture (QCA) library. Contact your distributor about a Konversation package with QCA support, or rebuild Konversation with QCA present."));
        #endif
    }

    void OutputFilter::command_keyx()
    {
        QStringList parms = m_parameter.split(' ', QString::SkipEmptyParts);

        #ifdef HAVE_QCA2
        if (parms.count() == 0 && !m_destination.isEmpty())
            parms.prepend(m_destination);
        else if (parms.count() !=1)
        {
            m_result = usage(i18n("Usage: %1keyx <nick|channel> triggers DH1080 key exchange with the target.", m_commandChar));

            return;
        }

        m_server->initKeyExchange(parms[0]);

        m_result = info(i18n("Beginning DH1080 key exchange with %1.", parms[0]));
        #else
        m_result = error(i18n("Setting an encryption key requires Konversation to have been built with support for the Qt Cryptographic Architecture (QCA) library. Contact your distributor about a Konversation package with QCA support, or rebuild Konversation with QCA present."));
        #endif
    }

    void OutputFilter::command_delkey()
    {
        QString parameter(m_parameter.isEmpty () ? m_destination : m_parameter);

        if (parameter.isEmpty() || parameter.contains(' '))
        {
            m_result = usage(i18n("Usage: %1delkey <nick> or <channel> deletes the encryption key for nick or channel", m_commandChar));

            return;
        }

        if(!m_server->getKeyForRecipient(parameter).isEmpty())
        {
            m_server->setKeyForRecipient(parameter, "");

            if (isAChannel(parameter) && m_server->getChannelByName(parameter))
                m_server->getChannelByName(parameter)->setEncryptedOutput(false);
            else if (m_server->getQueryByName(parameter))
                m_server->getQueryByName(parameter)->setEncryptedOutput(false);

            m_result = info(i18n("The key for %1 has been deleted.", parameter));
        }
        else
            m_result = error(i18n("No key has been set for %1.", parameter));
    }

    void OutputFilter::command_showkey()
    {
        QString parameter(m_parameter.isEmpty() ? m_destination : m_parameter);

        QString key(m_server->getKeyForRecipient(parameter));

        QWidget* mw = Application::instance()->getMainWindow();

        if (!key.isEmpty())
            KMessageBox::information(mw, i18n("The key for %1 is \"%2\".", parameter, key), i18n("Blowfish"));
        else
            KMessageBox::information(mw, i18n("No key has been set for %1.", parameter));
    }

    void OutputFilter::command_kill()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1KILL <nick> [comment]", m_commandChar));
        else
        {
            QString victim = m_parameter.section(' ', 0, 0);

            m_result.toServer = "KILL " + victim + " :" + m_parameter.mid(victim.length() + 1);
        }
    }

    void OutputFilter::command_dns()
    {
        if (m_parameter.isEmpty())
            m_result = usage(i18n("Usage: %1DNS <nick>", m_commandChar));
        else
        {
            QStringList splitted = m_parameter.split(' ');
            QString target = splitted[0];

            QHostAddress address(target);

            // Parameter is an IP address
            if (address != QHostAddress::Null)
            {
                QHostInfo resolved = QHostInfo::fromName(address.toString());

                if (resolved.error() == QHostInfo::NoError)
                {
                    m_result.typeString = i18n("DNS");
                    m_result.output = i18n("Resolved %1 to: %2", target, resolved.hostName());
                    m_result.type = Program;
                }
                else
                    m_result = error(i18n("Unable to resolve %1", target));
            }
            // Parameter is presumed to be a host due to containing a dot. Yeah, it's dumb.
            // FIXME: The reason we detect the host by occurrence of a dot is the large penalty
            // we would incur by using inputfilter to find out if there's a user==target on the
            // server - once we have a better API for this, switch to it.
            else if (target.contains('.'))
            {
                QHostInfo resolved = QHostInfo::fromName(target);

                if (resolved.error() == QHostInfo::NoError && !resolved.addresses().isEmpty())
                {
                    QString resolvedTarget = resolved.addresses().first().toString();

                    m_result.typeString = i18n("DNS");
                    m_result.output = i18n("Resolved %1 to: %2", target, resolvedTarget);
                    m_result.type = Program;
                }
                else
                    m_result = error(i18n("Unable to resolve %1", target));
            }
            // Parameter is either host nor IP, so request a lookup from server, which in
            // turn lets inputfilter do the job.
            else
                m_server->resolveUserhost(target);
        }
    }

    void OutputFilter::command_list()
    {
        emit openChannelList(m_parameter, true);
    }

    void OutputFilter::command_konsole()
    {
        emit openKonsolePanel();
    }

    void OutputFilter::command_queuetuner()
    {
        Application *konvApp = static_cast<Application*>(KApplication::kApplication());

        if (m_parameter.isEmpty() || m_parameter == "on")
            konvApp->showQueueTuner(true);
        else if(m_parameter == "off")
            konvApp->showQueueTuner(false);
        else
            m_result = usage(i18n("Usage: %1queuetuner [on | off]", m_commandChar));
    }

    OutputFilterResult OutputFilter::changeMode(const QString &parameter, char mode, char giveTake)
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
            else if(isAChannel(m_destination))
            {
                token = "MODE " + m_destination;
            }

            // Only continue if there was no error
            if(token.length())
            {
                unsigned int modeCount = nickList.count();
                QString modes;
                modes.fill(mode, modeCount);

                token += QString(" ") + QChar(giveTake) + modes;
                tmpToken = token;

                for(unsigned int index = 0; index < modeCount; index++)
                {
                    if((index % 3) == 0)
                    {
                        result.toServerList.append(token);
                        token = tmpToken;
                    }
                    token += ' ' + nickList[index];
                }

                if(token != tmpToken)
                {
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

    // # & + and ! are *often*, but not necessarily, channel identifiers. + and ! are non-RFC, so if a server doesn't offer 005 and
    // supports + and ! channels, I think thats broken behaviour on their part - not ours.
    bool OutputFilter::isAChannel(const QString &check)
    {
        if (check.isEmpty())
            return false;
        Q_ASSERT(m_server);
                                                  // XXX if we ever see the assert, we need the ternary
        return m_server? m_server->isAChannel(check) : bool(QString("#&").contains(check.at(0)));
    }
}
#include "outputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
