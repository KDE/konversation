/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  server.cpp  -  description
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <unistd.h>
#include <iostream>

#include <qregexp.h>

#include <klocale.h>

#include "server.h"
#include "ircserversocket.h"
#include "konversationapplication.h"

Server::Server(int id)
{
  QStringList serverEntry=QStringList::split(',',KonversationApplication::preferences.getServerById(id),true);

  tryNickNumber=0;

  serverWindow=new ServerWindow(this);
  setNickname(KonversationApplication::preferences.nicknameList[tryNickNumber]);
  serverWindow->show();

  serverName=serverEntry[1];
  serverPort=serverEntry[2].toInt();

  if(serverEntry[4] && serverEntry[4]!="")
  {
    autoJoin=true;
    autoJoinChannel=serverEntry[4];
    autoJoinChannelKey=serverEntry[5];
  }
  else autoJoin=false;

  autoRejoin=true;
  autoReconnect=true;

  connectToIRCServer();

  /* don't delete items when they are removed */
  channelList.setAutoDelete(false);
  /* For /msg query completion */
  completeQueryPosition=0;

  inputFilter.setServer(this);

  incomingTimer.start(10);

  connect(&incomingTimer,SIGNAL(timeout()),
                    this,SLOT  (processIncomingData()) );

  connect(&outputFilter,SIGNAL (openQuery(const QString&,const QString&)),
                   this,SLOT   (addQuery(const QString&,const QString&)) );
}

Server::~Server()
{
  cerr << "Server::~Server()" << endl;
}

void Server::connectToIRCServer()
{
  serverWindow->appendToStatus(i18n("Info"),i18n("Connecting ..."));

  serverSocket=new IRCServerSocket(serverName,serverPort,60);
  if(serverSocket->socket()<=-1)
  {
    broken(serverSocket);
  }
  else
  {
    connect(serverSocket,SIGNAL (readEvent(KSocket *)),this,SLOT (incoming(KSocket *)) );
    connect(serverSocket,SIGNAL (writeEvent(KSocket *)),this,SLOT (send(KSocket *)) );
    connect(serverSocket,SIGNAL (closeEvent(KSocket *)),this,SLOT (broken(KSocket *)) );

    connect(this,SIGNAL (nicknameChanged(const QString&)),serverWindow,SLOT (setNickname(const QString&)) );

    serverWindow->appendToStatus(i18n("Info"),i18n("Connected! Logging in ..."));

    QString connectString="USER " +
                          KonversationApplication::preferences.ident +
                          " 8 * :" +  /* 8 = +i; 4 = +w*/
                          KonversationApplication::preferences.realname;
    queue(connectString);
    queue("NICK "+getNickname());

    emit nicknameChanged(getNickname());

    serverSocket->enableRead(true);
  }
}

QString Server::getAutoJoinCommand()
{
  QString autoString("");

  if(autoJoin) autoString="JOIN "+autoJoinChannel+" "+autoJoinChannelKey;

  return autoString;
}

QString Server::getNextNickname()
{
  QString newNick=getNickname();
  if(tryNickNumber!=4)
  {
    tryNickNumber++;
    if(tryNickNumber==4) newNick=getNickname()+"_";
    else newNick=KonversationApplication::preferences.nicknameList[tryNickNumber];
  }
  return newNick;
}

void Server::processIncomingData()
{
  int pos;

  pos=inputBuffer.find('\n');
  if(pos!=-1)
  {
    QString line=inputBuffer.left(pos);

    inputBuffer=inputBuffer.mid(pos+1);

    cout << line << endl;

    inputFilter.parseLine(line);
  }
}

void Server::incoming(KSocket* ksocket)
{
  char buffer[513];
  int len=0;

  do
  {
    len=read(ksocket->socket(),buffer,512);
    buffer[len]=0;

    inputBuffer+=buffer;
  } while(len==512);
}

void Server::queue(const QString& buffer)
{
  if(buffer.length())
  {
    cerr << "Q: " << buffer << endl;

    outputBuffer+=buffer;
    outputBuffer+="\n";

    serverSocket->enableWrite(true);
  }
}

void Server::send(KSocket* ksocket)
{
  cerr << "-> " << outputBuffer << endl;
  write(ksocket->socket(),outputBuffer.latin1(),outputBuffer.length());
  serverSocket->enableWrite(false);
  outputBuffer="";
}

void Server::ctcpReply(QString& receiver,const QString& text)
{
  queue("NOTICE "+receiver+" :"+0x01+text+0x01);
}

