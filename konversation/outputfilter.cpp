/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  outputfilter.cpp  -  description
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdebug.h>

#include "konversationapplication.h"
#include "outputfilter.h"

OutputFilter::OutputFilter()
{
}

OutputFilter::~OutputFilter()
{
}

QString& OutputFilter::parse(const QString& myNick,const QString& originalLine,const QString& name)
{
  setCommandChar();

  toServer="";
  output="";
  type="";
  destination=name;

  action=false;
  program=false;
  command=false;
  query=false;
  
  QString inputLine=originalLine;

  // replace placeholders
  inputLine.replace(QRegExp("%%"),"%\x01");  // make sure to protect double %%
  inputLine.replace(QRegExp("%G"),"\x07");   // replace %G with ASCII BEL 0x07
  inputLine.replace(QRegExp("%\x01"),"%");   // restore double %% as single %

  QString line=inputLine.lower();

  // Action?
  if(line.startsWith(commandChar+"me ") && destination!="")
  {
    toServer="PRIVMSG "+name+" :"+'\x01'+"ACTION "+inputLine.mid(4)+'\x01';
    output=inputLine.mid(4);
    action=true;
  }
  // Convert double command chars at the beginning to single ones
  else if(line.startsWith(commandChar+commandChar) && destination!="")
  {
    toServer="PRIVMSG "+name+" :"+inputLine.mid(1);
    output=inputLine.mid(1);
  }
  // Server command?
  else if(line[0]==commandChar[0])
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
    else if(line.startsWith("dcc "))     parseDcc(parameter);
    else if(line.startsWith("invite "))  parseInvite(parameter);

    else if(line=="join")                parseJoin("");
    else if(line=="part")                parsePart("");
    else if(line=="leave")               parsePart("");
    else if(line=="quit")                parseQuit("");
    else if(line=="notice")              parseNotice("");
    else if(line=="kick")                parseKick("");
    else if(line=="topic")               parseTopic("");
    else if(line=="away")                parseAway("");
    else if(line=="unaway")              parseAway("");
    else if(line=="dcc")                 parseDcc("");
    else if(line=="invite")              parseInvite("");

    // Forward unknown commands to server
    else toServer=inputLine.mid(1);
  }
  // Ordinary message to channel/query?
  else if(destination!="")
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

void OutputFilter::parseOp(QString parameter)
{
  changeMode(parameter,'o','+');
}

void OutputFilter::parseDeop(QString parameter)
{
  changeMode(parameter,'o','-');
}

void OutputFilter::parseVoice(QString parameter)
{
  changeMode(parameter,'v','+');
}

void OutputFilter::parseUnvoice(QString parameter)
{
  changeMode(parameter,'v','-');
}

void OutputFilter::parseJoin(QString channelName)
{
  if(channelName=="")
  {
    type=i18n("Usage");
    output=i18n("Usage: %1JOIN <channel> [key]").arg(commandChar);
    program=true;
  }
  else
    toServer="JOIN "+channelName;
}

void OutputFilter::parseKick(QString parameter)
{
  if(isAChannel(destination))
  {
    // get nick to kick
    QString victim=parameter.left(parameter.find(" "));
    if(victim=="")
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
      if(reason=="") reason=identity.getKickReason();
      toServer="KICK "+destination+" "+victim+" :"+reason;
    }
  }
  else
  {
    type=i18n("Error");
    output=i18n("%1KICK does only work from within channels.").arg(commandChar);
    program=true;
  }
}

void OutputFilter::parsePart(QString parameter)
{
  // No parameter, try default part message
  if(parameter=="")
  {
    // But only if we actually are in a channel
    if(isAChannel(destination)) toServer="PART "+destination+" :"+identity.getPartReason();
    else
    {
      type=i18n("Error");
      output=i18n("%1PART without parameters works only from within a channel.").arg(commandChar);
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
      if(reason=="") reason=identity.getPartReason();
      toServer="PART "+channel+" :"+reason;
    }
    // part this channel with a given reason
    else
    {
      if(isAChannel(destination)) toServer="PART "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("%1PART without channel name works only from within a channel.").arg(commandChar);
        program=true;
      }
    }
  }
}

