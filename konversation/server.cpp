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

  $Id$
*/

#include <unistd.h>

#include <qregexp.h>
#include <qhostaddress.h>
#include <qtextcodec.h>

#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>

#include "server.h"
#include "query.h"
#include "channel.h"
#include "serverwindow.h"
#include "ircserversocket.h"
#include "konversationapplication.h"
#include "dcctransfer.h"
#include "dccrecipientdialog.h"
#include "nick.h"

Server::Server(int id)
{
  setName("server");
  QStringList serverEntry=QStringList::split(',',KonversationApplication::preferences.getServerById(id),true);
  setIdentity(KonversationApplication::preferences.getIdentityByName(serverEntry[7]));

  tryNickNumber=0;
  checkTime=0;
  reconnectCounter=0;

  serverName=serverEntry[1];
  serverPort=serverEntry[2].toInt();
  serverKey=serverEntry[3];

  resolver.setRecipient(this);
  installEventFilter(this);

  lastDccDir=QString::null;

  serverWindow=new ServerWindow(this);
  setNickname(identity->getNickname(tryNickNumber));
  bot=identity->getBot();
  botPassword=identity->getPassword();
  serverWindow->setIdentity(getIdentity());
  serverWindow->show();
  statusPanel=serverWindow->getStatusView();

  serverSocket.setAddress(serverName,serverPort);

  if(serverEntry[4] && !serverEntry[4].isEmpty())
  {
    setAutoJoin(true);
    setAutoJoinChannel(serverEntry[4]);
    setAutoJoinChannelKey(serverEntry[5]);
  }
  else autoJoin=false;

  autoRejoin=KonversationApplication::preferences.getAutoRejoin();
  autoReconnect=KonversationApplication::preferences.getAutoReconnect();

  connectToIRCServer();

  // don't delete items when they are removed
  channelList.setAutoDelete(false);
  // For /msg query completion
  completeQueryPosition=0;

  inputFilter.setServer(this);
  outputFilter.setIdentity(identity);

  notifyTimer.setName("notifyTimer");
  incomingTimer.setName("incomingTimer");
  incomingTimer.start(10);

  connect(&incomingTimer,SIGNAL(timeout()),
                    this,SLOT  (processIncomingData()) );

  connect(&outputFilter,SIGNAL (openQuery(const QString&,const QString&)),
                   this,SLOT   (addQuery(const QString&,const QString&)) );
  connect(&outputFilter,SIGNAL (requestDccSend()),
                   this,SLOT   (requestDccSend()) );
  connect(&outputFilter,SIGNAL (requestDccSend(const QString&)),
                   this,SLOT   (requestDccSend(const QString&)) );

  connect(&notifyTimer,SIGNAL(timeout()),
                  this,SLOT  (notifyTimeout()) );
  connect(&notifyCheckTimer,SIGNAL(timeout()),
                  this,SLOT  (notifyCheckTimeout()) );

  connect(&inputFilter,SIGNAL(welcome()),
                  this,SLOT  (connectionEstablished()) );
  connect(&inputFilter,SIGNAL(notifyResponse(const QString&)),
                  this,SLOT  (notifyResponse(const QString&)) );
  connect(&inputFilter,SIGNAL(addDccGet(const QString&, const QStringList&)),
                  this,SLOT  (addDccGet(const QString&, const QStringList&)) );
  connect(&inputFilter,SIGNAL(resumeDccGetTransfer(const QString&, const QStringList&)),
                  this,SLOT  (resumeDccGetTransfer(const QString&, const QStringList&)) );
  connect(&inputFilter,SIGNAL(resumeDccSendTransfer(const QString&, const QStringList&)),
                  this,SLOT  (resumeDccSendTransfer(const QString&, const QStringList&)) );
  connect(&inputFilter,SIGNAL(userhost(const QString&,const QString&,bool,bool)),
                  this,SLOT  (userhost(const QString&,const QString&,bool,bool)) );

  connect(this,SIGNAL(serverLag(int)),serverWindow,SLOT(updateLag(int)) );
  connect(this,SIGNAL(tooLongLag(int)),serverWindow,SLOT(tooLongLag(int)) );
  connect(this,SIGNAL(resetLag()),serverWindow,SLOT(resetLag()) );
  connect(this,SIGNAL(addDccPanel()),serverWindow,SLOT(addDccPanel()) );
  connect(this,SIGNAL(closeDccPanel()),serverWindow,SLOT(closeDccPanel()) );

  connect(&serverSocket,SIGNAL (connectionSuccess())  ,this,SLOT (ircServerConnectionSuccess()) );
  connect(&serverSocket,SIGNAL (connectionFailed(int)),this,SLOT (broken(int)) );
  connect(&serverSocket,SIGNAL (readyRead()),this,SLOT (incoming()) );
  connect(&serverSocket,SIGNAL (readyWrite()),this,SLOT (send()) );
  connect(&serverSocket,SIGNAL (closed(int)),this,SLOT (broken(int)) );
}

