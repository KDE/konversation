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

#include <klocale.h>
#include <kdebug.h>
#include <kio/passdlg.h>
#include <kconfig.h>
#include <kdeversion.h>

#ifndef KDE_MAKE_VERSION
#define KDE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))
#endif

#ifndef KDE_IS_VERSION
#define KDE_IS_VERSION(a,b,c) ( KDE_VERSION >= KDE_MAKE_VERSION(a,b,c) )
#endif

#if KDE_IS_VERSION(3,1,94)
#include <kshell.h>
#endif

#include "outputfilter.h"
#include "konversationapplication.h"
#include "ignore.h"

#if QT_VERSION < 0x030100
#include "main.h"
#endif

OutputFilter::OutputFilter()
{
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
      // cut alias replacement from definition
      QString aliasReplace(aliasList[index].section(' ',1));
      // protect "%%"
      aliasReplace.replace(QRegExp("%%"),"%\x01");
      // replace %p placeholder with rest of line
      aliasReplace.replace(QRegExp("%p"),line.section(' ',1));
      // restore "%<1>" as "%%"
      aliasReplace.replace(QRegExp("%\x01"),"%%");
      // modify line
      line=aliasReplace;
      // return "replaced"
      return true;
    }
  } // for

  return false;
}

QString& OutputFilter::parse(const QString& myNick,const QString& originalLine,const QString& name)
{
  setCommandChar();

  toServer=QString::null;
  toServerList=QStringList::QStringList();
  output=QString::null;
  type=QString::null;
  destination=name;

  action=false;
  program=false;
  command=false;
  query=false;

  QString inputLine(originalLine);

  if(!KonversationApplication::preferences.getDisableExpansion())
  {
    // replace placeholders
    inputLine.replace(QRegExp("%%"),"%\x01");  // make sure to protect double %%
    inputLine.replace(QRegExp("%B"),"\x02");   // replace %B with bold char
    inputLine.replace(QRegExp("%C"),"\x03");   // replace %C with color char
    inputLine.replace(QRegExp("%G"),"\x07");   // replace %G with ASCII BEL 0x07
    inputLine.replace(QRegExp("%I"),"\x09");   // replace %I with italics char
    inputLine.replace(QRegExp("%O"),"\x0f");   // replace %O with reset to default char
    inputLine.replace(QRegExp("%S"),"\x13");   // replace %S with strikethru char
    //  inputLine.replace(QRegExp("%?"),"\x15");
    inputLine.replace(QRegExp("%R"),"\x16");   // replace %R with reverse char
    inputLine.replace(QRegExp("%U"),"\x1f");   // replace %U with underline char

    inputLine.replace(QRegExp("%\x01"),"%");   // restore double %% as single %
  }

  QString line=inputLine.lower();

  // Action?
  if(line.startsWith(commandChar+"me ") && !destination.isEmpty())
  {
    toServer="PRIVMSG "+name+" :"+'\x01'+"ACTION "+inputLine.mid(4)+'\x01';
    output=inputLine.mid(4);
    action=true;
  }
  // Convert double command chars at the beginning to single ones
  else if(line.startsWith(commandChar+commandChar) && !destination.isEmpty())
  {
    toServer="PRIVMSG "+name+" :"+inputLine.mid(1);
    output=inputLine.mid(1);
  }
  // Server command?
  else if(line.at(0)==commandChar.at(0))
  {
    QString parameter=inputLine.mid(inputLine.find(" ")+1);
    parameter=parameter.stripWhiteSpace();

    line=line.mid(1);

    if     (line.startsWith("join "))    parseJoin(parameter);
    else if(line.startsWith("part "))    parsePart(parameter);
    else if(line.startsWith("leave "))   parsePart(parameter);
    else if(line.startsWith("quit "))    parseQuit(parameter);
    else if(line.startsWith("notice "))  parseNotice(parameter);
    else if(line.startsWith("j "))       parseJoin(parameter);
    else if(line.startsWith("msg "))     parseMsg(myNick,parameter);
    else if(line.startsWith("smsg "))    parseSMsg(parameter);
    else if(line.startsWith("query "))   parseQuery(parameter);
    else if(line.startsWith("op "))      parseOp(parameter);
    else if(line.startsWith("deop "))    parseDeop(parameter);
    else if(line.startsWith("voice "))   parseVoice(parameter);
    else if(line.startsWith("unvoice ")) parseUnvoice(parameter);
    else if(line.startsWith("ctcp "))    parseCtcp(parameter);
    else if(line.startsWith("kick "))    parseKick(parameter);
    else if(line.startsWith("topic "))   parseTopic(parameter);
    else if(line.startsWith("away "))    parseAway(parameter);
    else if(line.startsWith("invite "))  parseInvite(parameter);
    else if(line.startsWith("exec "))    parseExec(parameter);
    else if(line.startsWith("notify "))  parseNotify(parameter);
    else if(line.startsWith("oper "))    parseOper(myNick,parameter);
    else if(line.startsWith("ban "))     parseBan(parameter);
    else if(line.startsWith("unban "))   parseUnban(parameter);
    else if(line.startsWith("ignore "))  parseIgnore(parameter);
    else if(line.startsWith("quote "))   parseQuote(parameter);
    else if(line.startsWith("say "))     parseSay(parameter);

    else if(line.startsWith("raw "))     parseRaw(parameter);
    else if(line.startsWith("dcc "))     parseDcc(parameter);
    else if(line.startsWith("konsole ")) parseKonsole();
    
    else if(line.startsWith("aaway "))   parseAaway(parameter);
    else if(line.startsWith("ame "))     parseAme(parameter);
    else if(line.startsWith("amsg "))    parseAmsg(parameter);
    
    else if(line.startsWith("server "))  parseServer(parameter);
    
    else if(line.startsWith("prefs "))   parsePrefs(parameter);

    else if(line=="join")                parseJoin(QString::null);
    else if(line=="part")                parsePart(QString::null);
    else if(line=="leave")               parsePart(QString::null);
    else if(line=="quit")                parseQuit(QString::null);
    else if(line=="notice")              parseNotice(QString::null);
    else if(line=="kick")                parseKick(QString::null);
    else if(line=="topic")               parseTopic(QString::null);
    else if(line=="away")                parseAway(QString::null);
    else if(line=="unaway")              parseAway(QString::null);
    else if(line=="invite")              parseInvite(QString::null);
    else if(line=="exec")                parseExec(QString::null);
    else if(line=="notify")              parseNotify(QString::null);
    else if(line=="oper")                parseOper(myNick,QString::null);
    else if(line=="ban")                 parseBan(QString::null);
    else if(line=="unban")               parseUnban(QString::null);
    else if(line=="ignore")              parseIgnore(QString::null);
    else if(line=="quote")               parseQuote(QString::null);
    else if(line=="say")                 parseSay(QString::null);

    else if(line=="dcc")                 parseDcc(QString::null);
    else if(line=="raw")                 parseRaw(QString::null);
    else if(line=="konsole")             parseKonsole();

    else if(line=="aaway")               parseAaway(QString::null);
    else if(line=="ame")                 parseAme(QString::null);
    else if(line=="amsg")                parseAmsg(QString::null);
    
    else if(line == "server")            parseServer(QString::null);
    
    else if(line == "prefs")             parsePrefs(QString::null);
    
    // Forward unknown commands to server
    else toServer=inputLine.mid(1);
  }
  // Ordinary message to channel/query?
  else if(!destination.isEmpty())
  {
    toServer="PRIVMSG "+destination+" :"+inputLine;
    output=inputLine;
  }
  // Eveything else goes to the server unchanged
  else
  {
    toServer=inputLine;
    output=inputLine;
    type=i18n("Raw");
    program=true;
  }

  return output;
}