void Server::broken(KSocket* ksocket)
{
  cerr << "Connection broken!" << endl;
  serverWindow->appendToStatus(i18n("Error"),i18n("Connection to Server %1 lost.").arg(serverName));
  /* Close all queries and channels! */
  delete serverSocket;

  if(autoReconnect)
  {
    serverWindow->appendToStatus(i18n("Error"),i18n("Connection to Server %1 lost. Trying to reconnect.").arg(serverName));
    connectToIRCServer();
  }
  else serverWindow->appendToStatus(i18n("Error"),i18n("Connection to Server %1 lost.").arg(serverName));
}

void Server::addQuery(const QString& nickname,const QString& hostmask)
{
  // Only create new query object if there isn't already one with the same name
  Query* query=getQueryByName(nickname);
  if(!query)
  {
    query=new Query(serverWindow->getWindowContainer());
    query->setServer(this);
    query->setQueryName(nickname);
    // Add new query pane to tabwidget
    serverWindow->addView(query->getQueryPane(),0,nickname);

    connect(query,SIGNAL (newText(QWidget*)),serverWindow,SLOT (newText(QWidget*)) );
    connect(query,SIGNAL (closed(Query*)),this,SLOT (removeQuery(Query*)) );
    // Append query to internal list
    queryList.append(query);
  }
  // Always set hostmask
  query->setHostmask(hostmask);
}

QString Server::getNextQueryName()
{
  /* Check if completion position is out of range */
  if(completeQueryPosition>=queryList.count()) completeQueryPosition=0;
  /* return the next query in the list (for /msg completion) */
  if(queryList.count()) return queryList.at(completeQueryPosition++)->getQueryName();

  return QString::null;
}

void Server::removeQuery(Query* query)
{
  /* Remove query page from container */
  serverWindow->getWindowContainer()->removePage(query->getQueryPane());

  /* Traverse through list to find the query */
  Query* lookQuery=queryList.first();
  while(lookQuery)
  {
    /* Did we find our query? */
    if(lookQuery==query)
    {
      /* Remove it from the query list */
      queryList.remove(lookQuery);
      /* break out of the loop */
      lookQuery=0;
    }
    /* else select next query */
    else lookQuery=queryList.next();
  }
  /* Free the query object */
  delete query;
}

void Server::joinChannel(QString& name,QString& hostmask)
{
  // if(mode==singleWindows)
  //   Channel* channel=new Channel(this,name,0);
  // else

  /* Make sure to delete stale Channel on rejoin */
  Channel* channel=getChannelByName(name);
  if(channel)
  {
    removeChannel(channel);
    delete channel;
  }

  channel=new Channel(serverWindow->getWindowContainer());

  channel->setServer(this);
  channel->setChannelName(name);
  channel->setNickname(getNickname());

  serverWindow->addView(channel->getChannelPane(),1,name);
  channel->joinNickname(getNickname(),hostmask);
//  serverWindow->showView(channel->getChannelPane());
  // endif
  channelList.append(channel);
  connect(channel,SIGNAL (newText(QWidget*)),serverWindow,SLOT (newText(QWidget*)) );
}

void Server::removeChannel(Channel* channel)
{
  channelList.removeRef(channel);
}

void Server::updateChannelMode(QString& nick,QString& channelName,char mode,bool plus,QString& parameter)
{
  Channel* channel=getChannelByName(channelName);
  if(channel) channel->updateMode(nick,mode,plus,parameter);
}

void Server::updateChannelModeWidgets(QString& channelName,char mode,QString& parameter)
{
  Channel* channel=getChannelByName(channelName);
  if(channel) channel->updateModeWidgets(mode,true,parameter);
}

void Server::updateChannelQuickButtons(QStringList newButtons)
{
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->updateQuickButtons(newButtons);
    channel=channelList.next();
  }
}

Channel* Server::getChannelByName(const char* name)
{
  /* Convert wanted channel name to lowercase */
  QString wanted=name;
  wanted=wanted.lower();

  /* Traverse through list to find the channel named "name" */
  Channel* lookChannel=channelList.first();
  while(lookChannel)
  {
    if(lookChannel->getChannelName().lower()==wanted) return lookChannel;
    lookChannel=channelList.next();
  }
  /* No channel by that name found? Return 0 (Shouldn't happen) */
  cerr << "Server::getChannelByName(" << name << "): Channel name not found!" << endl;
  return 0;
}

Query* Server::getQueryByName(const char* name)
{
  /* Convert wanted query name to lowercase */
  QString wanted=name;
  wanted=wanted.lower();

  /* Traverse through list to find the query with "name" */
  Query* lookQuery=queryList.first();
  while(lookQuery)
  {
    if(lookQuery->getQueryName().lower()==wanted) return lookQuery;
    lookQuery=queryList.next();
  }
  /* No query by that name found? Must be a new query request. Return 0 */
  return 0;
}

void Server::addNickToChannel(QString& channelName,QString& nickname,QString& hostmask,bool op,bool voice)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->addNickname(nickname,hostmask,op,voice);
}