Server::~Server()
{
  kdDebug() << "Server::~Server()" << endl;

  // Make sure no signals get sent to a soon to be dying Server Window
  serverSocket.blockSignals(true);
  // Send out the last messages (usually the /QUIT)
  serverSocket.enableWrite(true);
  send();

  // notify KonversationApplication that this server is gone
  emit deleted(this);
  
  delete serverWindow;
}

QString Server::getServerName() { return serverName; }
int Server::getPort() { return serverPort; }

bool Server::getAutoJoin() { return autoJoin; }
void Server::setAutoJoin(bool on) { autoJoin=on; }

QString Server::getAutoJoinChannel() { return autoJoinChannel; }
void Server::setAutoJoinChannel(const QString &channel) { autoJoinChannel=channel; }

QString Server::getAutoJoinChannelKey() { return autoJoinChannelKey; }
void Server::setAutoJoinChannelKey(const QString &key) { autoJoinChannelKey=key; }

bool Server::isConnected() { return serverSocket.socketStatus()==KExtendedSocket::connected; }

void Server::connectToIRCServer()
{
  deliberateQuit=false;
  serverSocket.blockSignals(false);
  // Are we (still) connected (yet)?
  if(isConnected())
  {
    // just join our autojoin-channel if desired
    if (getAutoJoin()) queue(getAutoJoinCommand());
    // TODO: move autojoin here and use signals / slots
  }
  else
  {
    // (re)connect. Autojoin will be done by the input filter
    serverWindow->appendToStatus(i18n("Info"),i18n("Looking for server %1 ...").arg(serverSocket.host()));

    // QDns is broken, so don't use async lookup, use own threaded class instead
    resolver.setSocket(&serverSocket);
    resolver.start();
  }
}

bool Server::eventFilter(QObject* parent,QEvent* event)
{
  if(event->type()==QEvent::User)
  {
    lookupFinished();
    return true;
  }
  return QObject::eventFilter(parent,event);
}

void Server::lookupFinished()
{
  serverWindow->appendToStatus(i18n("Info"),i18n("Server found, connecting ..."));
  serverSocket.startAsyncConnect();
}

void Server::ircServerConnectionSuccess()
{
  reconnectCounter=0;

  connect(this,SIGNAL (nicknameChanged(const QString&)),serverWindow,SLOT (setNickname(const QString&)) );

  serverWindow->appendToStatus(i18n("Info"),i18n("Connected! Logging in ..."));

  QString connectString="USER " +
                        identity->getIdent() +
                        " 8 * :" +  // 8 = +i; 4 = +w
                        identity->getRealName();

  if (serverKey) queue("PASS "+serverKey);
  queue("NICK "+getNickname());
  queue(connectString);

  emit nicknameChanged(getNickname());

  serverSocket.enableRead(true);
}

void Server::broken(int state)
{
  serverSocket.enableRead(false);
  serverSocket.enableWrite(false);
  serverSocket.blockSignals(true);

  kdDebug() << "Connection broken (Socket fd " << serverSocket.fd() << ") " << state << "!" << endl;

  // TODO: Close all queries and channels!
  //       Or at least make sure that all gets reconnected properly

  if(autoReconnect && !getDeliberateQuit())
  {
    serverWindow->appendToStatus(i18n("Error"),i18n("Connection to Server %1 lost. Trying to reconnect.").arg(serverName));

    // TODO: Make retry counter configurable
    if(++reconnectCounter==10)
    {
      serverWindow->appendToStatus(i18n("Error"),i18n("Connection to Server %1 failed.").arg(serverName));
      reconnectCounter=0;
    }
    else
      // TODO: Make timeout configurable
      QTimer::singleShot(5000,this,SLOT(connectToIRCServer()));
  }
  else
  {
    serverWindow->appendToStatus(i18n("Error"),i18n("Connection to Server %1 closed.").arg(serverName));
  }
}

// Will be called from InputFilter as soon as the Welcome message was received
void Server::connectionEstablished()
{
  // get first notify very early
  startNotifyTimer(1000);
  // register with services
  if(!botPassword.isEmpty() && !bot.isEmpty())
    queue("PRIVMSG "+bot+" :identify "+botPassword);
}

void Server::notifyAction(QListViewItem* item)
{
  if(item)
  {
    // parse wildcards (toParse,nickname,channelName,nickList,queryName,parameter)
    QString out=parseWildcards(KonversationApplication::preferences.getNotifyDoubleClickAction(),
                               getNickname(),
                               QString::null,
                               QString::null,
                               item->text(0),
                               QString::null,
                               QString::null);
    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',out);
    for(unsigned int index=0;index<outList.count();index++)
    {
      outputFilter.parse(getNickname(),outList[index],QString::null);
      queue(outputFilter.getServerOutput());
    } // endfor
  }
}