void OutputFilter::parseOp(const QString &parameter)
{
  changeMode(parameter,'o','+');
}

void OutputFilter::parseDeop(const QString &parameter)
{
  changeMode(parameter,'o','-');
}

void OutputFilter::parseVoice(const QString &parameter)
{
  changeMode(parameter,'v','+');
}

void OutputFilter::parseUnvoice(const QString &parameter)
{
  changeMode(parameter,'v','-');
}

void OutputFilter::parseJoin(const QString &channelName)
{
  if(channelName.isEmpty())
  {
    type=i18n("Usage");
    output=i18n("Usage: %1JOIN <channel> [password]").arg(commandChar);
    program=true;
  }
  else
    toServer="JOIN " + channelName;
}

void OutputFilter::parseKick(const QString &parameter)
{
  if(isAChannel(destination))
  {
    // get nick to kick
    QString victim=parameter.left(parameter.find(" "));
    if(victim.isEmpty())
    {
      type=i18n("Usage");
      output=i18n("Usage: %1KICK <nick> [reason]").arg(commandChar);
      program=true;
    }
    else
    {
      // get kick reason (if any)
      QString reason=parameter.mid(victim.length()+1);
      // if no reason given, take default reason
      if(reason.isEmpty()) reason=identity.getKickReason();
      toServer="KICK "+destination+" "+victim+" :"+reason;
    }
  }
  else
  {
    type=i18n("Error");
    output=i18n("%1KICK only works from within channels.").arg(commandChar);
    program=true;
  }
}

