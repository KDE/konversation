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

void OutputFilter::resumeRequest(QString sender,QString fileName,QString port,int startAt)
{
  toServer="PRIVMSG "+sender+" :"+'\x01'+"DCC RESUME "+fileName+" "+port+" "+QString::number(startAt)+'\x01';
  output="Sending DCC Resume request for file \""+fileName+"\"";
  type=i18n("Resume");
}

QString& OutputFilter::parse(const QString& inputLine,const QString& name)
{
  QString line=inputLine.lower();

  toServer="";
  output="";
  type="";
  destination=name;

  action=false;
  server=false;
  command=false;
  /* Action? */
  if(line.startsWith("/me ") && destination!="")
  {
    toServer="PRIVMSG "+name+" :"+'\x01'+"ACTION "+inputLine.mid(4)+'\x01';
    output=inputLine.mid(4);
    action=true;
  }
  /* Convert double slashes at the beginning to single ones */
  else if(line.startsWith("//") && destination!="")
  {
    toServer="PRIVMSG "+name+" :"+inputLine.mid(1);
    output=inputLine.mid(1);
  }
  /* Server command? */
  else if(line[0]=='/')
  {
    QString parameter=inputLine.mid(inputLine.find(" ")+1);
    parameter=parameter.stripWhiteSpace();

    if     (line.startsWith("/join "))    parseJoin(parameter);
    else if(line.startsWith("/part "))    parsePart(parameter);
    else if(line.startsWith("/leave "))   parsePart(parameter);
    else if(line.startsWith("/quit "))    parseQuit(parameter);
    else if(line.startsWith("/notice "))  parseNotice(parameter);
    else if(line.startsWith("/j "))       parseJoin(parameter);
    else if(line.startsWith("/msg "))     parseMsg(parameter);
    else if(line.startsWith("/query "))   parseQuery(parameter);
    else if(line.startsWith("/op "))      parseOp(parameter);
    else if(line.startsWith("/deop "))    parseDeop(parameter);
    else if(line.startsWith("/voice "))   parseVoice(parameter);
    else if(line.startsWith("/unvoice ")) parseUnvoice(parameter);
    else if(line.startsWith("/ctcp "))    parseCtcp(parameter);
    else if(line.startsWith("/kick "))    parseKick(parameter);
    else if(line.startsWith("/topic "))   parseTopic(parameter);
    else if(line.startsWith("/away "))    parseAway(parameter);

    else if(line=="/join")                parseJoin("");
    else if(line=="/part")                parsePart("");
    else if(line=="/leave")               parsePart("");
    else if(line=="/quit")                parseQuit("");
    else if(line=="/notice")              parseNotice("");
    else if(line=="/kick")                parseKick("");
    else if(line=="/topic")               parseTopic("");
    else if(line=="/away")                parseAway("");

    /* Forward unknown commands to server */
    else toServer=inputLine.mid(1);
  }
  /* Ordinary message to channel/query? */
  else if(destination!="")
  {
    toServer="PRIVMSG "+destination+" :"+inputLine;
    output=inputLine;
  }
  /* Eveything else goes to the server unchanged */
  else
  {
    toServer=inputLine;
    output=inputLine;
    type=i18n("Raw");
    command=true;
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
    output=i18n("Usage: /JOIN <channel>");
    command=true;
  }
  else
    toServer="JOIN "+channelName;
}

void OutputFilter::parseKick(QString parameter)
{
  if(isAChannel(destination))
  {
    /* get nick to kick */
    QString victim=parameter.left(parameter.find(" "));
    /* get kick reason (if any) */
    QString reason=parameter.mid(victim.length()+1);
    /* if no reason given, take default reason */
    if(reason=="") reason=KonversationApplication::preferences.getKickReason();
    toServer="KICK "+destination+" "+victim+" :"+reason;
  }
  else
  {
    type=i18n("Error");
    output=i18n("/KICK does only work from within channels.");
    command=true;
  }
}

void OutputFilter::parsePart(QString parameter)
{
  /* No parameter, try default part message */
  if(parameter=="")
  {
    /* But only if we actually are in a channel */
    if(isAChannel(destination)) toServer="PART "+destination+" :"+KonversationApplication::preferences.getPartReason();
    else
    {
      type=i18n("Error");
      output=i18n("/PART without parameters works only from within a channel.");
      command=true;
    }
  }
  else
  {
    /* part a given channel */
    if(isAChannel(parameter))
    {
      /* get channel name */
      QString channel=parameter.left(parameter.find(" "));
      /* get part reason (if any) */
      QString reason=parameter.mid(channel.length()+1);
      /* if no reason given, take default reason */
      if(reason=="") reason=KonversationApplication::preferences.getPartReason();
      toServer="PART "+channel+" :"+reason;
    }
    /* part this channel with a given reason */
    else
    {
      if(isAChannel(destination)) toServer="PART "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("/PART without channel name works only from within a channel.");
        command=true;
      }
    }
  }
}