void Server::notifyResponse(const QString& nicksOnline)
{
  // We received a 303 or "PONG :LAG" notify message, so calculate server lag
  int lag=notifySent.elapsed();
  emit serverLag(lag);
  // Stop check timer
  notifyCheckTimer.stop();

  // only update Nicks Online list if we got a 303 response, not a PONG
  if(nicksOnline!="###")
  {
    // First copy the old notify cache to a new cache, but all in lowercase
    QStringList notifyLowerCache=QStringList::split(' ',notifyCache.join(" ").lower());
    // Create a case correct nick list from the notification reply
    QStringList nickList=QStringList::split(' ',nicksOnline);
    // Create a lower case nick list from the notification reply
    QStringList nickLowerList=QStringList::split(' ',nicksOnline.lower());

    // Did some new nicks appear in our notify?
    for(unsigned int index=0;index<nickLowerList.count();index++)
    {
      if(notifyLowerCache.find(nickLowerList[index])==notifyLowerCache.end())
      {
        serverWindow->appendToFrontmost(i18n("Notify"),i18n("%1 is online.").arg(nickList[index]));
      }
    }

    // Did some nicks leave our notify?
    for(unsigned int index=0;index<notifyLowerCache.count();index++)
    {
      if(nickLowerList.find(notifyLowerCache[index])==nickLowerList.end())
      {
        serverWindow->appendToFrontmost(i18n("Notify"),i18n("%1 went offline.").arg(notifyCache[index]));
      }
    }

    // Finally copy the new ISON list with correct case to our notify cache
    notifyCache=nickList;
    emit nicksNowOnline(nickList);
  }
  // Next round
  startNotifyTimer();
}

void Server::startNotifyTimer(int msec)
{
  // make sure the timer gets started properly in case we have reconnected
  notifyTimer.stop();
  if(msec==0) msec=KonversationApplication::preferences.getNotifyDelay()*1000; // msec!
  // start the timer in one shot mode
  notifyTimer.start(msec,true);
  notifyCheckTimer.stop();
  // reset check time
  checkTime=0;
}

void Server::startNotifyCheckTimer()
{
  // start the timer in interval mode
  notifyCheckTimer.start(500);
}

void Server::notifyTimeout()
{
  bool sent=false;

  // Notify delay time is over, send ISON request if desired
  if(KonversationApplication::preferences.getUseNotify())
  {
    // But only if there actually are nicks in the notify list
    QString list=KonversationApplication::preferences.getNotifyString();
    if(!list.isEmpty())
    {
      queue("ISON "+list);
      // remember that we already sent out ISON
      sent=true;
    }
  }

  // if no ISON was sent, fall back to PING for lag measuring
  if(!sent) queue("PING LAG :"+getIrcName());
  // start check timer waiting for 303 or PONG response
  startNotifyCheckTimer();
}

void Server::notifyCheckTimeout()
{
  checkTime+=500;
  if(isConnected()) emit tooLongLag(checkTime);
}

QString Server::getAutoJoinCommand()
{
  // Multichannel joins
  QStringList channels=QStringList::split(' ',autoJoinChannel);
  QStringList keys=QStringList::split(' ',autoJoinChannelKey);
  
  QString autoString("JOIN "+channels.join(",")+" "+keys.join(","));

  return autoString;
}

QString Server::getNextNickname()
{
  QString newNick=getNickname();
  if(tryNickNumber!=4)
  {
    tryNickNumber++;
    if (tryNickNumber==4) newNick=getNickname()+"_";
    else newNick=identity->getNickname(tryNickNumber);
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

    getServerWindow()->appendToRaw(line);
    inputFilter.parseLine(line);
  }
}

void Server::incoming()
{
  char buffer[BUFFER_LEN];
  int len=0;

  len=read(serverSocket.fd(),buffer,BUFFER_LEN-1);

  buffer[len]=0;

  inputBuffer+=buffer;

  if(len==0) broken(0);
}

void Server::queue(const QString& buffer)
{
  // Only queue lines if we are connected
  if(isConnected() && buffer.length())
  {
    getServerWindow()->appendToRaw(buffer);
    outputBuffer+=buffer;
    outputBuffer+="\n";

    serverSocket.enableWrite(true);
  }
}