void OutputFilter::parsePart(const QString &parameter)
{
  // No parameter, try default part message
  if(parameter.isEmpty())
  {
    // But only if we actually are in a channel
    if(isAChannel(destination)) toServer="PART "+destination+" :"+identity.getPartReason();
    else
    {
      type=i18n("Error");
      output=i18n("%1PART without parameters only works from within a channel or a query.").arg(commandChar);
      program=true;
    }
  }
  else
  {
    // part a given channel
    if(isAChannel(parameter))
    {
      // get channel name
      QString channel=parameter.left(parameter.find(" "));
      // get part reason (if any)
      QString reason=parameter.mid(channel.length()+1);
      // if no reason given, take default reason
      if(reason.isEmpty()) reason=identity.getPartReason();
      toServer="PART "+channel+" :"+reason;
    }
    // part this channel with a given reason
    else
    {
      if(isAChannel(destination)) toServer="PART "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("%1PART without channel name only works from within a channel.").arg(commandChar);
        program=true;
      }
    }
  }
}

void OutputFilter::parseTopic(const QString &parameter)
{
  // No parameter, try to get current topic
  if(parameter.isEmpty())
  {
    // But only if we actually are in a channel
    if(isAChannel(destination)) toServer="TOPIC "+destination;
    else
    {
      type=i18n("Error");
      output=i18n("%1TOPIC without parameters only works from within a channel.").arg(commandChar);
      program=true;
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
      if(topic.isEmpty()) toServer="TOPIC "+channel;
      // otherwise set topic there
      else toServer="TOPIC "+channel+" :"+topic;
    }
    // set this channel's topic
    else
    {
      if(isAChannel(destination)) toServer="TOPIC "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("%1TOPIC without channel name only works from within a channel.").arg(commandChar);
        program=true;
      }
    }
  }
}

void OutputFilter::parseAway(const QString &reason)
{
  if(reason.isEmpty())
  {
    toServer="AWAY";
  }
  else
  {
    if(identity.getShowAwayMessage())
    {
      QString message=identity.getAwayMessage();
      emit sendToAllChannels(message.replace(QRegExp("%s",false),reason));
    }

    toServer="AWAY :"+reason;
  }
  // remove lines in output to prevent them sent twice in sending channel
  output=QString::null;
  toServerList.clear();
}

void OutputFilter::parseQuit(const QString &reason)
{
  toServer = "QUIT :";
  // if no reason given, take default reason
  if(reason.isEmpty())
    toServer+=identity.getPartReason();
  else
    toServer+=reason;
}

