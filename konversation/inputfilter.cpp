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
*/

#include <iostream>

#include <qstringlist.h>

#include <klocale.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define VERSION 0.2
#endif

#include "server.h"
#include "inputfilter.h"
#include "errorcodes.h"

InputFilter::InputFilter()
{
  cerr << "InputFilter::InputFilter()" << endl;
}

InputFilter::~InputFilter()
{
  cerr << "InputFilter::~InputFilter()" << endl;
}

void InputFilter::setServer(Server* newServer)
{
  server=newServer;
}

void InputFilter::parseLine(QString newLine)
{
  QString trailing="";
  /* Remove white spaces at the end and beginning */
  newLine=newLine.stripWhiteSpace();
  /* Find end of middle parameter list */
  int pos=newLine.find(" :");
  /* Was there a trailing parameter? */
  if(pos!=-1)
  {
    /* Copy trailing parameter */
    trailing=newLine.mid(pos+2);
    /* Cut trailing parameter from string */
    newLine=newLine.left(pos);
  }
  /* Remove all unneccessary white spaces to make parsing easier */
  QString incomingLine=newLine.simplifyWhiteSpace();

  QString prefix="";
  /* Do we have a prefix? */
  if(incomingLine[0]==':')
  {
    /* Find end of prefix */
    pos=incomingLine.find(' ');
    /* Copy prefix */
    prefix=incomingLine.mid(1,pos-1);
    /* Remove prefix from line */
    incomingLine=incomingLine.mid(pos+1);
  }

  /* Find end of command */
  pos=incomingLine.find(' ');
  /* Copy command (all lowercase to make parsing easier) */
  QString command=incomingLine.left(pos).lower();
  /* Are there parameters left in the string? */
  QStringList parameterList;
  if(pos!=-1)
  {
    /* Cut out the command */
    incomingLine=incomingLine.mid(pos+1);
    /* The rest of the string will be the parameter list */
    parameterList=QStringList::split(" ",incomingLine);
  }
  /* Server command, if no "!" was found in prefix */
  if(prefix.find('!')==-1 && prefix!=server->getNickname()) parseServerCommand(prefix,command,parameterList,trailing);
  else parseClientCommand(prefix,command,parameterList,trailing);
}

