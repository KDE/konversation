/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  inputfilter.cpp  -  description
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qstringlist.h>
#include <qdatetime.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdebug.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define VERSION 0.10
#endif

#include "inputfilter.h"
#include "server.h"
#include "errorcodes.h"
#include "konversationapplication.h"

#if QT_VERSION < 0x030100
#include "main.h"
#endif

InputFilter::InputFilter()
{
  kdDebug() << "InputFilter::InputFilter()" << endl;
  welcomeSent=false;
}

InputFilter::~InputFilter()
{
  kdDebug() << "InputFilter::~InputFilter()" << endl;
}

void InputFilter::setServer(Server* newServer)
{
  server=newServer;
}

void InputFilter::parseLine(QString newLine)
{
  QString trailing="";
  // Remove white spaces at the end and beginning
  newLine=newLine.stripWhiteSpace();
  // Find end of middle parameter list
  int pos=newLine.find(" :");
  // Was there a trailing parameter?
  if(pos!=-1)
  {
    // Copy trailing parameter
    trailing=newLine.mid(pos+2);
    // Cut trailing parameter from string
    newLine=newLine.left(pos);
  }
  // Remove all unneccessary white spaces to make parsing easier
  QString incomingLine=newLine.simplifyWhiteSpace();

  QString prefix="";
  // Do we have a prefix?
  if(incomingLine[0]==':')
  {
    // Find end of prefix
    pos=incomingLine.find(' ');
    // Copy prefix
    prefix=incomingLine.mid(1,pos-1);
    // Remove prefix from line
    incomingLine=incomingLine.mid(pos+1);
  }

  // Find end of command
  pos=incomingLine.find(' ');
  // Copy command (all lowercase to make parsing easier)
  QString command=incomingLine.left(pos).lower();
  // Are there parameters left in the string?
  QStringList parameterList;
  if(pos!=-1)
  {
    // Cut out the command
    incomingLine=incomingLine.mid(pos+1);
    // The rest of the string will be the parameter list
    parameterList=QStringList::split(" ",incomingLine);
  }
  // Server command, if no "!" was found in prefix
  if(prefix.find('!')==-1 && prefix!=server->getNickname()) parseServerCommand(prefix,command,parameterList,trailing);
  else parseClientCommand(prefix,command,parameterList,trailing);
}