void OutputFilter::parseNotice(const QString &parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  if(parameter.isEmpty() || message.isEmpty())
  {
    type=i18n("Usage");
    output=i18n("Usage: %1NOTICE <recipient> <message>").arg(commandChar);
    program=true;
  }
  else
  {
    type=i18n("Notice");
    toServer="NOTICE "+recipient+" :"+message;
    output=i18n("Sending notice \"%1\" to %2.").arg(message).arg(recipient);
    program=true;
  }
}

void OutputFilter::parseMsg(const QString &myNick, const QString &parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  if(message.startsWith(commandChar+"me"))
  {
    toServer="PRIVMSG "+recipient+" :"+'\x01'+"ACTION "+message.mid(4)+'\x01';
    output=QString("* %1 %2").arg(myNick).arg(message.mid(4));
  }
  else
  {
    toServer="PRIVMSG "+recipient+" :"+message;
    output=message;
  }
  type="-> "+recipient;
  query=true;
}

void OutputFilter::parseSMsg(const QString &parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  if(message.startsWith(commandChar+"me"))
  {
    toServer="PRIVMSG "+recipient+" :"+'\x01'+"ACTION "+message.mid(4)+'\x01';
  }
  else
  {
    toServer="PRIVMSG "+recipient+" :"+message;
  }
}

void OutputFilter::parseCtcp(const QString &parameter)
{
  QString recipient=parameter.section(' ',0,0);  // who is the recipient?
  QString request=parameter.section(' ',1,1);    // what is the first word of the ctcp?
  QString message=parameter.section(' ',1);      // what is the complete ctcp command?

  if(request.lower()=="ping")
  {
#if QT_VERSION < 0x030100
    unsigned int time_t=toTime_t(QDateTime::currentDateTime());
#else
    unsigned int time_t=QDateTime::currentDateTime().toTime_t();
#endif
    toServer=QString("PRIVMSG %1 :\x01PING %2\x01").arg(recipient).arg(time_t);
    output=i18n("Sending CTCP-%1 request to %2").arg("PING").arg(recipient);
  }
  else
  {
    toServer=QString("PRIVMSG "+recipient+" :"+'\x01'+message+'\x01');
    output=i18n("Sending CTCP-%1 request to %2").arg(message).arg(recipient);
  }
  type=i18n("CTCP");
  program=true;
}

void OutputFilter::parseQuery(const QString &parameter)
{
  QStringList queryList=QStringList::split(' ',parameter);
  for(unsigned int index=0;index<queryList.count();index++) emit openQuery(queryList[index],QString::null);
}

void OutputFilter::changeMode(const QString &parameter,char mode,char giveTake)
{
  // TODO: Make sure this works with +l <limit> and +k <password> also!
  QString token=QString::QString();
  QString tmpToken=QString::null;
  QStringList nickList=QStringList::split(' ',parameter);
  if(nickList.count())
  {
    // Check if the user specified a channel
    if(isAChannel(nickList[0]))
    {
      token="MODE "+nickList[0];
      // remove the first element
      nickList.remove(nickList.begin());
    }
    // Add default destination if it is a channel
    else if(isAChannel(destination)) token="MODE "+destination;
    // Only continue if there was no error
    if(token.length())
    {
      unsigned int modeCount=nickList.count();
/*      if(modeCount>3)
      {
        modeCount=3;
        output=i18n("Modes can only take a certain number of nick names at the same time."
                    "The server may truncate your mode list.");
        type=i18n("Warning");
        program=true;
      } */

      QString modes;
      modes.fill(mode,modeCount);

      token+=QString(" ")+giveTake+modes;
      tmpToken=token;

      for(unsigned int index=0;index<modeCount;index++)
      {
        if ((index % 3) == 0)
        {
          toServerList.append(token);
          token=tmpToken;
        }
        token+=" "+nickList[index];
      }
      if (token!=tmpToken) toServerList.append(token);
    }
  }
}

