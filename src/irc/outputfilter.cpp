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
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#include "outputfilter.h"
#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "awaymanager.h"
#include "ignore.h"
#include "server.h"
#include "irccharsets.h"
#include "linkaddressbook/addressbook.h"
#include "query.h"

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qmap.h>
#include <qtextcodec.h>
#include <QByteArray>

#include <klocale.h>
#include <kdebug.h>
#include <KIO/PasswordDialog>
#include <kconfig.h>
#include <kdeversion.h>
#include <kshell.h>
#include <kmessagebox.h>


namespace Konversation
{
    OutputFilter::OutputFilter(Server* server)
        : QObject(server)
    {
        m_server = server;
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

    QStringList OutputFilter::splitForEncoding(const QString& inputLine, int max)
    {
        int sublen = 0; //The encoded length since the last split
        int charLength = 0; //the length of this char
        int lastBreakPoint = 0;

        //FIXME should we run this through the encoder first, checking with "canEncode"?
        QString text = inputLine; // the text we'll send, currently in Unicode
        QStringList finals; // The strings we're going to output

        QString channelCodecName=Preferences::channelEncoding(m_server->getDisplayName(), destination);
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

        while(text.length() > max)
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

    OutputFilterResult OutputFilter::parse(const QString& myNick,const QString& originalLine,const QString& name)
    {
        setCommandChar();

        OutputFilterResult result;
        destination=name;

        QString inputLine(originalLine);

        if(inputLine.isEmpty() || inputLine == "\n")
            return result;

        //Protect against nickserv auth being sent as a message on the off chance
        // someone didn't notice leading spaces
        {
            QString testNickServ( inputLine.trimmed() );
            if(testNickServ.startsWith(commandChar+"nickserv", Qt::CaseInsensitive)
              || testNickServ.startsWith(commandChar+"ns", Qt::CaseInsensitive))
            {
                    inputLine = testNickServ;
            }
        }

        if(!Preferences::self()->disableExpansion())
        {
            // replace placeholders
            inputLine.replace("%%","%\x01");      // make sure to protect double %%
            inputLine.replace("%B","\x02");       // replace %B with bold char
            inputLine.replace("%C","\x03");       // replace %C with color char
            inputLine.replace("%G","\x07");       // replace %G with ASCII BEL 0x07
            inputLine.replace("%I","\x09");       // replace %I with italics char
            inputLine.replace("%O","\x0f");       // replace %O with reset to default char
            inputLine.replace("%S","\x13");       // replace %S with strikethru char
            //  inputLine.replace(QRegExp("%?"),"\x15");
            inputLine.replace("%R","\x16");       // replace %R with reverse char
            inputLine.replace("%U","\x1f");       // replace %U with underline char
            inputLine.replace("%\x01","%");       // restore double %% as single %
        }

        QString line=inputLine.toLower();

        // Convert double command chars at the beginning to single ones
        if(line.startsWith(commandChar+commandChar) && !destination.isEmpty())
        {
            inputLine=inputLine.mid(1);
            goto BYPASS_COMMAND_PARSING;
        }
        // Server command?
        else if(line.startsWith(commandChar))
        {
            QString command = inputLine.section(' ', 0, 0).mid(1).toLower();
            QString parameter = inputLine.section(' ', 1);

            if (command !="topic")
                parameter = parameter.trimmed();

            if     (command == "join")     result = parseJoin(parameter);
            else if(command == "part")     result = parsePart(parameter);
            else if(command == "leave")    result = parsePart(parameter);
            else if(command == "quit")     result = parseQuit(parameter);
            else if(command == "close")    result = parseClose(parameter);
            else if(command == "notice")   result = parseNotice(parameter);
            else if(command == "j")        result = parseJoin(parameter);
            else if(command == "me")       result = parseMe(parameter, destination);
            else if(command == "msg")      result = parseMsg(myNick,parameter, false);
            else if(command == "m")        result = parseMsg(myNick,parameter, false);
            else if(command == "smsg")     result = parseSMsg(parameter);
            else if(command == "query")    result = parseMsg(myNick,parameter, true);
            else if(command == "op")       result = parseOp(parameter);
            else if(command == "deop")     result = parseDeop(myNick,parameter);
            else if(command == "hop")      result = parseHop(parameter);
            else if(command == "dehop")    result = parseDehop(myNick,parameter);
            else if(command == "voice")    result = parseVoice(parameter);
            else if(command == "unvoice")  result = parseUnvoice(myNick,parameter);
            else if(command == "devoice")  result = parseUnvoice(myNick,parameter);
            else if(command == "ctcp")     result = parseCtcp(parameter);
            else if(command == "ping")     result = parseCtcp(parameter.section(' ', 0, 0) + " ping");
            else if(command == "kick")     result = parseKick(parameter);
            else if(command == "topic")    result = parseTopic(parameter);
            else if(command == "away")     parseAway(parameter);
            else if(command == "unaway")   parseBack();
            else if(command == "back")     parseBack();
            else if(command == "invite")   result = parseInvite(parameter);
            else if(command == "exec")     result = parseExec(parameter);
            else if(command == "notify")   result = parseNotify(parameter);
            else if(command == "oper")     result = parseOper(myNick,parameter);
            else if(command == "ban")      result = parseBan(parameter);
            else if(command == "unban")    result = parseUnban(parameter);
            else if(command == "kickban")  result = parseBan(parameter,true);
            else if(command == "ignore")   result = parseIgnore(parameter);
            else if(command == "unignore") result = parseUnignore(parameter);
            else if(command == "quote")    result = parseQuote(parameter);
            else if(command == "say")      result = parseSay(parameter);
            else if(command == "list")     result = parseList(parameter);
            else if(command == "names")    result = parseNames(parameter);
            else if(command == "raw")      result = parseRaw(parameter);
            else if(command == "dcc")      result = parseDcc(parameter);
            else if(command == "konsole")  parseKonsole();
            else if(command == "aaway")    KonversationApplication::instance()->getAwayManager()->requestAllAway(parameter);
            else if(command == "aunaway")    KonversationApplication::instance()->getAwayManager()->requestAllUnaway();
            else if(command == "aback")    KonversationApplication::instance()->getAwayManager()->requestAllUnaway();
            else if(command == "ame")      result = parseAme(parameter);
            else if(command == "amsg")     result = parseAmsg(parameter);
            else if(command == "omsg")     result = parseOmsg(parameter);
            else if(command == "onotice")  result = parseOnotice(parameter);
            else if(command == "server")   parseServer(parameter);
            else if(command == "reconnect")  emit reconnectServer();
            else if(command == "disconnect") emit disconnectServer();
            else if(command == "charset")  result = parseCharset(parameter);
            else if(command == "encoding")  result = parseCharset(parameter);
            else if(command == "setkey")   result = parseSetKey(parameter);
            else if(command == "delkey")   result = parseDelKey(parameter);
            else if(command == "showkey")  result = parseShowKey(parameter);
            else if(command == "dns")      result = parseDNS(parameter);
            else if(command == "kill")     result = parseKill(parameter);
            else if(command == "queuetuner") result = parseShowTuner(parameter);

            // Forward unknown commands to server
            else
            {
                result.toServer = inputLine.mid(1);
                result.type = Message;
            }
        }
        // Ordinary message to channel/query?
        else if(!destination.isEmpty())
        {
            BYPASS_COMMAND_PARSING:

            QStringList outputList=splitForEncoding(inputLine, m_server->getPreLength("PRIVMSG", destination));
            if (outputList.count() > 1)
            {
                result.output=QString();
                result.outputList=outputList;
                for ( QStringList::ConstIterator it = outputList.constBegin(); it != outputList.constEnd(); ++it )
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

    OutputFilterResult OutputFilter::parseShowTuner(const QString &parameter)
    {
        KonversationApplication *konvApp = static_cast<KonversationApplication*>(KApplication::kApplication());
        OutputFilterResult result;

        if(parameter.isEmpty() || parameter == "on")
            konvApp->showQueueTuner(true);
        else if(parameter == "off")
            konvApp->showQueueTuner(false);
        else
            result = usage(i18n("Usage: %1queuetuner [on | off]", commandChar));

        return result;
    }

    OutputFilterResult OutputFilter::parseOp(const QString &parameter)
    {
        return changeMode(parameter,'o','+');
    }

    OutputFilterResult OutputFilter::parseDeop(const QString &ownNick, const QString &parameter)
    {
        return changeMode(addNickToEmptyNickList(ownNick,parameter),'o','-');
    }

    OutputFilterResult OutputFilter::parseHop(const QString &parameter)
    {
        return changeMode(parameter, 'h', '+');
    }

    OutputFilterResult OutputFilter::parseDehop(const QString &ownNick, const QString &parameter)
    {
        return changeMode(addNickToEmptyNickList(ownNick,parameter), 'h', '-');
    }

    OutputFilterResult OutputFilter::parseVoice(const QString &parameter)
    {
        return changeMode(parameter,'v','+');
    }

    OutputFilterResult OutputFilter::parseUnvoice(const QString &ownNick, const QString &parameter)
    {
        return changeMode(addNickToEmptyNickList(ownNick,parameter),'v','-');
    }

    OutputFilterResult OutputFilter::parseJoin(QString& channelName)
    {
        OutputFilterResult result;

        if(channelName.contains(",")) // Protect against #foo,0 tricks
            channelName = channelName.remove(",0");
        //else if(channelName == "0") // FIXME IRC RFC 2812 section 3.2.1

        if (channelName.isEmpty())
        {
            if (destination.isEmpty() || !isAChannel(destination))
                return usage(i18n("Usage: %1JOIN <channel> [password]", commandChar));
            channelName=destination;
        }
        else if (!isAChannel(channelName))
            channelName = "#" + channelName.trimmed();

        Channel* channel = m_server->getChannelByName(channelName);

        if (channel)
        {
            // Note that this relies on the channels-flush-nicklists-on-disconnect behavior.
            if (!channel->numberOfNicks())
                result.toServer = "JOIN " + channelName;

            if (channel->joined()) emit showView (channel);
        }
        else
            result.toServer = "JOIN " + channelName;

        return result;
    }

    OutputFilterResult OutputFilter::parseKick(const QString &parameter)
    {
        OutputFilterResult result;

        if(isAChannel(destination))
        {
            // get nick to kick
            QString victim = parameter.left(parameter.indexOf(' '));

            if(victim.isEmpty())
            {
                result = usage(i18n("Usage: %1KICK <nick> [reason]", commandChar));
            }
            else
            {
                // get kick reason (if any)
                QString reason = parameter.mid(victim.length() + 1);

                // if no reason given, take default reason
                if(reason.isEmpty())
                {
                    reason = m_server->getIdentity()->getKickReason();
                }

                result.toServer = "KICK " + destination + ' ' + victim + " :" + reason;
            }
        }
        else
        {
            result = error(i18n("%1KICK only works from within channels.", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parsePart(const QString &parameter)
    {
        OutputFilterResult result;

        // No parameter, try default part message
        if(parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if(isAChannel(destination))
            {
                result.toServer = "PART " + destination + " :" + m_server->getIdentity()->getPartReason();
            }
            else
            {
                result = error(i18n("%1PART without parameters only works from within a channel or a query.", commandChar));
            }
        }
        else
        {
            // part a given channel
            if(isAChannel(parameter))
            {
                // get channel name
                QString channel = parameter.left(parameter.indexOf(' '));
                // get part reason (if any)
                QString reason = parameter.mid(channel.length() + 1);

                // if no reason given, take default reason
                if(reason.isEmpty())
                {
                    reason = m_server->getIdentity()->getPartReason();
                }

                result.toServer = "PART " + channel + " :" + reason;
            }
            // part this channel with a given reason
            else
            {
                if(isAChannel(destination))
                {
                    result.toServer = "PART " + destination + " :" + parameter;
                }
                else
                {
                    result = error(i18n("%1PART without channel name only works from within a channel.", commandChar));
                }
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseTopic(const QString &parameter)
    {
        OutputFilterResult result;

        // No parameter, try to get current topic
        if(parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if(isAChannel(destination))
            {
                result.toServer = "TOPIC " + destination;
            }
            else
            {
                result = error(i18n("%1TOPIC without parameters only works from within a channel.", commandChar));
            }
        }
        else
        {
            // retrieve or set topic of a given channel
            if(isAChannel(parameter))
            {
                // get channel name
                QString channel=parameter.left(parameter.indexOf(' '));
                // get topic (if any)
                QString topic=parameter.mid(channel.length()+1);
                // if no topic given, retrieve topic
                if(topic.isEmpty())
                {
                    m_server->requestTopic(channel);
                }
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
                if(isAChannel(destination))
                {
                    result.toServer = "TOPIC " + destination + " :" + parameter;
                }
                else
                {
                    result = error(i18n("%1TOPIC without channel name only works from within a channel.", commandChar));
                }
            }
        }

        return result;
    }

    void OutputFilter::parseAway(QString& reason)
    {
        if (reason.isEmpty() && m_server->isAway())
            m_server->requestUnaway();
        else
            m_server->requestAway(reason);
    }

    void OutputFilter::parseBack()
    {
        m_server->requestUnaway();
    }

    OutputFilterResult OutputFilter::parseNames(const QString &parameter)
    {
        OutputFilterResult result;
        result.toServer = "NAMES ";
        if (parameter.isNull())
        {
            return error(i18n("%1NAMES with no target may disconnect you from the server. Specify '*' if you really want this.", commandChar));
        }
        else if (parameter != QChar('*'))
        {
            result.toServer.append(parameter);
        }
        return result;
    }

    OutputFilterResult OutputFilter::parseClose(QString parm)
    {
        if (parm.isEmpty())
            parm=destination;

        if (isAChannel(parm) && m_server->getChannelByName(parm))
            m_server->getChannelByName(parm)->closeYourself(false);
        else if (m_server->getQueryByName(parm))
            m_server->getQueryByName(parm)->closeYourself(false);
        else if (parm.isEmpty()) // this can only mean one thing.. we're in the Server tab
            m_server->closeYourself(false);
        else
            return usage(i18n("Usage: %1close [window] closes the named channel or query tab, or the current tab if none specified.", commandChar));
        return OutputFilterResult();
    }

    OutputFilterResult OutputFilter::parseQuit(const QString &reason)
    {
        OutputFilterResult result;

        result.toServer = "QUIT :";
        // if no reason given, take default reason
        if(reason.isEmpty())
            result.toServer += m_server->getIdentity()->getQuitReason();
        else
            result.toServer += reason;

        return result;
    }

    OutputFilterResult OutputFilter::parseNotice(const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.indexOf(' '));
        QString message = parameter.mid(recipient.length()+1);

        if(parameter.isEmpty() || message.isEmpty())
        {
            result = usage(i18n("Usage: %1NOTICE <recipient> <message>", commandChar));
        }
        else
        {
            result.typeString = i18n("Notice");
            result.toServer = "NOTICE " + recipient + " :" + message;
            result.output=i18nc("%1 is the message, %2 the recipient nickname","Sending notice \"%2\" to %1.", recipient, message);
            result.type = Program;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseMe(const QString &parameter, const QString &destination)
    {
        OutputFilterResult result;

        if (!destination.isEmpty() && !parameter.isEmpty())
        {
            result.toServer = "PRIVMSG " + destination + " :" + '\x01' + "ACTION " + parameter + '\x01';
            result.output = parameter;
            result.type = Action;
        }
        else
        {
            result = usage(i18n("Usage: %1ME text", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseMsg(const QString &myNick, const QString &parameter, bool isQuery)
    {
        OutputFilterResult result;
        QString recipient = parameter.section(" ", 0, 0, QString::SectionSkipEmpty);
        QString message = parameter.section(" ", 1);
        QString output;

        if (recipient.isEmpty())
        {
            result = error("Error: You need to specify a recipient.");
            return result;
        }

        if (isQuery && m_server->isAChannel(recipient))
        {
            result = error("Error: You cannot open queries to channels.");
            return result;
        }

        if (message.trimmed().isEmpty())
        {
            //empty result - we don't want to send any message to the server
            if (!isQuery)
            {
                 result = error("Error: You need to specify a message.");
                return result;
            }
        }
        else if (message.startsWith(commandChar+"me"))
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
            output = QString("* %1 %2").arg(myNick).arg(message.mid(4));
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + message;
            output = message;
        }

        ::Query *query;

        if (isQuery || output.isEmpty())
        {
            //if this is a /query, always open a query window.
            //treat "/msg nick" as "/query nick"

            //Note we have to be a bit careful here.
            //We only want to create ('obtain') a new nickinfo if we have done /query
            //or "/msg nick".  Not "/msg nick message".
            NickInfoPtr nickInfo = m_server->obtainNickInfo(recipient);
            query = m_server->addQuery(nickInfo, true /*we initiated*/);
            //force focus if the user did not specify any message
            if (output.isEmpty()) emit showView(query);
        }
        else
        {
            //We have  "/msg nick message"
            query = m_server->getQueryByName(recipient);
        }

        if (query && !output.isEmpty())
        {
            if (message.startsWith(commandChar+"me"))
                                                  //log if and only if the query open
                query->appendAction(m_server->getNickname(), message.mid(4));
            else
                                                  //log if and only if the query open
                query->appendQuery(m_server->getNickname(), output);
        }

        if (output.isEmpty()) return result;       //result should be completely empty;
        //FIXME - don't do below line if query is focused
        result.output = output;
        result.typeString= recipient;
        result.type = PrivateMessage;
        return result;
    }

    OutputFilterResult OutputFilter::parseSMsg(const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.indexOf(' '));
        QString message = parameter.mid(recipient.length() + 1);

        if(message.startsWith(commandChar + "me"))
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + message;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseCtcp(const QString &parameter)
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
            unsigned int time_t = QDateTime::currentDateTime().toTime_t();
            result.toServer = QString("PRIVMSG %1 :\x01PING %2\x01").arg(recipient).arg(time_t);
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

    OutputFilterResult OutputFilter::changeMode(const QString &parameter,char mode,char giveTake)
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

    OutputFilterResult OutputFilter::parseDcc(const QString &parameter)
    {
        OutputFilterResult result;

        // No parameter, just open DCC panel
        if(parameter.isEmpty())
        {
            emit addDccPanel();
        }
        else
        {
            QString tmpParameter = parameter;
            QStringList parameterList = tmpParameter.replace("\\ ", "%20").split(' ');

            QString dccType = parameterList[0].toLower();

            if(dccType=="close")
            {
                emit closeDccPanel();
            }
            else if(dccType=="send")
            {
                if(parameterList.count()==1)      // DCC SEND
                {
                    emit requestDccSend();
                }                                 // DCC SEND <nickname>
                else if(parameterList.count()==2)
                {
                    emit requestDccSend(parameterList[1]);
                }                                 // DCC SEND <nickname> <file> [file] ...
                else if(parameterList.count()>2)
                {
                    // TODO: make sure this will work:
                    //output=i18n("Usage: %1DCC SEND nickname [fi6lename] [filename] ...").arg(commandChar);
                    KUrl fileURL(parameterList[2]);

                    //We could easily check if the remote file exists, but then we might
                    //end up asking for creditionals twice, so settle for only checking locally
                    if(!fileURL.isLocalFile() || QFile::exists( fileURL.path() ))
                    {
                        emit openDccSend(parameterList[1],fileURL);
                    }
                    else
                    {
                        result = error(i18n("File \"%1\" does not exist.", parameterList[2]));
                    }
                }
                else                              // Don't know how this should happen, but ...
                {
                    result = usage(i18n("Usage: %1DCC [SEND nickname filename]", commandChar));
                }
            }
            // TODO: DCC Chat etc. comes here
            else if(dccType=="chat")
            {
                if(parameterList.count()==2)
                {
                    emit openDccChat(parameterList[1]);
                }
                else
                {
                    result = usage(i18n("Usage: %1DCC [CHAT nickname]", commandChar));
                }
            }
            else
            {
                result = error(i18n("Unrecognized command %1DCC %2. Possible commands are SEND, CHAT, CLOSE.", commandChar, parameterList[0]));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::sendRequest(const QString &recipient,const QString &fileName,const QString &address,uint port,unsigned long size)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + ' ' + QString::number(port) + ' ' + QString::number(size) + '\x01';

        return result;
    }

    OutputFilterResult OutputFilter::passiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,unsigned long size,const QString &token)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + " 0 " + QString::number(size) + ' ' + token + '\x01';

        return result;
    }

    // Accepting Resume Request
    OutputFilterResult OutputFilter::acceptResumeRequest(const QString &recipient,const QString &fileName,uint port,int startAt)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + ' ' + QString::number(port)
            + ' ' + QString::number(startAt) + '\x01';

        return result;
    }

    // Accepting Passive Resume Request
    OutputFilterResult OutputFilter::acceptPassiveResumeRequest(const QString &recipient,const QString &fileName,uint port,int startAt,const QString &token)
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

    // Appect Passive Send Request, there aktive doesnt need that
    OutputFilterResult OutputFilter::acceptPassiveSendRequest(const QString& recipient,const QString &fileName,const QString &address,uint port,unsigned long size,const QString &token)
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


    OutputFilterResult OutputFilter::parseInvite(const QString &parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1INVITE <nick> [channel]", commandChar));
        }
        else
        {
            QString nick = parameter.section(' ', 0, 0, QString::SectionSkipEmpty);
            QString channel = parameter.section(' ', 1, 1, QString::SectionSkipEmpty);

            if(channel.isEmpty())
            {
                if(isAChannel(destination))
                {
                    channel = destination;
                }
                else
                {
                    result = error(i18n("%1INVITE without channel name works only from within channels.", commandChar));
                }
            }

            if(!channel.isEmpty())
            {
                if(isAChannel(channel))
                {
                    result.toServer = "INVITE " + nick + ' ' + channel;
                }
                else
                {
                    result = error(i18n("%1 is not a channel.", channel));
                }
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseExec(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1EXEC <script> [parameter list]", commandChar));
        }
        else
        {
            QStringList parameterList = parameter.split(' ');

            if(!parameterList[0].contains("../"))
            {
                emit launchScript(destination, parameter);
            }
            else
            {
                result = error(i18n("Script name may not contain \"../\"!"));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseRaw(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty() || parameter == "open")
        {
            emit openRawLog(true);
        }
        else if(parameter == "close")
        {
            emit closeRawLog();
        }
        else
        {
            result = usage(i18n("Usage: %1RAW [OPEN | CLOSE]", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseNotify(const QString& parameter)
    {
        OutputFilterResult result;

        QString groupName = m_server->getDisplayName();

        int serverGroupId = -1;

        if (m_server->getServerGroup())
            serverGroupId = m_server->getServerGroup()->id();

        if (!parameter.isEmpty() && serverGroupId != -1)
        {
            QStringList list = parameter.split(' ');

            for(int index = 0; index < list.count(); index++)
            {
                // Try to remove current pattern
                if(!Preferences::removeNotify(groupName, list[index]))
                {
                    // If remove failed, try to add it instead
                    if(!Preferences::addNotify(serverGroupId, list[index]))
                    {
                        kDebug() << "Adding failed!";
                    }
                }
            }                                     // endfor
        }

        // show (new) notify list to user
        //TODO FIXME uh, wtf? my brain has no room in it for this kind of fucking shit

        QString list = Preferences::notifyStringByGroupName(groupName) + ' ' + Konversation::Addressbook::self()->allContactsNicksForServer(m_server->getServerName(), m_server->getDisplayName()).join(" ");

        result.typeString = i18n("Notify");

        if(list.isEmpty())
            result.output = i18n("Current notify list is empty.");
        else
           result.output = i18n("Current notify list: %1", list);

        result.type = Program;
        return result;
    }

    OutputFilterResult OutputFilter::parseOper(const QString& myNick,const QString& parameter)
    {
        OutputFilterResult result;
        QStringList parameterList = parameter.split(' ');

        if(parameter.isEmpty() || parameterList.count() == 1)
        {
            QString nick((parameterList.count() == 1) ? parameterList[0] : myNick);
            QString password;
            bool keep = false;

            int ret = KIO::PasswordDialog::getNameAndPassword
                (
                nick,
                password,
                &keep,
                i18n("Enter user name and password for IRC operator privileges:"),
                false,
                i18n("IRC Operator Password")
                );

            if(ret == KIO::PasswordDialog::Accepted)
            {
                result.toServer = "OPER " + nick + ' ' + password;
            }
        }
        else
        {
            result.toServer = "OPER " + parameter;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseBan(const QString& parameter, bool kick)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage = true;

        if(!parameter.isEmpty())
        {
            QStringList parameterList=parameter.split(' ');
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
                else if (isAChannel(destination))
                    channel = destination;
                else
                {
                    // destination is no channel => error
                    if (!kick)
                        result = error(i18n("%1BAN without channel name works only from inside a channel.", commandChar));
                    else
                        result = error(i18n("%1KICKBAN without channel name works only from inside a channel.", commandChar));

                    // no usage information after error
                    showUsage = false;
                }
                // signal server to ban this user if all went fine
                if (!channel.isEmpty())
                {
                    if (kick)
                    {
                        QString victim = parameterList[0];
                        parameterList.pop_front();

                        QString reason = parameterList.join(" ");

                        result.toServer = "KICK " + channel + ' ' + victim + " :" + reason;

                        emit banUsers(QStringList(victim),channel,option);
                    }
                    else
                    {
                        emit banUsers(parameterList,channel,option);
                    }

                    // syntax was correct, so reset flag
                    showUsage = false;
                }
            }
        }

        if (showUsage)
        {
            if (!kick)
                result = usage(i18n("Usage: %1BAN [-HOST | -DOMAIN | -USERHOST | -USERDOMAIN] [channel] <user|mask>", commandChar));
            else
                result = usage(i18n("Usage: %1KICKBAN [-HOST | -DOMAIN | -USERHOST | -USERDOMAIN] [channel] <user|mask> [reason]", commandChar));
        }

        return result;
    }

    // finally set the ban
    OutputFilterResult OutputFilter::execBan(const QString& mask,const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " +b " + mask;
        return result;
    }

    OutputFilterResult OutputFilter::parseUnban(const QString& parameter)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage=true;

        if(!parameter.isEmpty())
        {
            QStringList parameterList = parameter.split(' ');
            QString channel;
            QString mask;

            // if the user specified a channel
            if(isAChannel(parameterList[0]))
            {
                // get channel
                channel = parameterList[0];
                // remove channel from parameter list
                parameterList.pop_front();
            }
            // otherwise the current destination must be a channel
            else if(isAChannel(destination))
                channel = destination;
            else
            {
                // destination is no channel => error
                result = error(i18n("%1UNBAN without channel name works only from inside a channel.", commandChar));
                // no usage information after error
                showUsage = false;
            }
            // if all went good, signal server to unban this mask
            if(!channel.isEmpty())
            {
                emit unbanUsers(parameterList[0], channel);
                // syntax was correct, so reset flag
                showUsage = false;
            }
        }

        if(showUsage)
        {
            result = usage(i18n("Usage: %1UNBAN [channel] pattern", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::execUnban(const QString& mask,const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " -b " + mask;
        return result;
    }

    OutputFilterResult OutputFilter::parseIgnore(const QString& parameter)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage = true;

        // did the user give parameters at all?
        if(!parameter.isEmpty())
        {
            QStringList parameterList = parameter.split(' ');

            // if nothing else said, only ignore channels and queries
            int value = Ignore::Channel | Ignore::Query;

            // user specified -all option
            if(parameterList[0].toLower() == "-all")
            {
                // ignore everything
                value = Ignore::All;
                parameterList.pop_front();
            }

            // were there enough parameters?
            if(parameterList.count() >= 1)
            {
                for(int index=0;index<parameterList.count();index++)
                {
                    if(!parameterList[index].contains('!'))
                    {
                        parameterList[index] += "!*";
                    }

                    Preferences::addIgnore(parameterList[index] + ',' + QString::number(value));
                }

                result.output = i18n("Added %1 to your ignore list.", parameterList.join(", "));
                result.typeString = i18n("Ignore");
                result.type = Program;

                // all went fine, so show no error message
                showUsage = false;
            }
        }

        if(showUsage)
        {
            result = usage(i18n("Usage: %1IGNORE [ -ALL ] <user 1> <user 2> ... <user n>", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseUnignore(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1UNIGNORE <user 1> <user 2> ... <user n>", commandChar));
        }
        else
        {
            QString unignore = parameter.simplified();
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
            if (succeeded.count()>=1)
            {
                m_server->appendMessageToFrontmost(i18n("Ignore"),i18n("Removed %1 from your ignore list.", succeeded.join(", ")));
            }

            // One failed unignore
            if (failed.count()==1)
            {
                m_server->appendMessageToFrontmost(i18n("Error"),i18n("No such ignore: %1", failed.join(", ")));
            }

            // Multiple failed unignores
            if (failed.count()>1)
            {
                m_server->appendMessageToFrontmost(i18n("Error"),i18n("No such ignores: %1", failed.join(", ")));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseQuote(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1QUOTE command list", commandChar));
        }
        else
        {
            result.toServer = parameter;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseSay(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1SAY text", commandChar));
        }
        else
        {
            result.toServer = "PRIVMSG " + destination + " :" + parameter;
            result.output = parameter;
        }

        return result;
    }

    void OutputFilter::parseKonsole()
    {
        emit openKonsolePanel();
    }

    // Accessors

    void OutputFilter::setCommandChar() { commandChar=Preferences::self()->commandChar(); }

    // # & + and ! are *often*, but not necessarily, channel identifiers. + and ! are non-RFC, so if a server doesn't offer 005 and
    // supports + and ! channels, I think thats broken behaviour on their part - not ours.
    bool OutputFilter::isAChannel(const QString &check)
    {
        if (check.isEmpty())
            return false;
        Q_ASSERT(m_server);
                                                  // XXX if we ever see the assert, we need the ternary
        return m_server? m_server->isAChannel(check) : QString("#&").contains(check.at(0));
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

    OutputFilterResult OutputFilter::parseAme(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1AME text", commandChar));
        }

        emit multiServerCommand("me", parameter);
        return result;
    }

    OutputFilterResult OutputFilter::parseAmsg(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1AMSG text", commandChar));
        }

        emit multiServerCommand("msg", parameter);
        return result;
    }

    void OutputFilter::parseServer(const QString& parameter)
    {
        if (parameter.isEmpty() && !m_server->isConnected() && !m_server->isConnecting())
            emit reconnectServer();
        else
        {
            QStringList splitted = parameter.split(' ');
            QString host = splitted[0];
            QString port = "6667";
            QString password;

            if (splitted.count() == 3)
                emit connectTo(Konversation::CreateNewConnection, splitted[0], splitted[1].toUInt(), splitted[2]);
            else if (splitted.count() == 2)
            {
                if (splitted[0].contains(QRegExp(":[0-9]+$")))
                    emit connectTo(Konversation::CreateNewConnection, splitted[0], 0, splitted[1]);
                else
                    emit connectTo(Konversation::CreateNewConnection, splitted[0], splitted[1].toUInt());
            }
            else
                emit connectTo(Konversation::CreateNewConnection, splitted[0]);
        }
    }

    OutputFilterResult OutputFilter::parseOmsg(const QString& parameter)
    {
        OutputFilterResult result;

        if(!parameter.isEmpty())
        {
            result.toServer = "PRIVMSG @"+destination+" :"+parameter;
        }
        else
        {
            result = usage(i18n("Usage: %1OMSG text", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseOnotice(const QString& parameter)
    {
        OutputFilterResult result;

        if(!parameter.isEmpty())
        {
            result.toServer = "NOTICE @"+destination+" :"+parameter;
            result.typeString = i18n("Notice");
            result.type = Program;
            result.output = i18n("Sending notice \"%1\" to %2.", parameter, destination);
        }
        else
        {
            result = usage(i18n("Usage: %1ONOTICE text", commandChar));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseCharset(const QString& charset)
    {
        OutputFilterResult result;

        if (charset.isEmpty ())
        {
            result = info (i18n("Current encoding is: %1",
                                QString(m_server->getIdentity()->getCodec()->name())));
            return result;
        }

        QString shortName = Konversation::IRCCharsets::self()->ambiguousNameToShortName(charset);

        if(!shortName.isEmpty())
        {
            m_server->getIdentity()->setCodecName(shortName);
            emit encodingChanged();
            result = info (i18n("Switched to %1 encoding.", shortName));
        }
        else
        {
            result = error(i18n("%1 is not a valid encoding.", charset));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseSetKey(const QString& parameter)
    {

        QStringList parms = parameter.split(' ', QString::SkipEmptyParts);

        if (parms.count() == 1 && !destination.isEmpty())
            parms.prepend(destination);
        else if (parms.count() != 2)
            return usage(i18n("Usage: %1setkey [<nick|channel>] <key> sets the encryption key for nick or channel. %2setkey <key> when in a channel or query tab to set the key for it.", commandChar, commandChar) );

        m_server->setKeyForRecipient(parms[0], parms[1].toLocal8Bit());

        if (isAChannel(parms[0]) && m_server->getChannelByName(parms[0]))
            m_server->getChannelByName(parms[0])->setEncryptedOutput(true);
        else if (m_server->getQueryByName(parms[0]))
            m_server->getQueryByName(parms[0])->setEncryptedOutput(true);

        return info(i18n("The key for %1 has been set.", parms[0]));
    }

    OutputFilterResult OutputFilter::parseDelKey(const QString& prametr)
    {
        QString parameter(prametr.isEmpty()?destination:prametr);

        if(parameter.isEmpty() || parameter.contains(' '))
            return usage(i18n("Usage: %1delkey <nick> or <channel> deletes the encryption key for nick or channel", commandChar));

        m_server->setKeyForRecipient(parameter, "");

        if (isAChannel(parameter) && m_server->getChannelByName(parameter))
            m_server->getChannelByName(parameter)->setEncryptedOutput(false);
        else if (m_server->getQueryByName(parameter))
            m_server->getQueryByName(parameter)->setEncryptedOutput(false);

        return info(i18n("The key for %1 has been deleted.", parameter));
    }

    OutputFilterResult OutputFilter::parseShowKey(const QString& prametr)
    {
        QString parameter(prametr.isEmpty()?destination:prametr);
        QString key(m_server->getKeyForRecipient(parameter));
        QWidget *mw=KonversationApplication::instance()->getMainWindow();
        if (!key.isEmpty())
            KMessageBox::information(mw, i18n("The key for %1 is \"%2\".", parameter, key), i18n("Blowfish"));
        else
            KMessageBox::information(mw, i18n("No key has been set for %1.", parameter));
        OutputFilterResult result;
        return result;
    }

    OutputFilterResult OutputFilter::parseList(const QString& parameter)
    {
        OutputFilterResult result;

        emit openChannelList(parameter, true);

        return result;
    }

    OutputFilterResult OutputFilter::parseDNS(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1DNS <nick>", commandChar));
        }
        else
        {
            QStringList splitted = parameter.split(' ');
            QString target = splitted[0];

            QHostAddress address(target);

            // Parameter is an IP address
            if (address != QHostAddress::Null)
            {
                QHostInfo resolved = QHostInfo::fromName(address.toString());
                if (resolved.error() == QHostInfo::NoError)
                {
                    result.typeString = i18n("DNS");
                    result.output = i18n("Resolved %1 to: %2", target, resolved.hostName());
                    result.type = Program;
                }
                else
                {
                    result = error(i18n("Unable to resolve %1", target));
                }
            }
            // Parameter is presumed to be a host due to containing a dot. Yeah, it's dumb.
            // FIXME: The reason we detect the host by occurrence of a dot is the large penalty
            // we would incur by using inputfilter to find out if there's a user==target on the
            // server - once we have a better API for this, switch to it.
            else if (target.contains('.'))
            {
                QHostInfo resolved = QHostInfo::fromName(target);
                if(resolved.error() == QHostInfo::NoError && !resolved.addresses().isEmpty())
                {
                    QString resolvedTarget = resolved.addresses().first().toString();
                    result.typeString = i18n("DNS");
                    result.output = i18n("Resolved %1 to: %2", target, resolvedTarget);
                    result.type = Program;
                }
                else
                {
                    result = error(i18n("Unable to resolve %1", target));
                }
            }
            // Parameter is either host nor IP, so request a lookup from server, which in
            // turn lets inputfilter do the job.
            else
            {
                m_server->resolveUserhost(target);
            }
        }

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

    OutputFilterResult OutputFilter::parseKill(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1KILL <nick> [comment]", commandChar));
        }
        else
        {
            QString victim = parameter.section(' ', 0, 0);
            result.toServer = "KILL " + victim + " :" + parameter.mid(victim.length() + 1);
        }

        return result;
    }
}
#include "outputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