void InputFilter::parseClientCommand(QString& prefix,QString& command,QStringList& parameterList,QString& trailing)
{
  /* Extract nickname fron prefix */
  QString sourceNick=prefix.left(prefix.find("!"));
  QString sourceHostmask=prefix.mid(prefix.find("!")+1);

  if(command=="privmsg")
  {
    /* Channel message */
    if(parameterList[0][0]=='#')
    {
      /* CTCP message? */
      if(trailing[0]==1)
      {
        /* cut out the CTCP command */
        QString ctcp=trailing.mid(1,trailing.find(1,1)-1);

        QString ctcpCommand=ctcp.left(ctcp.find(" "));
        QString ctcpArgument=ctcp.mid(ctcp.find(" ")+1);

        /* If it was a ctcp action, build an action string */
        if(ctcpCommand.lower()=="action") server->appendActionToChannel(parameterList[0],sourceNick,ctcpArgument);
        /* No known CTCP request, give a general message */
        else server->appendServerMessageToChannel(parameterList[0],"CTCP",i18n("Received unknown CTCP-%1 request from %2 to Channel %3").arg(ctcp).arg(sourceNick).arg(parameterList[0]));
      }
      /* No CTCP, so it's an ordinary channel message */
      else server->appendToChannel(parameterList[0],sourceNick,trailing);
    }
    /* No channel message */
    else
    {
      /* CTCP message? */
      if(trailing[0]==1)
      {
        /* cut out the CTCP command */
        QString ctcp=trailing.mid(1,trailing.find(1,1)-1);

        QString ctcpCommand=ctcp.left(ctcp.find(" "));
        QString ctcpArgument=ctcp.mid(ctcp.find(" ")+1);

        /* If it was a ctcp action, build an action string */
        if(ctcpCommand.lower()=="action")
        {
          /* Check if this nick is already in a query with us */
          Query* query=server->getQueryByName(sourceNick);
          /* If not, create a new one */
          if(!query) server->addQuery(sourceNick,sourceHostmask);
          /* else remember hostmask for this nick, it could have changed */
          else server->addHostmaskToNick(sourceNick,sourceHostmask);

          server->appendActionToQuery(sourceNick,ctcpArgument);
        }
        /* Maybe it was a version request, so act appropriately */
        else if(ctcpCommand.lower()=="version")
        {
          server->appendStatusMessage(i18n("CTCP"),i18n("Received Version request from %1.").arg(sourceNick));
          server->ctcpReply(sourceNick,QString("VERSION Konversation %1 (C)2002 Dario Abatianni").arg(VERSION));
        }
        /* No known CTCP request, give a general message */
        else server->appendStatusMessage(i18n("CTCP"),i18n("Received unknown CTCP-%1 request from %2").arg(ctcp).arg(sourceNick));
      }
      /* No CTCP, so it's an ordinary query message */
      else
      {
        /* Create a new query (server will check for dupes) */
        server->addQuery(sourceNick,sourceHostmask);
        /* else remember hostmask for this nick, it could have changed */
        server->addHostmaskToNick(sourceNick,sourceHostmask);
        /* Append this message to the query */
        server->appendToQuery(sourceNick,trailing);
      }
    }
  }
  else if(command=="join")
  {
    /* Did we join the channel, or was it someone else? */
    if(server->isNickname(sourceNick))
    {
      /* Join the channel */
      server->joinChannel(trailing,sourceHostmask);
      /* Request modes for the channel */
      server->queue("MODE "+trailing);
    }
    else server->nickJoinsChannel(trailing,sourceNick,sourceHostmask);
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
  else if(command=="mode") /* mode #channel -/+ mmm params */
  {
    const QString modestring=parameterList[1];

    bool plus=false;
    int parameterIndex=0;
    /* List of modes that need a parameter (note exception with -k and -l) */
    QString parameterModes="oOvkbleI";

    for(int index=0;modestring[index]!=0;index++)
    {
      unsigned char mode=modestring[index];
      QString parameter;

      /* Check if this is a mode or a +/- qualifier */
      if(mode=='+' || mode=='-') plus=(mode=='+');
      else
      {
        /* Check if this was a parameter mode */
        if(parameterModes.find(mode)!=-1)
        {
          /* Check if the mode actually wants a parameter. -k and -l do not! */
          if(plus || (!plus && (mode!='k') && (mode!='l')))
          {
            /* Remember the mode parameter */
            parameter=parameterList[2+parameterIndex];
            /* Switch to next parameter */
            parameterIndex++;
          }
        }
        /* Let the channel update its modes */
        server->updateChannelMode(sourceNick,parameterList[0],mode,plus,parameter);
      }
    }
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
    server->queue("PONG "+trailing);
  }
  else if(command=="001" || command=="002" || command=="003")
  {
    /* Remember server's insternal name (don't know if I will need this, though) */
    if(command=="001") server->setIrcName(prefix);
    server->appendStatusMessage(i18n("Welcome"),trailing);
  }
  else if(command=="004") server->appendStatusMessage(i18n("Welcome"),
                          i18n("Server %1 (Version %2), User modes: %3, Channel modes: %4").
                          arg(parameterList[1]).
                          arg(parameterList[2]).
                          arg(parameterList[3]).
                          arg(parameterList[4]) );

  else if(command==RPL_CHANNELMODEIS)
  {
    const QString modeString=parameterList[2];
    /* This is the string the user will see */
    QString modesAre="";

    for(int index=0;modeString[index]!=0;index++)
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
        else if(mode=='l')
        {
          parameter=parameterList[parameterCount++];
          modesAre+=i18n("limited to %1 users").arg(parameter);
        }
        server->updateChannelModeWidgets(parameterList[1],mode,parameter);
      }
    } // endfor
    if(modesAre!="") server->appendCommandMessageToChannel(parameterList[1],i18n("Mode"),"Channel modes: "+modesAre);
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
  /* Topic set message */
  else if(command==RPL_TOPIC)
  {
    /* Update server window */
    server->setChannelTopic(parameterList[1],trailing);
  }
  /* Nick already on the server, so try another one */
  else if(command==ERR_NICKNAMEINUSE)
  {
    /* Get the next nick from the list */
    QString newNick=server->getNextNickname();
    /* Update Server window */
    server->setNickname(newNick);
    /* Show message */
    server->appendStatusMessage(i18n("Nick"),i18n("Nickname already in use. Trying %1.").arg(newNick));
    /* Send nickchange request to the server */
    server->queue("NICK "+newNick);
  }
  else if(command==RPL_MOTD)
  {
    server->appendStatusMessage(i18n("MOTD"),trailing);
  }
  else if(command==RPL_ENDOFMOTD)
  {
    server->appendStatusMessage(i18n("MOTD"),i18n("End of Message Of The Day"));
    /* Autojoin (for now this must be enough) */
    server->queue(server->getAutoJoinCommand());
  }
  /* All yet unknown messages go into the status window unaltered */
  else
  {
    server->appendStatusMessage(command,parameterList.join(" ")+" "+trailing);
  }
}