void Server::send()
{
  // Check if we are still online
  if(isConnected())
  {
    // To make lag calculation more precise, we reset the timer here
    if(outputBuffer.startsWith("ISON") ||
       outputBuffer.startsWith("PING LAG")) notifySent.start();
    
    // Don't reconnect if we WANT to quit
    else if(outputBuffer.startsWith("QUIT")) setDeliberateQuit(true);
    
    // TODO: Implement Flood-Protection here
    write(serverSocket.fd(),outputBuffer,outputBuffer.length());
    serverSocket.enableWrite(false);
  }

  outputBuffer=QString::null;
}

void Server::dcopSay(const QString& target,const QString& command)
{
  if(isAChannel(target))
  {
    Channel* channel=getChannelByName(target);
    if(channel) channel->sendChannelText(command);
  }
  else
  {
    Query* query=getQueryByName(target);
    if(query==0)
    {
      addQuery(target,QString::null);
      query=getQueryByName(target);
    }
    if(query) query->sendQueryText(command);
  }
}

void Server::dcopInfo(const QString& string)
{
  appendStatusMessage(i18n("DCOP"),string);
}

void Server::ctcpReply(const QString &receiver,const QString &text)
{
  queue("NOTICE "+receiver+" :"+'\x01'+text+'\x01');
}

bool Server::getDeliberateQuit()
{
  return deliberateQuit;
}

void Server::setDeliberateQuit(bool on)
{
  deliberateQuit=on;
}

void Server::addQuery(const QString &nickname,const QString &hostmask)
{
  // Only create new query object if there isn't already one with the same name
  Query* query=getQueryByName(nickname);
  if(!query)
  {
    query=new Query(serverWindow->getWindowContainer());
    query->setServer(this);
    query->setIdentity(getIdentity());
    query->setName(nickname);
    // Add new query pane to tabwidget
    serverWindow->addView(query,0,nickname);

    connect(query,SIGNAL (newText(QWidget*)),serverWindow,SLOT (newText(QWidget*)) );
    connect(query,SIGNAL (closed(Query*)),this,SLOT (removeQuery(Query*)) );
    connect(query,SIGNAL (sendFile(const QString&)),this,SLOT (requestDccSend(const QString &)) );
    // Append query to internal list
    queryList.append(query);
  }

  // try to get hostmask if there's none yet
  if(hostmask.isEmpty()) requestUserhost(nickname);

  // Always set hostmask
  if(query) query->setHostmask(hostmask);
}

void Server::closeQuery(const QString &name)
{
  Query* query=getQueryByName(name);
  removeQuery(query);
}

void Server::closeChannel(const QString &name)
{
  outputFilter.parse(getNickname(),KonversationApplication::preferences.getCommandChar()+"PART",name);
  queue(outputFilter.getServerOutput());
}

void Server::requestUserhost(const QString& nicks)
{
  inputFilter.setAutomaticRequest(true);
  queue("USERHOST "+nicks);
}

void Server::requestBan(const QStringList& users,const QString& channel,const QString& a_option)
{
  QString hostmask;
  QString banCommand;
  QString option=a_option.lower();

  Channel* targetChannel=getChannelByName(channel);

  for(unsigned int index=0;index<users.count();index++)
  {
    // first, set the ban mask to the specified nick
    QString mask=users[index];
    // did we specify an option?
    if(!option.isEmpty())
    {
      // try to find specified nick on the channel
      Nick* targetNick=targetChannel->getNickByName(mask);
      // if we found the nick try to find their hostmask
      if(targetNick)
      {
        QString hostmask=targetNick->getHostmask();
        // if we found the hostmask, add it to the ban mask
        if(!hostmask.isEmpty())
        {
          mask=targetNick->getNickname()+"!"+hostmask;

          // adapt ban mask to the option given
          if(option=="host")
            mask="*!*@*."+hostmask.section('.',1);
          else if(option=="domain")
            mask="*!*@"+hostmask.section('@',1);
          else if(option=="userhost")
            mask="*!"+hostmask.section('@',0,0)+"@*."+hostmask.section('.',1);
          else if(option=="userdomain")
            mask="*!"+hostmask.section('@',0,0)+"@"+hostmask.section('@',1);
        }
      }
    }

    outputFilter.execBan(mask,channel);

    banCommand=outputFilter.getServerOutput();
    
    queue(banCommand);
  }
}

void Server::requestDccSend()
{
  requestDccSend(QString::null);
}