void OutputFilter::parseDcc(const QString &parameter)
{
  // No parameter, just open DCC panel
  if(parameter.isEmpty()) emit openDccPanel();
  else
  {
    QStringList parameterList=QStringList::split(' ',parameter);

    QString dccType=parameterList[0].lower();

    if(dccType=="close") emit closeDccPanel();
    else if(dccType=="send")
    {
      if(parameterList.count()==1)                 // DCC SEND
        emit requestDccSend();
      else if(parameterList.count()==2)            // DCC SEND <nickname>
        emit requestDccSend(parameterList[1]);
      else if(parameterList.count()>2)             // DCC SEND <nickname> <file> [file] ...
      {
// TODO: make sure this will work:
//        output=i18n("Usage: %1DCC SEND nickname [fi6lename] [filename] ...").arg(commandChar);
        QFile file(parameterList[2]);
        if(file.exists())
          emit openDccSend(parameterList[1],parameterList[2]);
        else
        {
          type=i18n("Error");
          output=i18n("Error: File \"%1\" does not exist.").arg(parameterList[2]);
          program=true;
        }
      }
      else   // Don't know how this should happen, but ...
      {
        type=i18n("Usage");
        output=i18n("Usage: %1DCC [SEND nickname filename]").arg(commandChar);
        program=true;
      }
    }
    // TODO: DCC Chat etc. comes here
    else if(dccType=="chat")
    {
      if(parameterList.count()==2)
        emit requestDccChat(parameterList[1]);
      else
      {
        type=i18n("Usage");
        output=i18n("Usage: %1DCC [CHAT nickname]").arg(commandChar);
        program=true;
      }
    }
    else
    {
      type=i18n("Error");
      output=i18n("Error: Unrecognized command DCC %1. Possible commands are SEND, CHAT, CLOSE.").arg(parameterList[0]);
      program=true;
    }
  }
}

void OutputFilter::sendRequest(const QString &recipient,const QString &fileName,const QString &address,const QString &port,unsigned long size)
{
  QFile file(fileName);
  QFileInfo info(file);

  toServer="PRIVMSG "+recipient+" :"+'\x01'+"DCC SEND "+info.fileName().replace(QRegExp(" "),"_")+" "+address+" "+port+" "+QString::number(size)+'\x01';
  output=i18n("Offering \"%1\" to %2 for upload.").arg(fileName).arg(recipient);
  type=i18n("DCC");
  program=true;
}

// Accepting Resume Request
void OutputFilter::acceptRequest(const QString &recipient,const QString &fileName,const QString &port,int startAt)
{
  toServer="PRIVMSG "+recipient+" :"+'\x01'+"DCC ACCEPT "+fileName+" "+port+" "+QString::number(startAt)+'\x01';
  output=i18n("Accepting DCC Resume request from \"%1\" for file \"%2\".").arg(recipient).arg(fileName);
  type=i18n("DCC");
  program=true;
}

void OutputFilter::resumeRequest(const QString &sender,const QString &fileName,const QString &port,int startAt)
{
  QString newFileName(fileName);
  newFileName.replace(QRegExp(" "),"_");
  toServer="PRIVMSG "+sender+" :"+'\x01'+"DCC RESUME "+newFileName+" "+port+" "+QString::number(startAt)+'\x01';
  output=i18n("Sending DCC Resume request to \"%1\" for file \"%2\".").arg(sender).arg(fileName);
  type=i18n("DCC");
  program=true;
}