void InputFilter::parseClientCommand(QString& prefix,QString& command,QStringList& parameterList,QString& trailing)
{
  // Extract nickname fron prefix
  QString sourceNick=prefix.left(prefix.find("!"));
  QString sourceHostmask=prefix.mid(prefix.find("!")+1);

  if(command=="privmsg")
  {
    // Channel message
    if(isAChannel(parameterList[0]))
    {
      // CTCP message?
      if(trailing[0]==1)
      {
        // cut out the CTCP command
        QString ctcp=trailing.mid(1,trailing.find(1,1)-1);

        QString ctcpCommand=ctcp.left(ctcp.find(" ")).lower();
        QString ctcpArgument=ctcp.mid(ctcp.find(" ")+1);

        // If it was a ctcp action, build an action string
        if(ctcpCommand=="action")
        {
          if(!isIgnore(prefix,Ignore::Channel))
            server->appendActionToChannel(parameterList[0],sourceNick,ctcpArgument);
        }
        // Answer ping requests
        else if(ctcpCommand=="ping")
        {
          if(!isIgnore(prefix,Ignore::CTCP))
          {
            server->appendStatusMessage(i18n("CTCP"),i18n("Received CTCP-PING request from %1 to channel %2, sending answer.").arg(sourceNick).arg(parameterList[0]));
            server->ctcpReply(sourceNick,QString("PING %1").arg(ctcpArgument));
          }
        }

        // No known CTCP request, give a general message
        else
        {
          if(!isIgnore(prefix,Ignore::CTCP))
            server->appendServerMessageToChannel(parameterList[0],"CTCP",i18n("Received unknown CTCP-%1 request from %2 to Channel %3").arg(ctcp).arg(sourceNick).arg(parameterList[0]));
        }
      }
      // No CTCP, so it's an ordinary channel message
      else
      {
        if(!isIgnore(prefix,Ignore::Channel))
          server->appendToChannel(parameterList[0],sourceNick,trailing);
      }
    }
    // No channel message
    else
    {
      // CTCP message?
      if(trailing[0]==1)
      {
        // cut out the CTCP command
        QString ctcp=trailing.mid(1,trailing.find(1,1)-1);

        QString ctcpCommand=ctcp.left(ctcp.find(" ")).lower();
        QString ctcpArgument=ctcp.mid(ctcp.find(" ")+1);

        // If it was a ctcp action, build an action string
        if(ctcpCommand=="action")
        {
          // Check if we ignore queries from this nick
          if(!isIgnore(prefix,Ignore::Channel))
          {
            // Check if this nick is already in a query with us
            Query* query=server->getQueryByName(sourceNick);
            // If not, create a new one
            if(!query) server->addQuery(sourceNick,sourceHostmask);
            // else remember hostmask for this nick, it could have changed
            else server->addHostmaskToNick(sourceNick,sourceHostmask);

            server->appendActionToQuery(sourceNick,ctcpArgument);
          }
        }
        // DCC request?
        else if(ctcpCommand=="dcc")
        {
          if(!isIgnore(prefix,Ignore::DCC))
          {
            // Extract DCC type and argument list
            QString dccType=ctcpArgument.lower().section(' ',0,0);
            QStringList dccArgument=QStringList::split(' ',ctcpArgument.mid(ctcpArgument.find(" ")+1).lower());

            // Incoming file?
            if(dccType=="send")
            {
              emit addDccGet(sourceNick,dccArgument);
            }
            // Incoming file that shall be resumed?
            else if(dccType=="accept")
              {
              emit resumeDccGetTransfer(sourceNick,dccArgument);
            }
            // Remote client wants our sent file resumed
            else if(dccType=="resume")
            {
              emit resumeDccSendTransfer(sourceNick,dccArgument);
            }
          }
        }
        // Maybe it was a version request, so act appropriately
        else if(ctcpCommand=="version")
        {
          if(!isIgnore(prefix,Ignore::CTCP))
          {
            server->appendStatusMessage(i18n("CTCP"),i18n("Received Version request from %1.").arg(sourceNick));
            server->ctcpReply(sourceNick,QString("VERSION Konversation %1 (C)2002-2003 Dario Abatianni and Matthias Gierlings").arg(VERSION));
          }
        }
        // Answer ping requests
        else if(ctcpCommand=="ping")
        {
          if(!isIgnore(prefix,Ignore::CTCP))
          {
            server->appendStatusMessage(i18n("CTCP"),i18n("Received CTCP-PING request from %1, sending answer.").arg(sourceNick));
            server->ctcpReply(sourceNick,QString("PING %1").arg(ctcpArgument));
          }
        }
        // No known CTCP request, give a general message
        else
        {
          if(!isIgnore(prefix,Ignore::CTCP))
            server->appendStatusMessage(i18n("CTCP"),i18n("Received unknown CTCP-%1 request from %2").arg(ctcp).arg(sourceNick));
        }
      }
      // No CTCP, so it's an ordinary query message
      else
      {
        if(!isIgnore(prefix,Ignore::Query))
        {
          // Create a new query (server will check for dupes)
          server->addQuery(sourceNick,sourceHostmask);
          // else remember hostmask for this nick, it could have changed
          server->addHostmaskToNick(sourceNick,sourceHostmask);
          // Append this message to the query
          server->appendToQuery(sourceNick,trailing);
        }
      }
    }
  }
  else if(command=="notice")
  {
    if(!isIgnore(prefix,Ignore::Notice))
    {
      // Channel notice?
      if(isAChannel(parameterList[0]))
      {
        server->appendServerMessageToChannel(parameterList[0],i18n("Notice"),i18n("-%1 to %2- %3").arg(sourceNick).arg(parameterList[0]).arg(trailing));
      }
      // Private notice
      else
      {
        // Was this a CTCP reply?
        if(trailing[0]==1)
        {
          // cut 0x01 bytes from trailing string
          QString ctcp(trailing.mid(1,trailing.length()-2));
          QString replyReason(ctcp.section(' ',0,0));
          QString reply(ctcp.section(' ',1));

          // pong reply, calculate turnaround time
          if(replyReason.lower()=="ping")
          {
#if QT_VERSION < 0x030100
            int dateArrived=toTime_t(QDateTime::currentDateTime());
#else
            int dateArrived=QDateTime::currentDateTime().toTime_t();
#endif
            int dateSent=reply.toInt();
            
            server->appendStatusMessage(i18n("CTCP"),i18n("Received CTCP-PING reply from %1: %2 seconds").arg(sourceNick).arg(dateArrived-dateSent));
          }
          // all other ctcp replies get a general message
          else
            server->appendStatusMessage(i18n("CTCP"),i18n("Received CTCP-%1 reply from %2: %3").arg(replyReason).arg(sourceNick).arg(reply));
        }
        // No, so it was a normal notice
        else
          server->appendStatusMessage(i18n("Notice"),i18n("-%1- %2").arg(sourceNick).arg(trailing));
      }
    }
  }
  else if(command=="join")
  {
    QString channelName(trailing);
    /* Sometimes JOIN comes without ":" in front of the channel name */
    if(channelName=="") channelName=parameterList[parameterList.count()-1];
    /* Did we join the channel, or was it someone else? */
    if(server->isNickname(sourceNick))
    {
      QString key;
/*
      // TODO: Try to remember channel keys for autojoins and manual joins, so
      //       we can get %k to work

      if(channelName.find(' ')!=-1)
      {
        key=channelName.section(' ',1,1);
        channelName=channelName.section(' ',0,0);
        kdDebug() << "Found channel key " << key << endl;
      }
*/
      // Join the channel
      server->joinChannel(channelName,sourceHostmask,key);
      // Request modes for the channel
      server->queue("MODE "+channelName);
    }
    else server->nickJoinsChannel(channelName,sourceNick,sourceHostmask);
  }
  else if(command=="kick")
  {
    server->nickWasKickedFromChannel(parameterList[0],parameterList[1],sourceNick,trailing);
  }
  else if(command=="part")
  {
    server->removeNickFromChannel(parameterList[0],sourceNick,trailing);
  }
  else if(command=="quit")
  {
    server->removeNickFromServer(sourceNick,trailing);
  }
  else if(command=="nick")
  {
    server->renameNick(sourceNick,trailing);
  }
  else if(command=="topic")
  {
    server->setChannelTopic(sourceNick,parameterList[0],trailing);
  }
  else if(command=="mode") // mode #channel -/+ mmm params
  {
    parseModes(sourceNick,parameterList);
  }
  else if(command=="invite")
  {
    server->appendStatusMessage(i18n("Invite"),i18n("%1 invited you into channel %2").arg(sourceNick).arg(trailing));
  }
  else
  {
    server->appendStatusMessage(command,parameterList.join(" ")+" "+trailing);
  }
}