void Server::requestDccSend(const QString &a_recipient)
{
  QString recipient(a_recipient);
  // if we don't have a recipient yet, let the user select one
  if(!recipient)
  {
    QStringList nickList;
    Channel* lookChannel=channelList.first();

    // fill nickList with all nicks we know about
    while(lookChannel)
    {
      QList<Nick> nicks=lookChannel->getNickList();
      Nick* lookNick=nicks.first();
      while(lookNick)
      {
        nickList.append(lookNick->getNickname());
        lookNick=nicks.next();
      }
      lookChannel=channelList.next();
    }

    // add Queries as well, but don't insert duplicates
    Query* lookQuery=queryList.first();
    while(lookQuery)
    {
      if(nickList.find(lookQuery->getName())==nickList.end()) nickList.append(lookQuery->getName());
      lookQuery=queryList.next();
    }

    recipient=DccRecipientDialog::getNickname(getServerWindow(),nickList);
  }
  // do we have a recipient *now*?
  if(recipient && !recipient.isEmpty())
  {
    QString fileName=KFileDialog::getOpenFileName(
                                                   lastDccDir,
                                                   QString::null,
                                                   getServerWindow(),
                                                   i18n("Select file to send to %1").arg(recipient)
                                                 );
    if(!fileName.isEmpty())
    {
      QFileInfo fileInfo(fileName);

      lastDccDir=fileInfo.dirPath();

      if(fileInfo.isDir())
        appendStatusMessage(i18n("DCC"),i18n("Error: \"%1\" is not a regular file.").arg(fileName));
      else
        addDccSend(recipient,fileName);
    }
  }
}

void Server::addDccSend(const QString &recipient,const QString &fileName)
{
  emit addDccPanel();

  // Get our own IP address. Don't laugh! This works!
  QString ip=KExtendedSocket::localAddress(serverSocket.fd())->pretty();
  ip=ip.section('-',0,0);

  // We already checked that the file exists in output filter / requestDccSend() resp.
  QFile file(fileName);
  QString size=QString::number(file.size());

//  kdDebug() << ip << endl;

  DccTransfer* newDcc=new DccTransfer(serverWindow->getDccPanel()->getListView(),
                  DccTransfer::Send,
                  KonversationApplication::preferences.getDccPath(),
                  recipient,
                  fileName,            // name
                  size,                // size (will be set by DccTransfer)
                  ip,                  // ip
                  0);                  // port (will be set by DccTransfer)

  connect(newDcc,SIGNAL (send(const QString&,const QString&,const QString&,const QString&,unsigned long)),
    this,SLOT (dccSendRequest(const QString&,const QString&,const QString&,const QString&,unsigned long)) );
  connect(newDcc,SIGNAL (dccSendDone(const QString&)),this,SLOT (dccSendDone(const QString&)) );
  newDcc->startSend();
}

void Server::addDccGet(const QString &sourceNick, const QStringList &dccArguments)
{
  emit addDccPanel();

  QHostAddress ip;

  ip.setAddress(dccArguments[1].toULong());

  appendStatusMessage(i18n("DCC"),
                      QString("%1 offers the file \"%2\" (%3 bytes) for download (%4:%5).")
                              .arg(sourceNick)               // name
                              .arg(dccArguments[0])          // file
                              .arg((dccArguments[3].isEmpty()) ? i18n("unknown") : dccArguments[3] )  // size
                              .arg(ip.toString())            // ip
                              .arg(dccArguments[2])          // port
                             );

  DccTransfer* newDcc=new DccTransfer(serverWindow->getDccPanel()->getListView(),
                  DccTransfer::Get,
                  KonversationApplication::preferences.getDccPath(),
                  sourceNick,
                  dccArguments[0],     // name
                  dccArguments[3],     // size
                  ip.toString(),       // ip
                  dccArguments[2]);    // port

  connect(newDcc,SIGNAL (resumeGet(const QString&,const QString&,const QString&,int)),this,
         SLOT (dccResumeGetRequest(const QString&,const QString&,const QString&,int)) );
  connect(newDcc,SIGNAL (dccGetDone(const QString&)),
              this,SLOT (dccGetDone(const QString&)) );

  if(KonversationApplication::preferences.getDccAutoGet()) newDcc->startGet();
}

void Server::requestDccPanel()
{
  emit addDccPanel();
}

void Server::requestCloseDccPanel()
{
  emit closeDccPanel();
}

void Server::dccSendRequest(const QString &partner, const QString &fileName, const QString &address, const QString &port, unsigned long size)
{
  kdDebug() << "Server::dccSendRequest()" << endl;
  outputFilter.sendRequest(partner,fileName,address,port,size);
  queue(outputFilter.getServerOutput());
  appendStatusMessage(outputFilter.getType(),outputFilter.getOutput());
}

void Server::dccResumeGetRequest(const QString &sender, const QString &fileName, const QString &port, int startAt)
{
  kdDebug() << "Server::dccResumeGetRequest()" << endl;
  outputFilter.resumeRequest(sender,fileName,port,startAt);
  queue(outputFilter.getServerOutput());
  appendStatusMessage(outputFilter.getType(),outputFilter.getOutput());
}