void OutputFilter::parseTopic(QString parameter)
{
  // No parameter, try to get current topic
  if(parameter=="")
  {
    // But only if we actually are in a channel
    if(isAChannel(destination)) toServer="TOPIC "+destination;
    else
    {
      type=i18n("Error");
      output=i18n("%1TOPIC without parameters works only from within a channel.").arg(commandChar);
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
      if(topic=="") toServer="TOPIC "+channel;
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
        output=i18n("%1TOPIC without channel name works only from within a channel.").arg(commandChar);
        program=true;
      }
    }
  }
}

void OutputFilter::parseAway(QString reason)
{
  if(reason=="")
  {
    if(identity.getShowAwayMessage())
      sendToAllChannels(identity.getReturnMessage());

    emit unAway();
    toServer="AWAY";
  }
  else
  {
    if(identity.getShowAwayMessage())
    {
      QString message=identity.getAwayMessage();
      sendToAllChannels(message.replace(QRegExp("%s",false),reason));
    }

    emit away();
    toServer="AWAY :"+reason;
  }
  // remove lines in output to prevent them sent twice in sending channel
  output="";
}

void OutputFilter::parseQuit(QString reason)
{
  // if no reason given, take default reason
  if(reason=="") reason=identity.getPartReason();
  toServer="QUIT :"+reason;
}

void OutputFilter::parseNotice(QString parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  if(parameter=="" || message=="")
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

void OutputFilter::parseMsg(QString myNick,QString parameter)
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
  type=QString("-> %1").arg(recipient);
  query=true;
}

void OutputFilter::parseSMsg(QString parameter)
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

void OutputFilter::parseCtcp(QString parameter)
{
  QString recipient=parameter.section(' ',0,0);  // who is the recipient?
  QString request=parameter.section(' ',1,1);    // what is the first word of the ctcp?
  QString message=parameter.section(' ',1);      // what is the complete ctcp command?

  if(request.lower()=="ping")
  {
    toServer=QString("PRIVMSG "+recipient+" :\x01PING %1\x01").arg(QString::number(QDateTime::currentDateTime().toTime_t()));
    output=i18n("Sending CTCP-PING request to %1").arg(recipient);
  }
  else
  {
    toServer=QString("PRIVMSG "+recipient+" :"+'\x01'+message+'\x01');
    output=i18n("Sending CTCP-%1 request to %2").arg(message).arg(recipient);
  }
  type=i18n("CTCP");
  program=true;
}

void OutputFilter::parseQuery(QString parameter)
{
  QStringList queryList=QStringList::split(' ',parameter);

  for(unsigned int index=0;index<queryList.count();index++) emit openQuery(queryList[index],"");
}

void OutputFilter::changeMode(QString parameter,char mode,char giveTake)
{
  // TODO: Make sure this works with +l <limit> and +k <keyword> also!
  QStringList nickList=QStringList::split(' ',parameter);
  if(nickList.count())
  {
    // Check if the user specified a channel
    if(isAChannel(nickList[0]))
    {
      toServer="MODE "+nickList[0];
      // remove the first element
      nickList.remove(nickList.begin());
    }
    // Add default destination if it is a channel
    else if(isAChannel(destination)) toServer="MODE "+destination;
    // Only continue if there was no error
    if(toServer.length())
    {
      unsigned int modeCount=nickList.count();
      if(modeCount>3)
      {
        modeCount=3;
        output=i18n("Modes can only take a certain number of nick names at the same time."
                    "The server may truncate your mode list.");
        type=i18n("Warning");
        program=true;
      }

      QString modes;
      modes.fill(mode,modeCount);

      toServer+=QString(" ")+giveTake+modes;

      for(unsigned int index=0;index<modeCount;index++) toServer+=" "+nickList[index];
    }
  }
}