void OutputFilter::parseTopic(QString parameter)
{
  /* No parameter, try to get current topic */
  if(parameter=="")
  {
    /* But only if we actually are in a channel */
    if(isAChannel(destination)) toServer="TOPIC "+destination;
    else
    {
      type=i18n("Error");
      output=i18n("/TOPIC without parameters works only from within a channel.");
      command=true;
    }
  }
  else
  {
    /* retrieve or set topic of a given channel */
    if(isAChannel(parameter))
    {
      /* get channel name */
      QString channel=parameter.left(parameter.find(" "));
      /* get topic (if any) */
      QString topic=parameter.mid(channel.length()+1);
      /* if no topic given, retrieve topic */
      if(topic=="") toServer="TOPIC "+channel;
      /* otherwise set topic there */
      else toServer="TOPIC "+channel+" :"+topic;
    }
    /* set this channel's topic */
    else
    {
      if(isAChannel(destination)) toServer="TOPIC "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("/TOPIC without channel name works only from within a channel.");
        command=true;
      }
    }
  }
}

void OutputFilter::parseAway(QString reason)
{
  toServer="AWAY :"+reason;
}

void OutputFilter::parseQuit(QString reason)
{
  /* if no reason given, take default reason */
  if(reason=="") reason=KonversationApplication::preferences.getPartReason();
  toServer="QUIT :"+reason;
}

void OutputFilter::parseNotice(QString parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  if(parameter=="" || message=="")
  {
    type=i18n("Usage");
    output=i18n("Usage: /NOTICE <recipient> <message>");
    command=true;
  }
  else
  {
    toServer="NOTICE "+recipient+" :"+message;
    output=i18n("Sending notice \"%1\" to %2.").arg(message).arg(recipient);
    command=true;
  }
}

void OutputFilter::parseMsg(QString parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  if(message.startsWith("/me")) toServer="PRIVMSG "+recipient+" :"+'\x01'+"ACTION "+message.mid(4)+'\x01';
  else toServer="PRIVMSG "+recipient+" :"+message;
}

void OutputFilter::parseCtcp(QString parameter)
{
  QString recipient=parameter.left(parameter.find(" "));
  QString message=parameter.mid(recipient.length()+1);

  toServer=QString("PRIVMSG "+recipient+" :"+'\x01'+message+'\x01');
  
  output=i18n("Sending CTCP-%1 request to %2").arg(message).arg(recipient);
  type=i18n("CTCP");
  command=true;
}

void OutputFilter::parseQuery(QString parameter)
{
  QStringList queryList=QStringList::split(' ',parameter);

  for(unsigned int index=0;index<queryList.count();index++) emit openQuery(queryList[index],"");
}

void OutputFilter::changeMode(QString parameter,char mode,char giveTake)
{
  /* TODO: Make sure this works with +l <limit> and +k <keyword> also! */
  QStringList nickList=QStringList::split(' ',parameter);
  if(nickList.count())
  {
    /* Check if the user specified a channel */
    if(isAChannel(nickList[0]))
    {
      toServer="MODE "+nickList[0];
      /* remove the first element */
      nickList.remove(nickList.begin());
    }
    /* Add default destination if it is a channel*/
    else if(isAChannel(destination)) toServer="MODE "+destination;
    /* Only continue if there was no error */
    if(toServer.length())
    {
      unsigned int modeCount=nickList.count();
      if(modeCount>3)
      {
        modeCount=3;
        output=i18n("Modes can only take a certain number of nick names at the same time."
                    "The server may truncate your mode list.");
        type=i18n("Warning");
        /* TODO: Issue a warning here */
      }

      QString modes;
      modes.fill(mode,modeCount);

      toServer+=QString(" ")+giveTake+modes;

      for(unsigned int index=0;index<modeCount;index++) toServer+=" "+nickList[index];
    }
  }
}

/* Accessors */

bool OutputFilter::isAction() { return action; };
bool OutputFilter::isCommand() { return command; };

QString& OutputFilter::getOutput() { return output; };
QString& OutputFilter::getServerOutput() { return toServer; };
QString& OutputFilter::getType() { return type; };

/* # & + and ! are Channel identifiers */
bool OutputFilter::isAChannel(QString check)
{
  QChar initial=check.at(0);

  return (initial=='#' || initial=='&' || initial=='+' || initial=='!');
}