void Server::resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
  // Check if there actually is a transfer going on on that port
  DccTransfer* dccTransfer=serverWindow->getDccPanel()->getTransferByPort(dccArguments[1],DccTransfer::ResumeGet);
  if(!dccTransfer)
    // Check if there actually is a transfer going on with that name, could be behind a NAT
    // so the port number may get changed
    // mIRC substitutes this with "file.ext", so we have a problem here with mIRCs behind a NAT
    dccTransfer=serverWindow->getDccPanel()->getTransferByName(dccArguments[0],DccTransfer::ResumeGet);

  if(dccTransfer)
  {
    // overcome mIRCs brain-dead "file.ext" substitution
    QString fileName=dccTransfer->getFile();
    appendStatusMessage(i18n("DCC"),i18n("Resuming file \"%1\", offered by %2 from position %3.").arg(fileName).arg(sourceNick).arg(dccArguments[2]));
    dccTransfer->startResumeGet(dccArguments[2]);
  }
  else
  {
    appendStatusMessage(i18n("Error"),i18n("No DCC download running on port %1!").arg(dccArguments[1]));
  }
}

void Server::resumeDccSendTransfer(const QString &recipient, const QStringList &dccArguments)
{
  // Check if there actually is a transfer going on on that port
  DccTransfer* dccTransfer=serverWindow->getDccPanel()->getTransferByPort(dccArguments[1],DccTransfer::Send);
  if(!dccTransfer)
    // Check if there actually is a transfer going on with that name, could be behind a NAT
    // so the port number may get changed
    // mIRC substitutes this with "file.ext", so we have a problem here with mIRCs behind a NAT
    dccTransfer=serverWindow->getDccPanel()->getTransferByName(dccArguments[0],DccTransfer::Send);

  if(dccTransfer)
  {
    QString fileName=dccTransfer->getFile();
    appendStatusMessage(i18n("DCC"),i18n("Resuming file \"%1\", offered by %2 from position %3.").arg(fileName).arg(recipient).arg(dccArguments[2]));
    kdDebug() << "Sending ACCEPT" << endl;
    dccTransfer->startResumeSend(dccArguments[2]);
    outputFilter.acceptRequest(recipient,fileName,dccArguments[1],dccArguments[2].toUInt());
    queue(outputFilter.getServerOutput());
    appendStatusMessage(outputFilter.getType(),outputFilter.getOutput());
  }
  else
  {
    appendStatusMessage(i18n("Error"),i18n("No DCC upload running on port %1!").arg(dccArguments[1]));
  }
}

void Server::dccGetDone(const QString &fileName)
{
  appendStatusMessage(i18n("DCC"),i18n("DCC download of file \"%1\" finished.").arg(fileName));
}

void Server::dccSendDone(const QString &fileName)
{
  appendStatusMessage(i18n("DCC"),i18n("DCC upload of file \"%1\" finished.").arg(fileName));
}

QString Server::getNextQueryName()
{
  // Check if completion position is out of range
  if(completeQueryPosition>=queryList.count()) completeQueryPosition=0;
  // return the next query in the list (for /msg completion)
  if(queryList.count()) return queryList.at(completeQueryPosition++)->getName();

  return QString::null;
}

void Server::removeQuery(Query* query)
{
  // Remove query page from container
  serverWindow->getWindowContainer()->removePage(query);

  // Traverse through list to find the query
  Query* lookQuery=queryList.first();
  while(lookQuery)
  {
    // Did we find our query?
    if(lookQuery==query)
    {
      // Remove it from the query list
      queryList.remove(lookQuery);
      // break out of the loop
      lookQuery=0;
    }
    // else select next query
    else lookQuery=queryList.next();
  }
  // Free the query object
  delete query;
}

void Server::joinChannel(const QString &name, const QString &hostmask, const QString &/*key*/)
{
  // Make sure to delete stale Channel on rejoin.
  // FIXME: Hm ... Do we really have to? Wouldn't it be enough to just
  // keep the channel and set channel->setServer(server)?
  Channel* channel=getChannelByName(name);
  if(channel)
  {
    removeChannel(channel);
    delete channel;
  }

  channel=new Channel(serverWindow->getWindowContainer());

  channel->setServer(this);
  channel->setIdentity(getIdentity());
  channel->setName(name);
  channel->setNickname(getNickname());
  //channel->setKey(key);

  serverWindow->addView(channel,1,name);
  channel->joinNickname(getNickname(),hostmask);

  channelList.append(channel);
  connect(channel,SIGNAL (newText(QWidget*)),serverWindow,SLOT (newText(QWidget*)) );
  connect(channel,SIGNAL (prefsChanged()),serverWindow,SLOT (channelPrefsChanged()) );
  connect(channel,SIGNAL (sendFile()),this,SLOT (requestDccSend()) );
}

void Server::removeChannel(Channel* channel)
{
  channelList.removeRef(channel);
}