void Server::nickJoinsChannel(QString& channelName,QString& nickname,QString& hostmask)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->joinNickname(nickname,hostmask);
}

void Server::addHostmaskToNick(QString& sourceNick,QString& sourceHostmask)
{
  Channel* channel=channelList.first();

  while(channel)
  {
    Nick* nick=channel->getNickByName(sourceNick);
    if(nick) nick->setHostmask(sourceHostmask);
    channel=channelList.next();
  }
  /* Set hostmask for query with the same name */
  Query* query=getQueryByName(sourceNick);
  if(query) query->setHostmask(sourceHostmask);
}

void Server::removeNickFromChannel(QString& channelName,QString& nickname,QString& reason,bool quit)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->removeNick(nickname,reason,quit);
}

void Server::nickWasKickedFromChannel(QString& channelName,QString& nickname,QString& kicker,QString& reason)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->kickNick(nickname,kicker,reason);
}

void Server::removeNickFromServer(QString& nickname,QString& reason)
{
  Channel* channel=channelList.first();
  while(channel)
  {
    removeNickFromChannel(channel->getChannelName(),nickname,reason,true);
    channel=channelList.next();
  }
}

void Server::renameNick(QString& nickname,QString& newNick)
{
  /* Rename the nick in every channel they are in */
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->renameNick(nickname,newNick);
    channel=channelList.next();
  }
  /* If this was our own nickchange, tell our server object about it */
  if(nickname==getNickname()) setNickname(newNick);
  /* If we had a query with this nick, change that name, too */
  Query* query=queryList.first();
  while(query)
  {
    if(query->getQueryName().lower()==nickname.lower())
    {
      query->setQueryName(newNick);
      serverWindow->getWindowContainer()->changeTab(query->getQueryPane(),newNick);
    }
    query=queryList.next();
  }
}

void Server::appendToChannel(const char* channel,const char* nickname,const char* message)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->append(nickname,message);
}

void Server::appendActionToChannel(const char* channel,const char* nickname,const char* message)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->appendAction(nickname,message);
}

void Server::appendServerMessageToChannel(const char* channel,const char* type,const char* message)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->appendServerMessage(type,message);
}

void Server::appendCommandMessageToChannel(const char* channel,const char* command,const char* message)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->appendCommandMessage(command,message);
}

void Server::appendToQuery(const char* queryName,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendQuery(queryName,message);
  else cerr << "Server::appendToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendActionToQuery(const char* queryName,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendAction(queryName,message);
  else cerr << "Server::appendActionToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendServerMessageToQuery(const char* queryName,const char* type,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendServerMessage(type,message);
  else cerr << "Server::appendServerMessageToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendCommandMessageToQuery(const char* queryName,const char* command,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendCommandMessage(command,message);
  else cerr << "Server::appendCommandMessageToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendStatusMessage(const char* type,const char* message)
{
  serverWindow->appendToStatus(type,message);
}

void Server::setNickname(const QString& newNickname)
{
  nickname=newNickname;
  serverWindow->setNickname(newNickname);
}

void Server::setChannelTopic(QString& channel,QString &topic)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->setTopic(topic);
}

void Server::setChannelTopic(QString& nickname,QString& channel,QString &topic) // Overloaded
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->setTopic(nickname,topic);
}

bool Server::isNickname(QString& compare)
{
  return (nickname==compare);
}

QString& Server::getNickname()
{
  return nickname;
}

QString Server::parseWildcards(const QString& toParse,const QString& nickname,const QString& channelName,QStringList* nickList,const QString& queryName,const QString& parameter)
{
  /* cut button name from definition */
  QString out(toParse.mid(toParse.find(',')+1));
  /* define default separator and regular expression for definition */
  QString separator(" ");
  QRegExp separatorRegExp("%s[^%]*%");

  int pos;
  /* separator definition found? */
  pos=out.find(separatorRegExp);
  if(pos!=-1)
  {
    /* skip "%s" at the beginning */
    pos+=2;
    /* copy out all text to the next "%" as new separator */
    separator=out.mid(pos,out.find("%",pos+1)-pos);
    /* remove separator definition from string */
    out.replace(separatorRegExp,"");
  }

  out.replace(QRegExp("%u"),nickList->join(separator));
  if(channelName) out.replace(QRegExp("%c"),channelName);
  out.replace(QRegExp("%o"),nickname);
  out.replace(QRegExp("%n"),"\n");
//  out.replace(QRegExp("%f"),getFortuneCookie());
//  out.replace(QRegExp("%p"),parameter);
  if(queryName) out.replace(QRegExp("%q"),queryName);

  /* finally replace all "%p" with "%" */
  out.replace(QRegExp("%p"),"%");

  delete nickList;

  return out;
}