void OutputFilter::parseInvite(const QString &parameter)
{
  if(parameter.isEmpty())
  {
    type=i18n("Usage");
    output=i18n("Usage: INVITE <nick> [channel]");
    program=true;
  }
  else
  {
    QString nick=parameter.section(' ',0,0);
    QString channel=parameter.section(' ',1,1);

    if(channel.isEmpty())
    {
      if(isAChannel(destination)) channel=destination;
      else
      {
        type=i18n("Error");
        output=i18n("Error: INVITE without channel name works only from within channels.");
        program=true;
      }
    }

    if(!channel.isEmpty())
    {
      if(isAChannel(channel)) toServer="INVITE "+nick+" "+channel;
      else
      {
        type=i18n("Error");
        output=i18n("Error: %1 is not a channel.").arg(channel);
        program=true;
      }
    }
  }
}

void OutputFilter::parseExec(const QString& parameter)
{
  if(parameter.isEmpty())
  {
    type=i18n("Usage");
    output=i18n("Usage: EXEC <script> [parameter list]");
    program=true;
  }
  else
  {
    QStringList parameterList=QStringList::split(' ',parameter);
    if(parameterList[0].find("../")==-1) emit launchScript(parameter);
    else
    {
      type=i18n("Error");
      output=i18n("Error: Script name may not contain \"../\"!");
      program=true;
    }
  }
}

void OutputFilter::parseRaw(const QString& parameter)
{
  if(parameter.isEmpty() || parameter=="open")
    emit openRawLog(true);
  else if(parameter=="close")
    emit closeRawLog();
  else
  {
    type=i18n("Usage");
    output=i18n("Usage: RAW [OPEN | CLOSE]");
    program=true;
  }
}

void OutputFilter::parseNotify(const QString& parameter)
{
  if(!parameter.isEmpty())
  {
    QStringList list=QStringList::split(' ',parameter);

    for(unsigned int index=0;index<list.count();index++)
    {
      // Try to remove current pattern
      if(!KonversationApplication::preferences.removeNotify(list[index]))
      {
        // If remove failed, try to add it instead
        if(!KonversationApplication::preferences.addNotify(list[index]))
          kdDebug() << "OutputFilter::parseNotify(): Adding failed!" << endl;
      }
    } // endfor
  }

  // show (new) notify list to user
  QString list=KonversationApplication::preferences.getNotifyString();
  type=i18n("Notify");

  if(list.isEmpty())
    output=i18n("Current notify list is empty.");
  else
    output=i18n("Current notify list: %1").arg(list);

  program=true;
}

void OutputFilter::parseOper(const QString& myNick,const QString& parameter)
{
  QStringList parameterList=QStringList::split(' ',parameter);

  if(parameter.isEmpty() || parameterList.count()==1)
  {
    QString nick((parameterList.count()==1) ? parameterList[0] : myNick);
    QString password;
    bool keep=false;

    int result=KIO::PasswordDialog::getNameAndPassword
                                    (
                                      nick,
                                      password,
                                      &keep,
                                      i18n("Enter user name and password for IRC operator privileges:"),
                                      false,
                                      i18n("IRC Operator Password")
                                    );

    if(result==KIO::PasswordDialog::Accepted) toServer="OPER "+nick+" "+password;
  }
  else
  {
    toServer="OPER "+parameter;
  }
}

void OutputFilter::parseBan(const QString& parameter)
{
  // assume incorrect syntax first
  bool showUsage=true;

  if(!parameter.isEmpty())
  {
    QStringList parameterList=QStringList::split(' ',parameter);
    QString channel=QString::null;
    QString option=QString::null;
    // check for option
    bool host=(parameterList[0].lower()=="-host");
    bool domain=(parameterList[0].lower()=="-domain");
    bool uhost=(parameterList[0].lower()=="-userhost");
    bool udomain=(parameterList[0].lower()=="-userdomain");

    // remove possible option
    if(host || domain || uhost || udomain)
    {
      option=parameterList[0].mid(1);
      parameterList.pop_front();
    }

    // look for channel / ban mask
    if(parameterList.count())
    {
      // user specified channel
      if(isAChannel(parameterList[0]))
      {
        channel=parameterList[0];
        parameterList.pop_front();
      }
      // no channel, so assume current destination as channel
      else if(isAChannel(destination))
        channel=destination;
      else
      {
        // destination is no channel => error
        error(i18n("BAN without channel name works only from inside a channel."));
        // no usage information after error
        showUsage=false;
      }
      // signal server to ban this user if all went fine
      if(!channel.isEmpty())
      {
        emit banUsers(parameterList,channel,option);
        // syntax was correct, so reset flag
        showUsage=false;
      }
    }
  }

  if(showUsage) usage(i18n("Usage: BAN [-HOST | -DOMAIN] [channel] <user|mask>"));
}