void Server::updateChannelMode(const QString &nick, const QString &channelName, char mode, bool plus, const QString &parameter)
{
  Channel* channel=getChannelByName(channelName);
  if(channel) channel->updateMode(nick,mode,plus,parameter);
}

void Server::updateChannelModeWidgets(const QString &channelName, char mode, const QString &parameter)
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

// TODO: Maybe use a Signal / Slot mechanism for these things?
void Server::updateFonts()
{
  kdDebug() << "Server::updateFonts()" << endl;

  statusPanel->updateFonts();
  
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->updateFonts();
    channel=channelList.next();
  }

  Query* query=queryList.first();
  while(query)
  {
    query->updateFonts();
    query=queryList.next();
  }
  
  // TODO: To be revised. Why should the server update the serverWindow?
  //       This must be done with signals / slots ASAP
  if(serverWindow)
  {
    serverWindow->updateFonts();
    emit repaintTabs();
  }
}

// TODO: Maybe use a Signal / Slot mechanism for these things?
void Server::setShowQuickButtons(bool state)
{
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->showQuickButtons(state);
    channel=channelList.next();
  }
}

// TODO: Maybe use a Signal / Slot mechanism for these things?
void Server::setShowModeButtons(bool state)
{
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->showModeButtons(state);
    channel=channelList.next();
  }
}

Channel* Server::getChannelByName(const char* name)
{
  // Convert wanted channel name to lowercase
  QString wanted=name;
  wanted=wanted.lower();

  // Traverse through list to find the channel named "name"
  Channel* lookChannel=channelList.first();
  while(lookChannel)
  {
    if(lookChannel->getName().lower()==wanted) return lookChannel;
    lookChannel=channelList.next();
  }
  // No channel by that name found? Return 0. Happens on first channel join
  return 0;
}

Query* Server::getQueryByName(const char* name)
{
  // Convert wanted query name to lowercase
  QString wanted=name;
  wanted=wanted.lower();

  // Traverse through list to find the query with "name"
  Query* lookQuery=queryList.first();
  while(lookQuery)
  {
    if(lookQuery->getName().lower()==wanted) return lookQuery;
    lookQuery=queryList.next();
  }
  // No query by that name found? Must be a new query request. Return 0
  return 0;
}

void Server::addNickToChannel(const QString &channelName, const QString &nickname, const QString &hostmask, bool op, bool voice)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->addNickname(nickname,hostmask,op,voice);
}

void Server::nickJoinsChannel(const QString &channelName, const QString &nickname, const QString &hostmask)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->joinNickname(nickname,hostmask);
}

void Server::addHostmaskToNick(const QString& sourceNick, const QString& sourceHostmask)
{
  Channel* channel=channelList.first();

  while(channel)
  {
    Nick* nick=channel->getNickByName(sourceNick);
    if(nick) nick->setHostmask(sourceHostmask);
    channel=channelList.next();
  }
  // Set hostmask for query with the same name
  Query* query=getQueryByName(sourceNick);
  if(query) query->setHostmask(sourceHostmask);
}

void Server::removeNickFromChannel(const QString &channelName, const QString &nickname, const QString &reason, bool quit)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel)
  {
    if(outChannel->getNickByName(nickname)) outChannel->removeNick(nickname,reason,quit);
  }
}

void Server::nickWasKickedFromChannel(const QString &channelName, const QString &nickname, const QString &kicker, const QString &reason)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel)
  {
    if(outChannel->getNickByName(nickname)) outChannel->kickNick(nickname,kicker,reason);
  }
}

void Server::removeNickFromServer(const QString &nickname,const QString &reason)
{
  Channel* channel=channelList.first();
  while(channel)
  {
    removeNickFromChannel(channel->getName(),nickname,reason,true);
    channel=channelList.next();
  }
}

void Server::renameNick(const QString &nickname, const QString &newNick)
{
  // Rename the nick in every channel they are in
  Channel* channel=channelList.first();
  while(channel)
  {
    if(channel->getNickByName(nickname)) channel->renameNick(nickname,newNick);
    channel=channelList.next();
  }
  // If this was our own nickchange, tell our server object about it
  if(nickname==getNickname()) setNickname(newNick);
  // If we had a query with this nick, change that name, too
  Query* query=queryList.first();
  while(query)
  {
    if(query->getName().lower()==nickname.lower())
    {
      query->setName(newNick);
      serverWindow->getWindowContainer()->changeTab(query,newNick);
    }
    query=queryList.next();
  }
}