void InputFilter::parseServerCommand(QString& prefix,QString& command,QStringList& parameterList,QString& trailing)
{
  if(command=="ping")
  {
    QString text;
    text=(trailing.length()) ? trailing : parameterList.join(" ");
    text=":"+text;
    if(prefix.length()) text=prefix+" "+text;
    server->queue("PONG "+text);
  }
  else if(command=="error :closing link:")
  {
    kdDebug() << "link closed" << endl;
  }
  else if(command==RPL_WELCOME || command==RPL_YOURHOST || command==RPL_CREATED)
  {
    if(command==RPL_WELCOME)
    {
      // Remember server's insternal name
      server->setIrcName(prefix);
      // Send the welcome signal, so the server class knows we are connected properly
      if(!welcomeSent)
      {
        emit welcome();
        welcomeSent=true;
      }
    }
    server->appendStatusMessage(i18n("Welcome"),trailing);
  }
  else if(command==RPL_MYINFO) server->appendStatusMessage(i18n("Welcome"),
                          i18n("Server %1 (Version %2), User modes: %3, Channel modes: %4").
                          arg(parameterList[1]).
                          arg(parameterList[2]).
                          arg(parameterList[3]).
                          arg(parameterList[4]) );
  // FIXME: Untested
  else if(command==RPL_BOUNCE)
  {
    server->appendStatusMessage(i18n("Bounce"),parameterList.join(" "));
  }

  else if(command==RPL_CHANNELMODEIS)
  {
    const QString modeString=parameterList[2];
    // This is the string the user will see
    QString modesAre="";

    for(unsigned int index=0;index<modeString.length();index++)
    {
      QString parameter;
      int parameterCount=3;
      char mode=modeString[index];
      if(mode!='+')
      {
        parameter="";
        if(modesAre!="") modesAre+=", ";
        if(mode=='t') modesAre+=i18n("topic protection");
        else if(mode=='n') modesAre+=i18n("no messages from outside");
        else if(mode=='s') modesAre+=i18n("secret");
        else if(mode=='i') modesAre+=i18n("invite only");
        else if(mode=='p') modesAre+=i18n("private");
        else if(mode=='m') modesAre+=i18n("moderated");
        else if(mode=='k')
        {
          parameter=parameterList[parameterCount++];
          modesAre+=i18n("keyword protected");
        }
        else if(mode=='a') modesAre+=i18n("anonymous");
        else if(mode=='r') modesAre+=i18n("server reop");
        else if(mode=='c') modesAre+=i18n("no colors allowed");
        else if(mode=='l')
        {
          parameter=parameterList[parameterCount++];
          modesAre+=i18n("limited to %1 users").arg(parameter);
        }
        else modesAre+=mode;

        server->updateChannelModeWidgets(parameterList[1],mode,parameter);
      }
    } // endfor
    if(modesAre!="") server->appendCommandMessageToChannel(parameterList[1],i18n("Mode"),"Channel modes: "+modesAre);
  }
  else if(command==RPL_CHANNELCREATED)
  {
    QDateTime when;
    when.setTime_t(parameterList[2].toUInt());

    server->appendCommandMessageToChannel(parameterList[1],i18n("Created"),i18n("This channel has been created on %1.").arg(when.toString(Qt::LocalDate)));
  }
  else if(command==RPL_NAMREPLY)
  {
    QStringList nickList=QStringList::split(" ",trailing);

    for(unsigned int index=0;index<nickList.count();index++)
    {
      bool op=false;
      bool voice=false;
      QString nick=nickList[index];

      if(nick[0]=='@') op=true;
      if(nick[0]=='+') voice=true;

      QString nickname=(op || voice) ? nick.mid(1) : nick;
      QString hostmask="";

      server->addNickToChannel(parameterList[2],nickname,hostmask,op,voice);
    }
  }
  else if(command==RPL_ENDOFNAMES)
  {
    server->appendCommandMessageToChannel(parameterList[1],i18n("Names"),i18n("End of NAMES list."));
  }
  // Topic set messages
  else if(command==RPL_TOPIC)
  {
    // Update channel window
    server->setChannelTopic(parameterList[1],trailing);
  }
  else if(command==RPL_TOPICSETBY)
  {
    // Inform user who set the topic and when
    QDateTime when;
    when.setTime_t(parameterList[3].toUInt());
    server->appendCommandMessageToChannel(parameterList[1],i18n("Topic"),i18n("Topic was set by %1 on %2.").arg(parameterList[2]).arg(when.toString(Qt::LocalDate)));
  }
  else if(command==ERR_NOSUCHNICK)
  {
    server->appendStatusMessage(i18n("Error"),i18n("%1: No such nick/channel.").arg(parameterList[1]));
  }
  // Nick already on the server, so try another one
  else if(command==ERR_NICKNAMEINUSE)
  {
    // Get the next nick from the list
    QString newNick=server->getNextNickname();
    // Update Server window
    server->setNickname(newNick);
    // Show message
    server->appendStatusMessage(i18n("Nick"),i18n("Nickname already in use. Trying %1.").arg(newNick));
    // Send nickchange request to the server
    server->queue("NICK "+newNick);
  }
  else if(command==RPL_MOTDSTART)
  {
    server->appendStatusMessage(i18n("MOTD"),i18n("Message Of The Day:"));
  }
  else if(command==RPL_MOTD)
  {
    server->appendStatusMessage(i18n("MOTD"),trailing);
  }
  else if(command==RPL_ENDOFMOTD)
  {
    server->appendStatusMessage(i18n("MOTD"),i18n("End of Message Of The Day"));
    // Autojoin (for now this must be enough)
    if(server->getAutoJoin()) server->queue(server->getAutoJoinCommand());
  }
  else if(command==RPL_GLOBALUSERS) // Current global users: 589 Max: 845
  {
    QString current(trailing.section(' ',3,3));
    QString max(trailing.section(' ',5,5));
    server->appendStatusMessage(i18n("Users"),i18n("Current users on the network: %1 of at most %2.").arg(current).arg(max));
  }
  else if(command==RPL_LOCALUSERS) // Current local users: 589 Max: 845
  {
    QString current(trailing.section(' ',3,3));
    QString max(trailing.section(' ',5,5));
    server->appendStatusMessage(i18n("Users"),i18n("Current users on %1: %2 of at most %3.").arg(prefix).arg(current).arg(max));
  }
  else if(command==RPL_ISON)
  {
    // Tell server to start the next notify timer round
    emit notifyResponse(trailing);
  }
  else if(command=="pong")
  {
    // Since we use PONG replys to measure lag, too, we check, if this PONG was
    // due to Lag measures and tell the notify system about it. We use "###" as
    // response, because this couldn't be a 303 reply, so it must be a PONG reply
    if(trailing=="LAG") emit notifyResponse("###");
  }
  else if(command==RPL_AWAY)
  {
    server->appendStatusMessage(i18n("Away"),i18n("%1 is away: %2").arg(parameterList[1]).arg(trailing));
  }
  else if(command==RPL_INVITING)
  {
    server->appendStatusMessage(i18n("Invite"),i18n("You invited %1 into channel %2.").arg(parameterList[1]).arg(parameterList[2]));
  }
  else if(command=="mode")
  {
    parseModes(prefix,parameterList);
  }
  else if(command=="notice")
  {
    server->appendStatusMessage(i18n("Notice"),i18n("-%1- %2").arg(prefix).arg(trailing));
  }
  else if(command==RPL_WHOISUSER)
  {
    server->appendStatusMessage(i18n("Whois"),i18n("%1 is %2@%3 (%4)").arg(parameterList[1])
                                                                      .arg(parameterList[2])
                                                                      .arg(parameterList[3])
                                                                      .arg(trailing));
  }
  else if(command==RPL_WHOISCHANNELS)
  {
    QStringList userChannels;
    QStringList voiceChannels;
    QStringList opChannels;

    // get a list of all channels the user is in
    QStringList channelList=QStringList::split(' ',trailing);
    channelList.sort();

    // split up the list in channels where they are operator / user / voice
    for(unsigned int index=0;index<channelList.count();index++)
    {
      QString lookChannel=channelList[index];
      if(lookChannel.startsWith("@")) opChannels.append(lookChannel.mid(1));
      else if(lookChannel.startsWith("+")) voiceChannels.append(lookChannel.mid(1));
      else userChannels.append(lookChannel);
    }
    if(userChannels.count())
      server->appendStatusMessage(i18n("Whois"),i18n("%1 is user on channels: %2").arg(parameterList[1]).arg(userChannels.join(" ")) );
    if(voiceChannels.count())
      server->appendStatusMessage(i18n("Whois"),i18n("%1 has voice on channels: %2").arg(parameterList[1]).arg(voiceChannels.join(" ")) );
    if(opChannels.count())
      server->appendStatusMessage(i18n("Whois"),i18n("%1 is operator on channels: %2").arg(parameterList[1]).arg(opChannels.join(" ")) );
  }
  else if(command==RPL_WHOISSERVER)
  {
    server->appendStatusMessage(i18n("Whois"),i18n("%1 is online via %2 (%3)").arg(parameterList[1])
                                                                              .arg(parameterList[2])
                                                                              .arg(trailing) );
  }
  else if(command==RPL_WHOISIDENTIFY)
  {
    server->appendStatusMessage(i18n("Whois"),i18n("%1 has identified for this nick.").arg(parameterList[1]));
  }
  else if(command==RPL_WHOISIDLE)
  {
    server->appendStatusMessage(i18n("Whois"),i18n("%1 is idle since %2 seconds.").arg(parameterList[1])
                                                                                  .arg(parameterList[2]));
    if(parameterList.count()==4)
    {
      QDateTime when;
      when.setTime_t(parameterList[3].toUInt());

      server->appendStatusMessage(i18n("Whois"),i18n("%1 is online since %2.").arg(parameterList[1])
                                                                              .arg(when.toString(Qt::LocalDate)));
    }
  }
  else if(command==RPL_ENDOFWHOIS)
  {
    server->appendStatusMessage(i18n("Whois"),i18n("End of WHOIS list."));
  }
  // All yet unknown messages go into the frontmost window unaltered
  else
  {
    server->appendStatusMessage(command,parameterList.join(" ")+" "+trailing);
  }
}