void OutputFilter::parseDcc(QString parameter)
{
  // No parameter, just open DCC panel
  if(parameter=="") emit openDccPanel();
  else
  {
    QStringList parameterList=QStringList::split(' ',parameter);

    QString dccType=parameterList[0].lower();

    if(dccType=="close") emit closeDccPanel();
    else if(dccType=="send")
    {
      if(parameterList.count()==1)                 // DCC SEND
      {
        emit requestDccSend(NULL);
      }
      else if(parameterList.count()==2)            // DCC SEND <nickname>
      {
        emit requestDccSend(parameterList[1]);
      }
      else if(parameterList.count()>2)             // DCC SEND <nickname> <file> [file] ...
      {
// TODO: make sure this will work:
//        output=i18n("Usage: %1DCC SEND nickname [filename] [filename] ...").arg(commandChar);
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
    else
    {
      type=i18n("Error");
      output=i18n("Error: Unrecognized command DCC %1.").arg(parameterList[0]);
      program=true;
    }
  }
}

void OutputFilter::sendRequest(QString recipient,QString fileName,QString address,QString port,unsigned long size)
{
  QFile file(fileName);
  QFileInfo info(file);

  toServer="PRIVMSG "+recipient+" :"+'\x01'+"DCC SEND "+info.fileName()+" "+address+" "+port+" "+QString::number(size)+'\x01';
  output=i18n("Offering \"%1\" to %2 for upload.").arg(fileName).arg(recipient);
  type=i18n("DCC");
  program=true;
}

// Accepting Resume Request
void OutputFilter::acceptRequest(QString recipient,QString fileName,QString port,int startAt)
{
  toServer="PRIVMSG "+recipient+" :"+'\x01'+"DCC ACCEPT "+fileName+" "+port+" "+QString::number(startAt)+'\x01';
  output=i18n("Accepting DCC Resume request from \"%1\" for file \"%2\".").arg(recipient).arg(fileName);
  type=i18n("DCC");
  program=true;
}

void OutputFilter::resumeRequest(QString sender,QString fileName,QString port,int startAt)
{
  toServer="PRIVMSG "+sender+" :"+'\x01'+"DCC RESUME "+fileName+" "+port+" "+QString::number(startAt)+'\x01';
  output=i18n("Sending DCC Resume request to \"%1\" for file \"%2\".").arg(sender).arg(fileName);
  type=i18n("DCC");
  program=true;
}

void OutputFilter::parseInvite(QString parameter)
{
  if(parameter=="")
  {
    type=i18n("Usage");
    output=i18n("Usage: INVITE <nick> [channel]");
    program=true;
  }
  else
  {
    QString nick=parameter.section(' ',0,0);
    QString channel=parameter.section(' ',1,1);

    if(channel=="")
    {
      if(isAChannel(destination)) channel=destination;
      else
      {
        type=i18n("Error");
        output=i18n("Error: INVITE without channel name works only from within channels.");
        program=true;
      }
    }

    if(channel!="")
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

// Accessors

// Maybe we should switch to values instead of flags
bool OutputFilter::isAction() { return action; };
bool OutputFilter::isCommand() { return command; };
bool OutputFilter::isProgram() { return program; };
bool OutputFilter::isQuery() { return query; };

void OutputFilter::setCommandChar() { commandChar=KonversationApplication::preferences.getCommandChar(); }
void OutputFilter::setIdentity(const Identity& newIdentity) { identity=newIdentity; }

QString& OutputFilter::getOutput() { return output; };
QString& OutputFilter::getServerOutput() { return toServer; };
QString& OutputFilter::getType() { return type; };

//     # & + and ! are Channel identifiers
bool OutputFilter::isAChannel(QString check)
{
  QChar initial=check.at(0);

  return (initial=='#' || initial=='&' || initial=='+' || initial=='!');
}

#include "outputfilter.moc"