// finally set the ban
void OutputFilter::execBan(const QString& mask,const QString& channel)
{
  toServer="MODE "+channel+" +b "+mask;
}

void OutputFilter::parseUnban(const QString& parameter)
{
  // assume incorrect syntax first
  bool showUsage=true;

  if(!parameter.isEmpty())
  {
    QStringList parameterList=QStringList::split(' ',parameter);
    QString channel=QString::null;
    QString mask=QString::null;

    // if the user specified a channel
    if(isAChannel(parameterList[0]))
    {
      // get channel
      channel=parameterList[0];
      // remove channel from parameter list
      parameterList.pop_front();
    }
    // otherwise the current destination must be a channel
    else if(isAChannel(destination))
      channel=destination;
    else
    {
      // destination is no channel => error
      error(i18n("UNBAN without channel name works only from inside a channel."));
      // no usage information after error
      showUsage=false;
    }
    // if all went good, signal server to unban this mask
    if(!channel.isEmpty())
    {
      emit unbanUsers(parameterList[0],channel);
      // syntax was correct, so reset flag
      showUsage=false;
    }
  }

  if(showUsage) usage(i18n("Usage: UNBAN [channel] pattern"));
}

void OutputFilter::execUnban(const QString& mask,const QString& channel)
{
  toServer="MODE "+channel+" -b "+mask;
}

void OutputFilter::parseIgnore(const QString& parameter)
{
  // assume incorrect syntax first
  bool showUsage=true;

  // did the user give parameters at all?
  if(!parameter.isEmpty())
  {
    QStringList parameterList=QStringList::split(' ',parameter);

    // if nothing else said, only ignore channels and queries
    int value=Ignore::Channel | Ignore::Query;

    // user specified -all option
    if(parameterList[0].lower()=="-all")
    {
      // ignore everything
      value=Ignore::All;
      parameterList.pop_front();
    }

    // were there enough parameters?
    if(parameterList.count()>=1)
    {
      for(unsigned int index=0;index<parameterList.count();index++)
      {
        if(parameterList[index].contains('!')==0) parameterList[index] += "!*";
        KonversationApplication::preferences.addIgnore(parameterList[index]+","+QString::number(value));
      }

      output=i18n("Added %1 to your ignore list.").arg(parameterList.join(", "));
      type=i18n("Ignore");
      program=true;

      // all went fine, so show no error message
      showUsage=false;
    }
  }

  if(showUsage) usage(i18n("Usage: IGNORE [ -ALL ] user list"));
}

void OutputFilter::parseQuote(const QString& parameter)
{
  if(parameter.isEmpty())
    usage(i18n("Usage: QUOTE command list"));
  else
    toServer=parameter;
}

void OutputFilter::parseSay(const QString& parameter)
{
  if(parameter.isEmpty())
    usage(i18n("Usage: SAY text"));
  else
  {
    toServer="PRIVMSG "+destination+" :"+parameter;
    output=parameter;
  }
}

void OutputFilter::parseKonsole()
{
  emit openKonsolePanel();
}

// Accessors

// Maybe we should switch to values instead of flags
bool OutputFilter::isAction() { return action; }
bool OutputFilter::isCommand() { return command; }
bool OutputFilter::isProgram() { return program; }
bool OutputFilter::isQuery() { return query; }

