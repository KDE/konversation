/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  outputfilter.cpp  -  Converts input to RFC1459 output
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qmap.h>
#include <qvaluelist.h>
#include <qtextcodec.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/passdlg.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <kshell.h>

#include "outputfilter.h"
#include "konversationapplication.h"
#include "ignore.h"
#include "server.h"
#include "irccharsets.h"
#include "linkaddressbook/addressbook.h"

namespace Konversation {
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
        QStringList aliasList=KonversationApplication::preferences.getAliasList();
        QString cc(KonversationApplication::preferences.getCommandChar());
        // check if the line starts with a defined alias
        for(unsigned int index=0;index<aliasList.count();index++)
        {
            // cut alias pattern from definition
            QString aliasPattern(aliasList[index].section(' ',0,0));

            // pattern found?
            // TODO: cc may be a regexp character here ... we should escape it then
            if(line.find(QRegExp("^"+cc+aliasPattern+"\\b"))!=-1)
            {
                QString aliasReplace;

                // cut alias replacement from definition
                if ( aliasList[index].contains("%p") )
                    aliasReplace = aliasList[index].section(' ',1);
                else
                    aliasReplace = aliasList[index].section(' ',1 )+" "+line.section(' ',1 );

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
        } // for

        return false;
    }

    OutputFilterResult OutputFilter::parse(const QString& myNick,const QString& originalLine,const QString& name)
    {
        setCommandChar();

        OutputFilterResult result;
        destination=name;

        QString inputLine(originalLine);

        if(!KonversationApplication::preferences.getDisableExpansion())
        {
            // replace placeholders
            inputLine.replace("%%","%\x01");  // make sure to protect double %%
            inputLine.replace("%B","\x02");   // replace %B with bold char
            inputLine.replace("%C","\x03");   // replace %C with color char
            inputLine.replace("%G","\x07");   // replace %G with ASCII BEL 0x07
            inputLine.replace("%I","\x09");   // replace %I with italics char
            inputLine.replace("%O","\x0f");   // replace %O with reset to default char
            inputLine.replace("%S","\x13");   // replace %S with strikethru char
            //  inputLine.replace(QRegExp("%?"),"\x15");
            inputLine.replace("%R","\x16");   // replace %R with reverse char
            inputLine.replace("%U","\x1f");   // replace %U with underline char
            inputLine.replace("%\x01","%");   // restore double %% as single %
        }

        QString line=inputLine.lower();

        // Action?
        if(line.startsWith(commandChar+"me ") && !destination.isEmpty())
        {
            result.toServer = "PRIVMSG " + name + " :" + '\x01' + "ACTION " + inputLine.mid(4) + '\x01';
            result.output = inputLine.mid(4);
            result.type = Action;
        }
        // Convert double command chars at the beginning to single ones
        else if(line.startsWith(commandChar+commandChar) && !destination.isEmpty())
        {
            result.toServer = "PRIVMSG " + name + " :" + inputLine.mid(1);
            result.output = inputLine.mid(1);
            result.type = Message;
        }
        // Server command?
        else if(line.startsWith(commandChar))
        {
	    //FIXME There will be problems with turkish locale with all of this.
	    //Look at using kstricmp instead of .lower() and ==

            QString command = inputLine.section(' ', 0, 0).mid(1).lower();
            QString parameter = inputLine.section(' ', 1);
            parameter = parameter.stripWhiteSpace();

            if     (command == "join")    result = parseJoin(parameter);
            else if(command == "part")    result = parsePart(parameter);
            else if(command == "leave")   result = parsePart(parameter);
            else if(command == "quit")    result = parseQuit(parameter);
            else if(command == "notice")  result = parseNotice(parameter);
            else if(command == "j")       result = parseJoin(parameter);
            else if(command == "msg")     result = parseMsg(myNick,parameter);
            else if(command == "smsg")    result = parseSMsg(parameter);
            else if(command == "query")   parseQuery(parameter);
            else if(command == "op")      result = parseOp(parameter);
            else if(command == "deop")    result = parseDeop(parameter);
            else if(command == "voice")   result = parseVoice(parameter);
            else if(command == "unvoice") result = parseUnvoice(parameter);
            else if(command == "ctcp")    result = parseCtcp(parameter);
	    else if(command == "ping")    result = parseCtcp(parameter.section(' ', 0, 0) + " ping");
            else if(command == "kick")    result = parseKick(parameter);
            else if(command == "topic")   result = parseTopic(parameter);
            else if(command == "away")    result = parseAway(parameter);
            else if(command == "back")    result = parseAway(QString::null);
            else if(command == "invite")  result = parseInvite(parameter);
            else if(command == "exec")    result = parseExec(parameter);
            else if(command == "notify")  result = parseNotify(parameter);
            else if(command == "oper")    result = parseOper(myNick,parameter);
            else if(command == "ban")     result = parseBan(parameter);
            else if(command == "unban")   result = parseUnban(parameter);
            else if(command == "ignore")  result = parseIgnore(parameter);
            else if(command == "quote")   result = parseQuote(parameter);
            else if(command == "say")     result = parseSay(parameter);

            else if(command == "names")   result = parseNames(parameter);
            else if(command == "raw")     result = parseRaw(parameter);
            else if(command == "dcc")     result = parseDcc(parameter);
            else if(command == "konsole") parseKonsole();

            else if(command == "aaway")   parseAaway(parameter);
            else if(command == "ame")     result = parseAme(parameter);
            else if(command == "amsg")    result = parseAmsg(parameter);

            else if(command == "server")  parseServer(parameter);

            else if(command == "prefs")   result = parsePrefs(parameter);
	    
	    else if(command == "charset") parseCharset(parameter); 

            // Forward unknown commands to server
            else {
                result.toServer = inputLine.mid(1);
                result.type = Message;
            }
        }
        // Ordinary message to channel/query?
        else if(!destination.isEmpty())
        {
            result.toServer = "PRIVMSG " + destination + " :" + inputLine;
            result.output = inputLine;
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

    OutputFilterResult OutputFilter::parseOp(const QString &parameter)
    {
        return changeMode(parameter,'o','+');
    }

    OutputFilterResult OutputFilter::parseDeop(const QString &parameter)
    {
        return changeMode(parameter,'o','-');
    }

    OutputFilterResult OutputFilter::parseVoice(const QString &parameter)
    {
        return changeMode(parameter,'v','+');
    }

    OutputFilterResult OutputFilter::parseUnvoice(const QString &parameter)
    {
        return changeMode(parameter,'v','-');
    }

    OutputFilterResult OutputFilter::parseJoin(const QString &channelName)
    {
        OutputFilterResult result;

        if(channelName.isEmpty())
        {
            result = usage(i18n("Usage: %1JOIN <channel> [password]").arg(commandChar));
        } else {
	  if(!isAChannel(channelName))
	    result.toServer = "JOIN #" + channelName.stripWhiteSpace();
	  else
	    result.toServer = "JOIN " + channelName;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseKick(const QString &parameter)
    {
        OutputFilterResult result;

        if(isAChannel(destination))
        {
            // get nick to kick
            QString victim = parameter.left(parameter.find(" "));

            if(victim.isEmpty())
            {
                result = usage(i18n("Usage: %1KICK <nick> [reason]").arg(commandChar));
            }
            else
            {
                // get kick reason (if any)
                QString reason = parameter.mid(victim.length() + 1);

                // if no reason given, take default reason
                if(reason.isEmpty()) {
                    reason = m_server->getIdentity()->getKickReason();
                }

                result.toServer = "KICK " + destination + " " + victim + " :" + reason;
            }
        }
        else
        {
            result = error(i18n("%1KICK only works from within channels.").arg(commandChar));
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
            if(isAChannel(destination)) {
                result.toServer = "PART " + destination + " :" + m_server->getIdentity()->getPartReason();
            } else {
                result = error(i18n("%1PART without parameters only works from within a channel or a query.").arg(commandChar));
            }
        } else {
            // part a given channel
            if(isAChannel(parameter))
            {
                // get channel name
                QString channel = parameter.left(parameter.find(" "));
                // get part reason (if any)
                QString reason = parameter.mid(channel.length() + 1);

                // if no reason given, take default reason
                if(reason.isEmpty()) {
                    reason = m_server->getIdentity()->getPartReason();
                }

                result.toServer = "PART " + channel + " :" + reason;
            }
            // part this channel with a given reason
            else
            {
                if(isAChannel(destination)) {
                    result.toServer = "PART " + destination + " :" + parameter;
                } else {
                    result = error(i18n("%1PART without channel name only works from within a channel.").arg(commandChar));
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
            if(isAChannel(destination)) {
                result.toServer = "TOPIC " + destination;
            } else {
                result = error(i18n("%1TOPIC without parameters only works from within a channel.").arg(commandChar));
            }
        }
        else
        {
            // retrieve or set topic of a given channel
            if(isAChannel(parameter))
            {
                // get channel name
                QString channel=parameter.left(parameter.find(" "));
                // get topic (if any)
                QString topic=parameter.mid(channel.length()+1);
                // if no topic given, retrieve topic
                if(topic.isEmpty()) {
                    result.toServer = "TOPIC " + channel;
                }
                // otherwise set topic there
                else
                {
                    result.toServer = "TOPIC " + channel + " :" + topic;
                }
            }
            // set this channel's topic
            else
            {
                if(isAChannel(destination)) {
                    result.toServer = "TOPIC " + destination + " :" + parameter;
                } else {
                    result = error(i18n("%1TOPIC without channel name only works from within a channel.").arg(commandChar));
                }
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseAway(const QString &reason)
    {
        OutputFilterResult result;

        if(reason.isEmpty())
        {
            result.toServer = "AWAY";
        }
        else
        {
            if(m_server->getIdentity()->getShowAwayMessage())
            {
                QString message = m_server->getIdentity()->getAwayMessage();
                emit sendToAllChannels(message.replace(QRegExp("%s",false),reason));
            }

            result.toServer = "AWAY :" + reason;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseNames(const QString &parameter) {
        OutputFilterResult result;
        result.toServer = "NAMES ";
        if (parameter.isNull()) {
            return error(i18n("NAMES with no target may disconnect you from the server. Specify '*' if you really want this."));
        }
        else if (parameter != QChar('*')) {
            result.toServer.append(parameter);
        }
        return result;
    }

    OutputFilterResult OutputFilter::parseQuit(const QString &reason)
    {
        OutputFilterResult result;

        result.toServer = "QUIT :";
        // if no reason given, take default reason
        if(reason.isEmpty())
            result.toServer += m_server->getIdentity()->getPartReason();
        else
            result.toServer += reason;

        return result;
    }

    OutputFilterResult OutputFilter::parseNotice(const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.find(" "));
        QString message = parameter.mid(recipient.length()+1);

        if(parameter.isEmpty() || message.isEmpty())
        {
            result = usage(i18n("Usage: %1NOTICE <recipient> <message>").arg(commandChar));
        }
        else
        {
            result.typeString = i18n("Notice");
            result.toServer = "NOTICE " + recipient + " :" + message;
            result.output=i18n("Sending notice \"%1\" to %2.").arg(message).arg(recipient);
            result.type = Program;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseMsg(const QString &myNick, const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.find(" "));
        QString message = parameter.mid(recipient.length() + 1);

        if(message.startsWith(commandChar+"me"))
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
            result.output = QString("* %1 %2").arg(myNick).arg(message.mid(4));
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + message;
            result.output = message;
        }

        result.typeString= "-> " + recipient;
        result.type = Query;
        return result;
    }

    OutputFilterResult OutputFilter::parseSMsg(const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.find(" "));
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
        QString recipient = parameter.section(' ', 0, 0); // who is the recipient?
        QString request = parameter.section(' ', 1, 1, QString::SectionSkipEmpty);   // what is the first word of the ctcp?
        QString message = parameter.section(' ', 1, 0xffffff, QString::SectionSkipEmpty);      // what is the complete ctcp command?

        if(request.lower() == "ping") //Note that there may be locale problems with turkish with this
        {
            unsigned int time_t = QDateTime::currentDateTime().toTime_t();
            result.toServer = QString("PRIVMSG %1 :\x01PING %2\x01").arg(recipient).arg(time_t);
            result.output = i18n("Sending CTCP-%1 request to %2").arg("PING").arg(recipient);
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + message + '\x01';
            result.output = i18n("Sending CTCP-%1 request to %2").arg(message).arg(recipient);
        }

        result.typeString = i18n("CTCP");
        result.type = Program;
        return result;
    }

    void OutputFilter::parseQuery(const QString &parameter)
    {
        QStringList queryList = QStringList::split(' ', parameter);

        for(unsigned int index = 0; index < queryList.count(); index++) {
            emit openQuery(queryList[index], QString::null);
        }
    }

    OutputFilterResult OutputFilter::changeMode(const QString &parameter,char mode,char giveTake)
    {
        OutputFilterResult result;
        // TODO: Make sure this works with +l <limit> and +k <password> also!
        QString token;
        QString tmpToken;
        QStringList nickList = QStringList::split(' ', parameter);

        if(nickList.count())
        {
            // Check if the user specified a channel
            if(isAChannel(nickList[0]))
            {
                token = "MODE " + nickList[0];
                // remove the first element
                nickList.remove(nickList.begin());
            }
            // Add default destination if it is a channel
            else if(isAChannel(destination)) {
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
                    token += " " + nickList[index];
                }

                if(token != tmpToken) {
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
        if(parameter.isEmpty()) {
            emit openDccPanel();
        } else {
            QStringList parameterList = QStringList::split(' ', parameter);

            QString dccType = parameterList[0].lower();

            if(dccType=="close") {
                emit closeDccPanel();
            } else if(dccType=="send") {
                if(parameterList.count()==1) {                // DCC SEND
                    emit requestDccSend();
                } else if(parameterList.count()==2) {         // DCC SEND <nickname>
                    emit requestDccSend(parameterList[1]);
                } else if(parameterList.count()>2) {          // DCC SEND <nickname> <file> [file] ...
                    // TODO: make sure this will work:
                    //output=i18n("Usage: %1DCC SEND nickname [fi6lename] [filename] ...").arg(commandChar);
		    KURL fileURL(parameterList[2]);


		   //We could easily check if the remote file exists, but then we might
		   //end up asking for creditionals twice, so settle for only checking locally
                   if(!fileURL.isLocalFile() || QFile::exists( fileURL.path() )) {
                     emit openDccSend(parameterList[1],fileURL);
                   } else {
                      result = error(i18n("Error: File \"%1\" does not exist.").arg(parameterList[2]));
                   }
                }
                else   // Don't know how this should happen, but ...
                {
                    result = usage(i18n("Usage: %1DCC [SEND nickname filename]").arg(commandChar));
                }
            }
            // TODO: DCC Chat etc. comes here
            else if(dccType=="chat")
            {
                if(parameterList.count()==2) {
                    emit requestDccChat(parameterList[1]);
                } else {
                    result = usage(i18n("Usage: %1DCC [CHAT nickname]").arg(commandChar));
                }
            }
            else
            {
                result = error(i18n("Error: Unrecognized command DCC %1. Possible commands are SEND, CHAT, CLOSE.").arg(parameterList[0]));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::sendRequest(const QString &recipient,const QString &fileName,const QString &address,const QString &port,unsigned long size)
    {
        OutputFilterResult result;
        QFile file(fileName);
        QFileInfo info(file);

        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
                          + info.fileName().replace(" ","_")
                          + " " + address + " " + port + " " + QString::number(size) + '\x01';
        result.output = i18n("Offering \"%1\" to %2 for upload...").arg(fileName).arg(recipient);
        result.typeString = i18n("DCC");
        result.type = Program;

        return result;
    }

    // Accepting Resume Request
    OutputFilterResult OutputFilter::acceptRequest(const QString &recipient,const QString &fileName,const QString &port,int startAt)
    {
        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + " " + port
                          + " " + QString::number(startAt) + '\x01';
        result.output = i18n("Accepting DCC Resume request from \"%1\" for file \"%2\".").arg(recipient).arg(fileName);
        result.typeString = i18n("DCC");
        result.type = Program;

        return result;
    }

    OutputFilterResult OutputFilter::resumeRequest(const QString &sender,const QString &fileName,const QString &port,KIO::filesize_t startAt)
    {
        OutputFilterResult result;
        QString newFileName(fileName);
        newFileName.replace(" ", "_");
        result.toServer = "PRIVMSG " + sender + " :" + '\x01' + "DCC RESUME " + newFileName + " " + port + " "
                          + QString::number(startAt) + '\x01';
        result.output = i18n("Sending DCC Resume request to \"%1\" for file \"%2\".").arg(sender).arg(fileName);
        result.typeString = i18n("DCC");
        result.type = Program;
        return result;
    }

    OutputFilterResult OutputFilter::parseInvite(const QString &parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: INVITE <nick> [channel]"));
        }
        else
        {
            QString nick = parameter.section(' ', 0, 0);
            QString channel = parameter.section(' ', 1, 1);

            if(channel.isEmpty())
            {
                if(isAChannel(destination)) {
                    channel = destination;
                } else {
                    result = error(i18n("Error: INVITE without channel name works only from within channels."));
                }
            }

            if(!channel.isEmpty())
            {
                if(isAChannel(channel)) {
                    result.toServer = "INVITE " + nick + " " + channel;
                } else {
                    result = error(i18n("Error: %1 is not a channel.").arg(channel));
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
            result = usage(i18n("Usage: EXEC <script> [parameter list]"));
        }
        else
        {
            QStringList parameterList = QStringList::split(' ', parameter);

            if(parameterList[0].find("../") == -1)
            {
                emit launchScript(destination, parameter);
            }
            else
            {
                result = error(i18n("Error: Script name may not contain \"../\"!"));
            }
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseRaw(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty() || parameter == "open") {
            emit openRawLog(true);
        } else if(parameter == "close") {
            emit closeRawLog();
        } else {
            result = usage(i18n("Usage: RAW [OPEN | CLOSE]"));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseNotify(const QString& parameter)
    {
        OutputFilterResult result;

        QString groupName = m_server->getServerGroup();
        if(!parameter.isEmpty())
        {
            QStringList list = QStringList::split(' ', parameter);

            for(unsigned int index = 0; index < list.count(); index++)
            {
                // Try to remove current pattern
                if(!KonversationApplication::preferences.removeNotify(groupName, list[index]))
                {
                    // If remove failed, try to add it instead
                    if(!KonversationApplication::preferences.addNotify(groupName, list[index])) {
                        kdDebug() << "OutputFilter::parseNotify(): Adding failed!" << endl;
                    }
                }
            } // endfor
        }

        // show (new) notify list to user
        QString list = KonversationApplication::preferences.getNotifyStringByGroup(groupName) + " " + Konversation::Addressbook::self()->allContacts().join(" ");
        result.typeString = i18n("Notify");

        if(list.isEmpty())
            result.output = i18n("Current notify list is empty.");
        else
            result.output = i18n("Current notify list: %1").arg(list);

        result.type = Program;
        return result;
    }

    OutputFilterResult OutputFilter::parseOper(const QString& myNick,const QString& parameter)
    {
        OutputFilterResult result;
        QStringList parameterList = QStringList::split(' ', parameter);

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

            if(ret == KIO::PasswordDialog::Accepted) {
                result.toServer = "OPER " + nick + " " + password;
            }
        }
        else
        {
            result.toServer = "OPER " + parameter;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseBan(const QString& parameter)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage = true;

        if(!parameter.isEmpty())
        {
            QStringList parameterList=QStringList::split(' ',parameter);
            QString channel=QString::null;
            QString option=QString::null;
            // check for option
            bool host = (parameterList[0].lower() == "-host");
            bool domain = (parameterList[0].lower() == "-domain");
            bool uhost = (parameterList[0].lower() == "-userhost");
            bool udomain = (parameterList[0].lower() == "-userdomain");

            // remove possible option
            if(host || domain || uhost || udomain)
            {
                option = parameterList[0].mid(1);
                parameterList.pop_front();
            }

            // look for channel / ban mask
            if(parameterList.count())
            {
                // user specified channel
                if(isAChannel(parameterList[0]))
                {
                    channel = parameterList[0];
                    parameterList.pop_front();
                }
                // no channel, so assume current destination as channel
                else if(isAChannel(destination))
                    channel = destination;
                else
                {
                    // destination is no channel => error
                    result = error(i18n("BAN without channel name works only from inside a channel."));
                    // no usage information after error
                    showUsage = false;
                }
                // signal server to ban this user if all went fine
                if(!channel.isEmpty())
                {
                    emit banUsers(parameterList,channel,option);
                    // syntax was correct, so reset flag
                    showUsage = false;
                }
            }
        }

        if(showUsage) {
            result = usage(i18n("Usage: BAN [-HOST | -DOMAIN] [channel] <user|mask>"));
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
            QStringList parameterList = QStringList::split(' ', parameter);
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
                result = error(i18n("UNBAN without channel name works only from inside a channel."));
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

        if(showUsage) {
            result = usage(i18n("Usage: UNBAN [channel] pattern"));
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
            QStringList parameterList = QStringList::split(' ', parameter);

            // if nothing else said, only ignore channels and queries
            int value = Ignore::Channel | Ignore::Query;

            // user specified -all option
            if(parameterList[0].lower() == "-all")
            {
                // ignore everything
                value = Ignore::All;
                parameterList.pop_front();
            }

            // were there enough parameters?
            if(parameterList.count() >= 1)
            {
                for(unsigned int index=0;index<parameterList.count();index++)
                {
                    if(parameterList[index].contains('!') == 0) {
                        parameterList[index] += "!*";
                    }

                    KonversationApplication::preferences.addIgnore(parameterList[index] + "," + QString::number(value));
                }

                result.output = i18n("Added %1 to your ignore list.").arg(parameterList.join(", "));
                result.typeString = i18n("Ignore");
                result.type = Program;

                // all went fine, so show no error message
                showUsage = false;
            }
        }

        if(showUsage) {
            result = usage(i18n("Usage: IGNORE [ -ALL ] user list"));
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseQuote(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty()) {
            result = usage(i18n("Usage: QUOTE command list"));
        } else {
            result.toServer = parameter;
        }

        return result;
    }

    OutputFilterResult OutputFilter::parseSay(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty()) {
            result = usage(i18n("Usage: SAY text"));
        } else {
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

    void OutputFilter::setCommandChar() { commandChar=KonversationApplication::preferences.getCommandChar(); }

    // # & + and ! are *often*, but not necessarily, channel identifiers. + and ! are non-RFC, so if a server doesn't offer 005 and
    // supports + and ! channels, I think thats broken behaviour on their part - not ours.
    bool OutputFilter::isAChannel(const QString &check)
    {
        Q_ASSERT(m_server);
        return m_server? m_server->isAChannel(check) : QString("#&").contains(check.at(0)); // XXX if we ever see the assert, we need the ternary
    }

    OutputFilterResult OutputFilter::usage(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Usage");
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

    void OutputFilter::parseAaway(const QString& parameter)
    {
        emit multiServerCommand("away", parameter);
    }

    OutputFilterResult OutputFilter::parseAme(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty()) {
            result = usage(i18n("Usage: %1AME text").arg(commandChar));
        }

        emit multiServerCommand("me", parameter);
        return result;
    }

    OutputFilterResult OutputFilter::parseAmsg(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty()) {
            result = usage(i18n("Usage: %1AMSG text").arg(commandChar));
        }

        emit multiServerCommand("msg", parameter);
        return result;
    }

    void OutputFilter::parseServer(const QString& parameter)
    {
        if(parameter.isEmpty()) {
            emit reconnectServer();
        } else {
            QStringList splitted = QStringList::split(" ", parameter);
            QString password;
	    QStringList splitAddress;

            if(splitted.count() > 1) {
                password = splitted[1];
            }

	    splitAddress= QStringList::split(":", splitted[0], TRUE);
            QString port = "6667";

            if(splitAddress.count() == 2) { // IPv4 address with a port
	      port = splitAddress[1];
	      splitted[0] = splitAddress[0];
            }
	    else if(splitAddress.count() > 6) { // IPv6 address with a port
	      port = splitAddress[splitAddress.count()-1];
	      splitted[0] = splitted[0].section(':',0,5);
	    }

	    kdDebug() << "Server : " << splitted[0] << " Port : " << port << endl;

            emit connectToServer(splitted[0], port, password);
        }
    }

    OutputFilterResult OutputFilter::parsePrefs(const QString& parameter)
    {
        OutputFilterResult result;
        bool showUsage = false;

        if (parameter.isEmpty())
            showUsage = true;
        else
        {
            KConfig* config=KApplication::kApplication()->config();

            QStringList splitted = KShell::splitArgs(parameter);

            if (splitted.count() > 0)
            {
                QString group = splitted[0];
                QStringList groupList(config->groupList());
                uint i;
                if (group.lower() == "list")
                {
                    // List available groups.
                    result = usage(i18n("Available Preference Groups: ") + groupList.join("|"));
                }
                else
                {
                    // Validate group.
                    bool validGroup = false;
                    for (i = 0; i < groupList.count(); ++i)
                    {
                        if (group.lower() == groupList[i].lower())
                        {
                            validGroup = true;
                            group = groupList[i];
                            break;
                        }
                    }
                    if (validGroup && splitted.count() > 1)
                    {
                        QString option = splitted[1];
                        QMap<QString,QString> options = config->entryMap(group);
                        QValueList<QString> optionList = options.keys();
                        QValueList<QString> optionValueList = options.values();

                        if (option.lower() == "list")
                        {
                            // List available options in group.
                            QString output = i18n("Available Options in Group ") + group + ": ";

                            for (i = 0; i < optionList.count(); ++i)
                            {
                                output += optionList[i] + "(" + optionValueList[i] + ")|";
                            }

                            result = usage(output);
                        }
                        else
                        {
                            // Validate option.
                            bool validOption = false;
                            for (i = 0; i < optionList.count(); ++i)
                            {
                                if (option.lower() == optionList[i].lower())
                                {
                                    validOption = true;
                                    option = optionList[i];
                                    break;
                                }
                            }
                            if (validOption)
                            {
                                if (splitted.count() > 2)
                                {
                                    // Set the desired option.
                                    config->setGroup(group);
                                    config->writeEntry(option, splitted[2]);
                                    config->sync();
                                    // Reload preferences object.
                                    dynamic_cast<KonversationApplication*>(kapp)->readOptions();
                                }
                                // If no value given, just display current value.
                                else
                                {
                                    result = usage(group + "/" + option + " = " + options[option]);
                                }
                            } else {
                                showUsage = true;
                            }
                        }
                    } else {
                        showUsage = true;
                    }
                }
            } else {
                showUsage = true;
            }
        }

        if (showUsage) {
            result = usage(i18n("Usage: %1PREFS group option value or %2PREFS LIST to list groups or %3PREFS group LIST to list options in group.  Quote parameters if they contain spaces.").arg(commandChar, commandChar, commandChar));
        }

        return result;
    }

  void OutputFilter::parseCharset(const QString charset)
  {
    QString shortName = IRCCharsets::ambiguousNameToShortName(charset);
    if(!shortName.isEmpty())
      m_server->getIdentity()->setCodecName(shortName);
  }

}

#include "outputfilter.moc"