void InputFilter::parseModes(QString sourceNick,QStringList parameterList)
{
  const QString modestring=parameterList[1];

  bool plus=false;
  int parameterIndex=0;
  // List of modes that need a parameter (note exception with -k and -l)
  QString parameterModes="oOvkbleI";

  for(unsigned int index=0;index<modestring.length();index++)
  {
    unsigned char mode=modestring[index];
    QString parameter;

    // Check if this is a mode or a +/- qualifier
    if(mode=='+' || mode=='-') plus=(mode=='+');
    else
    {
       // Check if this was a parameter mode
      if(parameterModes.find(mode)!=-1)
      {
        // Check if the mode actually wants a parameter. -k and -l do not!
        if(plus || (!plus && (mode!='k') && (mode!='l')))
        {
          // Remember the mode parameter
          parameter=parameterList[2+parameterIndex];
          // Switch to next parameter
          parameterIndex++;
        }
      }
      // Let the channel update its modes
      server->updateChannelMode(sourceNick,parameterList[0],mode,plus,parameter);
    }
  }
}

// # & + and ! are Channel identifiers
bool InputFilter::isAChannel(QString check)
{
  QChar initial=check.at(0);

  return (initial=='#' || initial=='&' || initial=='+' || initial=='!');
}

bool InputFilter::isIgnore(QString sender,Ignore::Type type)
{
  bool doIgnore=false;

  QPtrList<Ignore> list=KonversationApplication::preferences.getIgnoreList();

  for(unsigned int index=0;index<list.count();index++)
  {
    Ignore* item=list.at(index);
    QRegExp ignoreItem(item->getName(),false,true);
    if(ignoreItem.exactMatch(sender) && (item->getFlags() & type)) doIgnore=true;
    if(ignoreItem.exactMatch(sender) && (item->getFlags() & Ignore::Exception)) return false;
  }

  return doIgnore;
}

#include "inputfilter.moc"