void Server::userhost(const QString& nick,const QString& hostmask,bool /* away */ ,bool /* ircOp */)
{
  addHostmaskToNick(nick,hostmask);
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
  else kdWarning() << "Server::appendToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendActionToQuery(const char* queryName,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendAction(queryName,message);
  else kdWarning() << "Server::appendActionToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendServerMessageToQuery(const char* queryName,const char* type,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendServerMessage(type,message);
  else kdWarning() << "Server::appendServerMessageToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendCommandMessageToQuery(const char* queryName,const char* command,const char* message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendCommandMessage(command,message);
  else kdWarning() << "Server::appendCommandMessageToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendStatusMessage(const char* type,const char* message)
{
  serverWindow->appendToFrontmost(type,message);
}

void Server::setNickname(const QString &newNickname)
{
  nickname=newNickname;
  serverWindow->setNickname(newNickname);
}

void Server::setChannelTopic(const QString &channel, const QString &newTopic)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel)
  {
    QTextCodec* codec=QTextCodec::codecForName(KonversationApplication::preferences.getCodec());
    QString topic=codec->toUnicode(newTopic);
    outChannel->setTopic(topic);
  }
}

void Server::setChannelTopic(const QString& nickname, const QString &channel, const QString &newTopic) // Overloaded
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel)
  {
    QTextCodec* codec=QTextCodec::codecForName(KonversationApplication::preferences.getCodec());
    QString topic=codec->toUnicode(newTopic);
    outChannel->setTopic(nickname,newTopic);
  }
}

bool Server::isNickname(const QString &compare)
{
  return (nickname==compare);
}

QString Server::getNickname()
{
  return nickname;
}

QString Server::parseWildcards(const QString &toParse, const QString &nickname, const QString &channelName, const QString &channelKey, const QString &nick, const QString &queryName, const QString &parameter)
{
  return parseWildcards(toParse,nickname,channelName,channelKey,QStringList::split(' ',nick),queryName,parameter);
}

QString Server::parseWildcards(const QString &toParse, const QString &nickname, const QString &channelName, const QString &channelKey, const QStringList &nickList, const QString &queryName, const QString &/*parameter*/)
{
  // TODO: parameter handling.
  //       since parameters are not functional yet

  // cut button name from definition
  QString out(toParse.mid(toParse.find(',')+1));
  // define default separator and regular expression for definition
  QString separator(" ");
  QRegExp separatorRegExp("%s[^%]*%");

  int pos;
  // separator definition found?
  pos=out.find(separatorRegExp);
  if(pos!=-1)
  {
    // skip "%s" at the beginning
    pos+=2;
    // copy out all text to the next "%" as new separator
    separator=out.mid(pos,out.find("%",pos+1)-pos);
    // remove separator definition from string
    out.replace(separatorRegExp, QString::null);
  }

  kdDebug() << "Replacing placeholders in: " << out << endl;
  out.replace(QRegExp("%u"),nickList.join(separator));
  if(channelName) out.replace(QRegExp("%c"),channelName);
  out.replace(QRegExp("%o"),nickname);
  out.replace(QRegExp("%k"),channelKey);
  out.replace(QRegExp("%K"),serverKey);
  out.replace(QRegExp("%n"),"\n");
//  out.replace(QRegExp("%f"),getFortuneCookie());
//  out.replace(QRegExp("%p"),parameter);
  if(queryName) out.replace(QRegExp("%q"),queryName);

  // finally replace all "%p" with "%"
  out.replace(QRegExp("%p"),"%");

  return out;
}

void Server::setIrcName(const QString &newIrcName)
{
  ircName=newIrcName;
}

QString Server::getIrcName()
{
  return ircName;
}

OutputFilter& Server::getOutputFilter()
{
  return outputFilter;
}

ServerWindow* Server::getServerWindow()
{
  return serverWindow;
}

void Server::setServerWindow(ServerWindow* newWindow)
{
  serverWindow=newWindow;
}

void Server::away()
{
  isAway=true;
}

void Server::sendToAllChannels(const QString &text)
{
  // Send a message to all channels we are in
  Channel* channel=channelList.first();

  while(channel)
  {
    channel->sendChannelText(text);
    channel=channelList.next();
  }
}

void Server::scriptNotFound(const QString& name)
{
  appendStatusMessage(i18n("DCOP"),i18n("Error: Could not find script \"%1\".").arg(name));
}

void Server::scriptExecutionError(const QString& name)
{
  appendStatusMessage(i18n("DCOP"),i18n("Error: Could not execute script \"%1\". Check file permissions.").arg(name));
}

void Server::unAway()
{
  isAway=false;
}

bool Server::isAChannel(const QString &check)
{
  QChar initial=check.at(0);

  return (initial=='#' || initial=='&' || initial=='+' || initial=='!');
}

void Server::addRawLog()
{
  getServerWindow()->addRawLog();
}

void Server::closeRawLog()
{
  getServerWindow()->closeRawLog();
}

void Server::setIdentity(const Identity *newIdentity) { identity=newIdentity; }
const Identity *Server::getIdentity() { return identity; }

#include "server.moc"
