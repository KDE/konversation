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
*/

#include <qstringlist.h>

#include <klocale.h>

#include "konversationapplication.h"
#include "outputfilter.h"

OutputFilter::OutputFilter()
{
}

OutputFilter::~OutputFilter()
{
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

    else if(line=="/join")                parseJoin("");
    else if(line=="/part")                parsePart("");
    else if(line=="/leave")               parsePart("");
    else if(line=="/quit")                parseQuit("");
    else if(line=="/kick")                parseKick("");
    else if(line=="/topic")               parseTopic("");

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
    type="Raw";
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
    output=i18n("/JOIN <channel>");
  }
  else
    toServer="JOIN "+channelName;
}

void OutputFilter::parseKick(QString parameter)
{
  if(destination.startsWith("#"))
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
  }
}

void OutputFilter::parsePart(QString parameter)
{
  /* No parameter, try default part message */
  if(parameter=="")
  {
    /* But only if we actually are in a channel */
    if(destination.startsWith("#")) toServer="PART "+destination+" :"+KonversationApplication::preferences.getPartReason();
    else
    {
      type=i18n("Error");
      output=i18n("/PART without parameters works only from within a channel.");
    }
  }
  else
  {
    /* part a given channel */
    if(parameter.startsWith("#"))
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
      if(destination.startsWith("#")) toServer="PART "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("/PART without channel name works only from within a channel.");
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
    if(destination.startsWith("#"))
      toServer="TOPIC "+destination;
    else
    {
      type=i18n("Error");
      output=i18n("/TOPIC without parameters works only from within a channel.");
    }
  }
  else
  {
    /* retrieve or set topic of a given channel */
    if(parameter.startsWith("#"))
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
      if(destination.startsWith("#")) toServer="TOPIC "+destination+" :"+parameter;
      else
      {
        type=i18n("Error");
        output=i18n("/TOPIC without channel name works only from within a channel.");
      }
    }
  }
}

void OutputFilter::parseQuit(QString reason)
{
  /* if no reason given, take default reason */
  if(reason=="") reason=KonversationApplication::preferences.getPartReason();
  toServer="QUIT :"+reason;
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
  type="CTCP";
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
    if(nickList[0].startsWith("#"))
    {
      toServer="MODE "+nickList[0];
      /* remove the first element */
      nickList.remove(nickList.begin());
    }
    /* Add default destination if it is a channel*/
    else if(destination.startsWith("#")) toServer="MODE "+destination;
    /* Only continue if there was no error */
    if(toServer.length())
    {
      unsigned int modeCount=nickList.count();
      if(modeCount>3)
      {
        modeCount=3;
        /* TODO: Issue a warning here */
      }

      QString modes;
      modes.fill(mode,modeCount);

      toServer+=QString(" ")+giveTake+modes;

      for(unsigned int index=0;index<modeCount;index++) toServer+=" "+nickList[index];
    }
  }
}