void OutputFilter::setCommandChar() { commandChar=KonversationApplication::preferences.getCommandChar(); }
void OutputFilter::setIdentity(const Identity *newIdentity)
{
  identity=*newIdentity;
  // TODO: move this into copy constructor! THis does not work yet!
  identity.setNicknameList(newIdentity->getNicknameList());
}

QString& OutputFilter::getOutput() { return output; }
QString& OutputFilter::getServerOutput() { return toServer; }

QStringList& OutputFilter::getServerOutputList()
{
  if (!toServer.isEmpty()) toServerList.append(toServer);

  return toServerList;
}

QString& OutputFilter::getType() { return type; }

//     # & + and ! are Channel identifiers
bool OutputFilter::isAChannel(const QString &check)
{
  QChar initial=check.at(0);

  return (initial=='#' || initial=='&' || initial=='+' || initial=='!');
}

void OutputFilter::usage(const QString& string)
{
  type=i18n("Usage");
  output=string;
  program=true;
}

void OutputFilter::error(const QString& string)
{
  type=i18n("Error");
  output=string;
  program=true;
}

void OutputFilter::parseAaway(const QString& parameter)
{
  emit multiServerCommand("away", parameter);
  output=QString::null;
}

void OutputFilter::parseAme(const QString& parameter)
{
  if(parameter.isEmpty()) {
    type=i18n("Usage");
    output=i18n("Usage: %1AME text").arg(commandChar);
    program=true;
    return;
  }
  
  emit multiServerCommand("me", parameter);
  output=QString::null;
}

void OutputFilter::parseAmsg(const QString& parameter)
{
  if(parameter.isEmpty()) {
    type=i18n("Usage");
    output=i18n("Usage: %1AMSG text").arg(commandChar);
    program=true;
    return;
  }
  
  emit multiServerCommand("msg", parameter);
  output=QString::null;
}

void OutputFilter::parseServer(const QString& parameter)
{
  if(parameter.isEmpty()) {
    emit reconnectServer();
  } else {
    QStringList splitted = QStringList::split(" ", parameter);
    QString password;
    
    if(splitted.count() > 1) {
      password = splitted[1];
    }
    
    splitted = QStringList::split(":", splitted[0]);
    QString port = "6667";
    
    if(splitted.count() > 1) {
      port = splitted[1];
    }

    emit connectToServer(splitted[0], port, password);
  }
  
  output=QString::null;
}

void OutputFilter::parsePrefs(const QString& parameter)
{
  bool showUsage = false;
  if (parameter.isEmpty())
    showUsage = true;
  else
  {
    KConfig* config=KApplication::kApplication()->config();
  
#if KDE_IS_VERSION(3,1,94)
    QStringList splitted = KShell::splitArgs(parameter);
#else
    QStringList splitted = QStringList::split(' ',parameter);
#endif
    if (splitted.count() > 0)
    {
      QString group = splitted[0];
      QStringList groupList(config->groupList());
      uint i;
      if (group.lower() == "list")
      {
        // List available groups.
        usage(i18n("Available Preference Groups: ") + groupList.join("|"));
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
        if (validGroup and splitted.count() > 1)
        {
          QString option = splitted[1];
          QMap<QString,QString> options = config->entryMap(group);
          QValueList<QString> optionList = options.keys();
          QValueList<QString> optionValueList = options.values();
          if (option.lower() == "list")
          {
            // List available options in group.
            output=i18n("Available Options in Group ") + group + ": ";
            for (i = 0; i < optionList.count(); ++i)
            {
              output = output + optionList[i] + "(" + optionValueList[i] + ")|";
            }
            usage(output);
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
              else usage(group + "/" + option + " = " + options[option]);
            } else showUsage = true;
          }
        } else showUsage = true;
      }
    } else showUsage = true;
  }
  if (showUsage)
    usage(i18n("Usage: %1PREFS group option value or %2PREFS LIST to list groups or %3PREFS group LIST to list options in group.  Quote parameters if they contain spaces.").arg(commandChar, commandChar, commandChar));
}

#include "outputfilter.moc"
