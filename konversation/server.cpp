/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#ifdef __linux__
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)
typedef unsigned long long __u64;
#endif
#endif

#include <netinet/in.h>
#include <stdlib.h>

#include <qregexp.h>
#include <qhostaddress.h>
#include <qtextcodec.h>
#include <qdatetime.h>

#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kresolver.h>
#include <ksocketdevice.h>
#include <kaction.h>
using namespace KNetwork;
#include <kstringhandler.h>
#include <kdeversion.h>
#include <kwin.h>

#include "server.h"
#include "query.h"
#include "channel.h"
#include "konversationapplication.h"
#include "dccpanel.h"
#include "dcctransfer.h"
#include "dcctransfersend.h"
#include "dcctransferrecv.h"
#include "dccrecipientdialog.h"
#include "nick.h"
#include "irccharsets.h"
#include "konversationmainwindow.h"
#include "statuspanel.h"
#include "rawlog.h"
#include "channellistpanel.h"
#include "scriptlauncher.h"
#include "konvidebug.h"
#include "servergroupsettings.h"
#include "addressbook.h"
#include "serverison.h"
#include "common.h"

#include <config.h>

#ifdef USE_KNOTIFY
#include <knotifyclient.h>
#endif


Server::Server(KonversationMainWindow* mainWindow, int id)
{
  m_serverGroup = KonversationApplication::preferences.serverGroupById(id);
  bot = getIdentity()->getBot();
  botPassword = getIdentity()->getPassword();

  init(mainWindow, getIdentity()->getNickname(0),"");
}

Server::Server(KonversationMainWindow* mainWindow,const QString& hostName,const QString& port,
	       const QString& channel,const QString& password, QString nick,const bool& useSSL)
{
  m_serverGroup.setName(hostName);
  m_serverGroup.setIdentityId(KonversationApplication::preferences.getIdentityByName("Default")->id());

  Konversation::ServerSettings serverSettings;
  serverSettings.setServer(hostName);
  serverSettings.setPort(port.toInt());
  serverSettings.setPassword(password);
  serverSettings.setSSLEnabled(useSSL);
  m_serverGroup.addServer(serverSettings);

  if(nick.isEmpty()) // Happens when we are invoked from an irc:/ url
    nick = getIdentity()->getNickname(0);

  init(mainWindow, nick, channel);
}

void Server::doPreShellCommand() {

    QString command = KonversationApplication::preferences.getPreShellCommand();
    statusView->appendServerMessage("Info","Running preconfigured command...");

    connect( &preShellCommand,SIGNAL(processExited(KProcess*)),this,SLOT(preShellCommandExited(KProcess*)));

    QStringList commandList = QStringList::split(" ",command);

    for ( QStringList::Iterator it = commandList.begin(); it != commandList.end(); ++it ) {
      preShellCommand << *it;
    }

    preShellCommand.start(); // Non blocking
}

Server::~Server()
{
  kdDebug() << "Server::~Server(" << getServerName() << ")" << endl;

  // Delete helper object.
  delete m_serverISON;
  m_serverISON = 0;
  // clear nicks online
  emit nicksNowOnline(this,QStringList(),true);
  // Make sure no signals get sent to a soon to be dying Server Window
  m_socket->blockSignals(true);

  // Close socket but don't delete it. QObject will take care of delete
  if(!m_serverGroup.serverByIndex(m_currentServerIndex).SSLEnabled())
    m_socket->close();
  
  // For SSL socket we autoclose socket when the Server object is deleted
  
  // Send out the last messages (usually the /QUIT)
  send();

#ifdef USE_MDI
/*
  while(channelList.count())
  {
    Channel* lookChannel=channelList.at(0);
    lookChannel->removeNick(getNickname(),getIdentity()->getPartReason(),true);
  }
  kdDebug() << "Server::~Server(): channel list count now: " << channelList.count() << endl;

  while(queryList.count())
  {
    queryList.at(0)->closeYourself();
  }
*/

  emit serverQuit(getIdentity()->getPartReason());
#else
  closeRawLog();
  closeChannelListPanel();

  channelList.setAutoDelete(true);
  channelList.clear();

  queryList.setAutoDelete(true);
  queryList.clear();
#endif

  // Delete all the NickInfos and ChannelNick structures.
  m_allNicks.clear();
  ChannelMembershipMap::Iterator it;
  for ( it = m_joinedChannels.begin(); it != m_joinedChannels.end(); ++it ) delete it.data();
  m_joinedChannels.clear();
  for ( it = m_unjoinedChannels.begin(); it != m_unjoinedChannels.end(); ++it ) delete it.data();
  m_unjoinedChannels.clear();
  m_queryNicks.clear();

  // notify KonversationApplication that this server is gone
  emit deleted(this);
}

void Server::init(KonversationMainWindow* mainWindow, const QString& nick, const QString& channel)
{
  setName(QString("server_" + m_serverGroup.name()).ascii());

  m_currentServerIndex = 0;
  m_tryReconnect=true;
  autoJoin = false;
  tryNickNumber = 0;
  checkTime = 0;
  reconnectCounter = 0;
  currentLag = 0;
  rawLog = 0;
  channelListPanel = 0;
  alreadyConnected = false;
  rejoinChannels = false;
  connecting = false;
  m_serverISON = 0;
  lastDccDir = QString::null;
  m_isAway = false;
  m_isAutoAway = false;
  m_socket = 0;

  // TODO fold these into a QMAP, and these need to be reset to RFC values if this server object is reused.
  serverNickPrefixModes = "ov";
  serverNickPrefixes = "@+";
  channelPrefixes = "#&";

  timerInterval = 1;  // flood protection
  autoRejoin = KonversationApplication::preferences.getAutoRejoin();
  autoReconnect = KonversationApplication::preferences.getAutoReconnect();

  setMainWindow(mainWindow);
  statusView = getMainWindow()->addStatusView(this);
  setNickname(nick);
  obtainNickInfo(getNickname());

  if(KonversationApplication::preferences.getRawLog())
  {
    addRawLog(false);
  }

  inputFilter.setServer(this);
  outputFilter = new Konversation::OutputFilter(this);
  m_scriptLauncher = new ScriptLauncher(this);

  // don't delete items when they are removed
  channelList.setAutoDelete(false);
  // For /msg query completion
  completeQueryPosition=0;

  Konversation::ChannelList tmpList = m_serverGroup.channelList();

  if(!channel.isEmpty())
    tmpList.push_front(channel);

  if(!tmpList.isEmpty()) {
    setAutoJoin(true);
    Konversation::ChannelList::iterator it;
    QString channels;
    QString keys;

    for(it = tmpList.begin(); it != tmpList.end(); ++it) {
      if(it != tmpList.begin()) {
        channels += ',';
        keys += ',';
      }

      channels += (*it).name();
      keys += (*it).password();
    }

    setAutoJoinChannel(channels);
    setAutoJoinChannelKey(keys);
  } else {
    setAutoJoin(false);
  }

  if(!m_serverGroup.connectCommands().isEmpty())
  {
    connectCommands = QStringList::split(";", m_serverGroup.connectCommands());
  }

  if(!KonversationApplication::preferences.getPreShellCommand().isEmpty()) {
    doPreShellCommand();
  } else {
    connectToIRCServer();
  }

  initTimers();

  if(KonversationApplication::preferences.getPreShellCommand().isEmpty())
    connectSignals();

  emit serverOnline(false);
}

void Server::initTimers()
{
  notifyTimer.setName("notify_timer");

  incomingTimer.setName("incoming_timer");

  outgoingTimer.setName("outgoing_timer");
  outgoingTimer.start(timerInterval);
}


void Server::connectSignals()
{
  connect(&incomingTimer,SIGNAL(timeout()),
                    this,SLOT  (processIncomingData()) );

  connect(&outgoingTimer,SIGNAL(timeout()),
                    this,SLOT  (send()) );

  connect(&unlockTimer,SIGNAL(timeout()),
                  this,SLOT  (unlockSending()) );

  connect(outputFilter,SIGNAL (requestDccSend()),
                   this,SLOT   (requestDccSend()) );
  connect(outputFilter,SIGNAL (requestDccSend(const QString&)),
                   this,SLOT   (requestDccSend(const QString&)) );
  connect(outputFilter, SIGNAL(multiServerCommand(const QString&, const QString&)),
    this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
  connect(outputFilter, SIGNAL(reconnectServer()), this, SLOT(reconnect()));

  connect(outputFilter,SIGNAL (openDccPanel()),
            this,SLOT   (requestDccPanel()) );
  connect(outputFilter,SIGNAL (closeDccPanel()),
            this,SLOT   (requestCloseDccPanel()) );
  connect(outputFilter,SIGNAL (openDccSend(const QString &, KURL)),
            this,SLOT   (addDccSend(const QString &, KURL)) );
  connect(outputFilter,SIGNAL (requestDccChat(const QString &)),
            this,SLOT   (requestDccChat(const QString &)) );
  connect(outputFilter,SIGNAL (connectToServer(const QString&, const QString&, const QString&)),
            this,SLOT   (connectToNewServer(const QString&, const QString&, const QString&)));

  connect(outputFilter,SIGNAL (openKonsolePanel()),
            this,SLOT   (requestKonsolePanel()) );

  connect(outputFilter,SIGNAL (sendToAllChannels(const QString&)),
            this,SLOT   (sendToAllChannels(const QString&)) );
  connect(outputFilter,SIGNAL (banUsers(const QStringList&,const QString&,const QString&)),
            this,SLOT   (requestBan(const QStringList&,const QString&,const QString&)) );
  connect(outputFilter,SIGNAL (unbanUsers(const QString&,const QString&)),
            this,SLOT   (requestUnban(const QString&,const QString&)) );

  connect(outputFilter,SIGNAL (openRawLog(bool)), this,SLOT (addRawLog(bool)) );
  connect(outputFilter,SIGNAL (closeRawLog()),this,SLOT (closeRawLog()) );

  connect(&notifyTimer,SIGNAL(timeout()),
                  this,SLOT  (notifyTimeout()) );
  connect(&notifyCheckTimer,SIGNAL(timeout()),
                  this,SLOT  (notifyCheckTimeout()) );

  connect(&inputFilter,SIGNAL(welcome(const QString&)),
                  this,SLOT  (connectionEstablished(const QString&)) );
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
  connect(&inputFilter,SIGNAL(topicAuthor(const QString&,const QString&)),
                  this,SLOT  (setTopicAuthor(const QString&,const QString&)) );
  connect(&inputFilter,SIGNAL(endOfWho(const QString&)),
                  this,SLOT  (endOfWho(const QString&)) );
  connect(&inputFilter,SIGNAL(invitation(const QString&,const QString&)),
                  this,SLOT  (invitation(const QString&,const QString&)) );
  connect(&inputFilter, SIGNAL(addToChannelList(const QString&, int, const QString& )),
    this, SLOT(addToChannelList(const QString&, int, const QString& )));

  connect(&inputFilter,SIGNAL (away()),this,SLOT (away()) );
  connect(&inputFilter,SIGNAL (unAway()),this,SLOT (unAway()) );
  connect(&inputFilter,SIGNAL (addDccChat(const QString&,const QString&,const QString&,const QStringList&,bool)),
         getMainWindow(),SLOT (addDccChat(const QString&,const QString&,const QString&,const QStringList&,bool)) );

  connect(this,SIGNAL(serverLag(Server*,int)),getMainWindow(),SLOT(updateLag(Server*,int)) );
  connect(this,SIGNAL(tooLongLag(Server*,int)),getMainWindow(),SLOT(tooLongLag(Server*,int)) );
  connect(this,SIGNAL(resetLag()),getMainWindow(),SLOT(resetLag()) );
  connect(this,SIGNAL(addDccPanel()),getMainWindow(),SLOT(addDccPanel()) );
  connect(this,SIGNAL(addKonsolePanel()),getMainWindow(),SLOT(addKonsolePanel()) );

  // Don't uncomment this.  You'll create an infinite loop.  The current sequence is:
  // signal PrefDialog::prefsChanged -> KonversationApplication::saveOptions ->
  // signal KonversationApplication::prefsChanged -> KonversationMainWindow::slotPrefsChanged ->
  // signal KonversationMainWindow::prefsChanged -> rest of program.
//  connect(getMainWindow(),SIGNAL(prefsChanged()),KonversationApplication::kApplication(),SLOT(saveOptions()));

  connect(this,SIGNAL (serverOnline(bool)),statusView,SLOT (serverOnline(bool)) );

  connect(outputFilter, SIGNAL(launchScript(const QString&, const QString&)),
    m_scriptLauncher, SLOT(launchScript(const QString&, const QString&)));

  connect(m_scriptLauncher, SIGNAL(scriptNotFound(const QString&)),
                      this, SLOT(scriptNotFound(const QString&)));
  connect(m_scriptLauncher, SIGNAL(scriptExecutionError(const QString&)),
                      this, SLOT(scriptExecutionError(const QString&)));

}

QString Server::getServerName()  const { return m_serverGroup.serverByIndex(m_currentServerIndex).server(); }
int Server::getPort() const { return m_serverGroup.serverByIndex(m_currentServerIndex).port(); }

QString Server::getServerGroup() const { return m_serverGroup.name(); }

int Server::getLag()  const { return currentLag; }

bool Server::getAutoJoin()  const { return autoJoin; }
void Server::setAutoJoin(bool on) { autoJoin=on; }

QString Server::getAutoJoinChannel() const { return autoJoinChannel; }
void Server::setAutoJoinChannel(const QString &channel) { autoJoinChannel=channel; }

QString Server::getAutoJoinChannelKey() const { return autoJoinChannelKey; }
void Server::setAutoJoinChannelKey(const QString &key) { autoJoinChannelKey=key; }

bool Server::isConnected()  const { 
  if(!m_socket) {
    return false;
  }

  return m_socket->state() == KNetwork::KClientSocketBase::Connected;
}

bool Server::isConnecting() const { return connecting; }

void Server::preShellCommandExited(KProcess* proc)
{
  if (proc->normalExit())
    statusView->appendServerMessage("Info","Process executed successfully!");
  else
    statusView->appendServerMessage("Warning","There was a problem while executing the command!");

  connectToIRCServer();
  connectSignals();
}

void Server::connectToIRCServer()
{
  outputBuffer.clear();
  deliberateQuit=false;
  
  if(m_socket) {
    m_socket->blockSignals(false);
  }

  connecting=true;

  // prevent sending queue until server has sent something or the timeout is through
  lockSending();

  // Are we (still) connected (yet)?
  if(isConnected())
  {
    // just join our autojoin-channel if desired
    if (getAutoJoin() && !rejoinChannels) queue(getAutoJoinCommand());
    // TODO: move autojoin here and use signals / slots
  }
  else
  {
    // This is needed to support server groups with mixed SSL and nonSSL servers
    delete m_socket;

    // connect() will do a async lookup too
    if(!m_serverGroup.serverByIndex(m_currentServerIndex).SSLEnabled()) {
      m_socket = new KNetwork::KBufferedSocket(QString::null, QString::null, this, "serverSocket");
      connect(m_socket,SIGNAL (connected(const KResolverEntry&)),this,SLOT (ircServerConnectionSuccess()));
    } else {
      m_socket = new SSLSocket(mainWindow, this, "serverSSLSocket");
      connect(m_socket,SIGNAL (sslInitDone()),this,SLOT (ircServerConnectionSuccess()));
      connect(m_socket,SIGNAL (sslFailure(QString)),this,SIGNAL(sslInitFailure()));
      connect(m_socket,SIGNAL (sslFailure(QString)),this,SLOT(sslError(QString)));
    }

    connect(m_socket,SIGNAL (hostFound()),this,SLOT(lookupFinished()));
    connect(m_socket,SIGNAL (gotError(int)),this,SLOT (broken(int)) );
    connect(m_socket,SIGNAL (readyRead()),this,SLOT (incoming()) );
    connect(m_socket,SIGNAL (readyWrite()),this,SLOT (send()) );
    connect(m_socket,SIGNAL (closed()),this,SLOT(closed()));

    m_socket->connect(m_serverGroup.serverByIndex(m_currentServerIndex).server(),
                      QString::number(m_serverGroup.serverByIndex(m_currentServerIndex).port()));

    // set up the connection details
    setPrefixes("ov","@+");
    statusView->appendServerMessage(i18n("Info"),i18n("Looking for server %1:%2...")
        .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server())
        .arg(m_serverGroup.serverByIndex(m_currentServerIndex).port()));
    
    // reset InputFilter (auto request info, /WHO request info)
    inputFilter.reset();
  }
}

void Server::showSSLDialog()
{
  static_cast<SSLSocket*>(m_socket)->showInfoDialog();
}

// set available channel types according to 005 RPL_ISUPPORT
void Server::setChannelTypes(const QString &pre)
{
  channelPrefixes=pre;
}

QString Server::getChannelTypes()
{
  return channelPrefixes;
}

// set user mode prefixes according to non-standard 005-Reply (see inputfilter.cpp)
void Server::setPrefixes(const QString &modes, const QString& prefixes)
{
  // NOTE: serverModes is QString::null, if server did not supply the
  // modes which relates to the network's nick-prefixes
  serverNickPrefixModes=modes;
  serverNickPrefixes=prefixes;
}

// return a nickname without possible mode character at the beginning
void Server::mangleNicknameWithModes(QString& nickname,bool& isAdmin,bool& isOwner,
                                     bool& isOp,bool& isHalfop,bool& hasVoice)
{
  isAdmin=false;
  isOwner=false;
  isOp=false;
  isHalfop=false;
  hasVoice=false;
  int modeIndex;
  Q_ASSERT(!nickname.isEmpty()); if(nickname.isEmpty()) return;
  while( (modeIndex=serverNickPrefixes.find(nickname[0])) != -1) {
    Q_ASSERT(!nickname.isEmpty()); if(nickname.isEmpty()) return;
    nickname=nickname.mid(1);
    // cut off the prefix
    bool recognisedMode=false;
    // determine, whether status is like op or like voice
    while((modeIndex)<int(serverNickPrefixes.length()) && !recognisedMode)
    {

      switch(serverNickPrefixes[modeIndex].latin1())
      {
        case '*':  // admin (EUIRC)
        {
            isAdmin=true;
	    recognisedMode=true;
	    break;
        }
        case '!':  // channel owner (RFC2811)
        {
          isOwner=true;
	  recognisedMode=true;
	  break;
        }
        case '@':  // channel operator (RFC1459)
        {
          isOp=true;
	  recognisedMode=true;
	  break;
        }
        case '%':  // halfop
        {
          isHalfop=true;
	  recognisedMode=true;
	  break;
        }
        case '+':  // voiced (RFC1459)
        {
          hasVoice=true;
	  recognisedMode=true;
	  break;
        }
        default:
        {
          modeIndex++;
          break;
        }
      } //switch to recognise the mode.
    } // loop through the modes to find one recognised
  } // loop through the name
}

/**
	When serverSocket emits hostFound() signal this slot is called.

 */
void Server::lookupFinished()
{
  // error during lookup
  if(m_serverGroup.serverByIndex(m_currentServerIndex).SSLEnabled() && m_socket->status())
  {
    // inform user about the error
    statusView->appendServerMessage(i18n("Error"),i18n("Server %1 not found.  %2")
        .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server())
        .arg(m_socket->errorString(m_socket->error())));

    m_socket->resetStatus();
    // prevent retrying to connect
    m_tryReconnect = false;
    // broken connection
    broken(m_socket->error());
  } else {
    statusView->appendServerMessage(i18n("Info"),i18n("Server found, connecting..."));
  }
}

void Server::ircServerConnectionSuccess()
{
  reconnectCounter=0;

  connect(this,SIGNAL (nicknameChanged(const QString&)),statusView,SLOT (setNickname(const QString&)) );
  statusView->appendServerMessage(i18n("Info"),i18n("Connected; logging in..."));

  QString connectString="USER " +
      getIdentity()->getIdent() +
      " 8 * :" +  // 8 = +i; 4 = +w
      getIdentity()->getRealName();

  if(!m_serverGroup.serverByIndex(m_currentServerIndex).password().isEmpty()) {
    queueAt(0, "PASS " + m_serverGroup.serverByIndex(m_currentServerIndex).password());
  }

  queueAt(1,"NICK "+getNickname());
  queueAt(2,connectString);

  QStringList::iterator iter;
  for(iter = connectCommands.begin(); iter != connectCommands.end(); ++iter)
  {
    QString output(*iter);
    /*if(output.startsWith("/"))
    {
      output.remove(0, 1);
    }*/
    Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),output,QString::null);
    queue(result.toServer);
  }

  emit nicknameChanged(getNickname());

  m_socket->enableRead(true);

  // wait at most 2 seconds for server to send something before sending the queue ourselves
  unlockTimer.start(2000);
}

void Server::broken(int state)
{
  m_socket->enableRead(false);
  m_socket->enableWrite(false);
  m_socket->blockSignals(true);

  alreadyConnected=false;
  connecting=false;
  outputBuffer.clear();

  notifyTimer.stop();
  notifyCheckTimer.stop();
  inputFilter.setLagMeasuring(false); // XXX paranoia?
  currentLag = -1; // XXX will this make it server independent now?
  emit resetLag();

  kdDebug() << "Connection broken (Socket fd " << m_socket->socketDevice()->socket() << ") "
    << state << "!" << endl;


  // clear nicks online
  emit nicksNowOnline(this,QStringList(),true);

  
  // TODO: Close all queries and channels!
  //       Or at least make sure that all gets reconnected properly
  if(autoReconnect && !getDeliberateQuit())
  {
    // TODO: Make retry counter configurable
    reconnectCounter++;
    bool cancelReconnect = false;
    if(state != KNetwork::KSocketBase::LookupFailure &&
       state != KNetwork::KSocketBase::ConnectionTimedOut &&
       state != KNetwork::KSocketBase::NetFailure &&
       state != KNetwork::KSocketBase::Timeout &&
       state != KNetwork::KSocketBase::UnknownError) cancelReconnect = true;

    if(cancelReconnect || reconnectCounter >= 10 || !m_tryReconnect)
    {
      QString error = i18n("Connection to Server %1 failed.  %2")
          .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server())
	  .arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));

      statusView->appendServerMessage(i18n("Error"),error);
//  Uncomment below if you want the server error message to be in the current window.
//      getMainWindow()->appendToFrontmostIfDifferent(i18n("Error"),error, statusView);
      reconnectCounter = 0;
      rejoinChannels = false;

      if(m_currentServerIndex < (m_serverGroup.serverList().count() - 1)) {
        m_currentServerIndex++;
	error = i18n("Trying server %1 instead.")
            .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server());
        statusView->appendServerMessage(i18n("Error"),error );

//  Uncomment below if you want the server error message to be in the current window.
//        getMainWindow()->appendToFrontmostIfDifferent(i18n("Error"),error,statusView);

        connectToIRCServer();
      } else {
      }
    }
    else
    {
      QString error = i18n("Connection to Server %1 lost.  %2.  Trying to reconnect.")
	                .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server())
			.arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));
      
      statusView->appendServerMessage(i18n("Error"), error);

//  Uncomment below if you want the server error message to be in the current window.
//      getMainWindow()->appendToFrontmostIfDifferent(i18n("Error"),error,statusView);

      // TODO: Make timeout configurable
      QTimer::singleShot(5000,this,SLOT(connectToIRCServer()));
      rejoinChannels = true;
    }
  }
  else if(getDeliberateQuit()) // If we quit the connection with the server
  {
    getMainWindow()->serverQuit(this);
  }
  else
  {
    QString error = i18n("Connection to Server %1 failed.")
	              .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server());
    statusView->appendServerMessage(i18n("Error"),error);

//  Uncomment below if you want the server error message to be in the current window.
//    getMainWindow()->appendToFrontmostIfDifferent(i18n("Error"),error, statusView);
  }

  emit serverOnline(false);
}

void Server::sslError(QString reason)
{
  QString error = i18n("Could not connect to %1:%2 using SSL encryption.  Maybe the server does not support SSL, or perhaps you have the wrong port? %3")
	             .arg(m_serverGroup.serverByIndex(m_currentServerIndex).server())
		     .arg(m_serverGroup.serverByIndex(m_currentServerIndex).port())
		     .arg(reason);
  statusView->appendServerMessage(i18n("SSL Connection Error"),error);
  m_tryReconnect=false;

}

// Will be called from InputFilter as soon as the Welcome message was received
void Server::connectionEstablished(const QString& ownHost)
{
  if(!ownHost.isEmpty())
  {
    kdDebug() << "Server::connectionEstablished(): start resolving the hostname by RPL_WELCOME: " << ownHost << endl;
    KNetwork::KResolver::resolveAsync(this,SLOT(gotOwnResolvedHostByWelcome(KResolverResults)),ownHost,"0");
  }

  emit serverOnline(true);
  if(m_isAutoAway) //we are in autoaway, so tell the server
    setAutoAway();

  if(!alreadyConnected)
  {
    alreadyConnected=true;
    // Make a helper object to build ISON (notify) list and map offline nicks to addressbook.
    // TODO: Give the object a kick to get it started?
    m_serverISON = new ServerISON(this);
     // get first notify very early
    startNotifyTimer(1000);
    // register with services
    if(!botPassword.isEmpty() && !bot.isEmpty())
      queue("PRIVMSG "+bot+" :identify "+botPassword);
    // get own ip by userhost
    requestUserhost(nickname);

    if(rejoinChannels) {
      rejoinChannels = false;
      autoRejoinChannels();
    }
  }
  else
    kdDebug() << "alreadyConnected==true! How did that happen?" << endl;
}

void Server::gotOwnResolvedHostByWelcome(KResolverResults res)
{
  if ( res.error() == KResolver::NoError && !res.isEmpty() ) {
    ownIpByWelcome = res.first().address().nodeName();
    kdDebug() << "Server::gotOwnResolvedHostByWelcome(): Success: " << ownIpByWelcome << endl;
  }
  else
    kdDebug() << "Server::gotOwnResolvedHostByWelcome(): Got error: " << ( int )res.error() << endl;
}

void Server::quitServer()
{
  QString command(KonversationApplication::preferences.getCommandChar()+"QUIT");
  Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),command,QString::null);
  queue(result.toServer);
}

void Server::notifyAction(const QString& nick)
{
  // parse wildcards (toParse,nickname,channelName,nickList,parameter)
  QString out=parseWildcards(KonversationApplication::preferences.getNotifyDoubleClickAction(),
                             getNickname(),
                             QString::null,
                             QString::null,
                             nick,
                             QString::null);
  // Send all strings, one after another
  QStringList outList=QStringList::split('\n',out);
  for(unsigned int index=0;index<outList.count();index++)
  {
    Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),outList[index],QString::null);
    queue(result.toServer);
  } // endfor
}

void Server::notifyResponse(const QString& nicksOnline)
{
  // We received a 303 or "PONG :LAG" notify message, so calculate server lag
  int lag=notifySent.elapsed();
  currentLag=lag;
  // inform main window
  emit serverLag(this,lag);
  // Stop check timer
  notifyCheckTimer.stop();
  // Switch off lag measuring mode
  inputFilter.setLagMeasuring(false);

  // only update Nicks Online list if we got a 303 response, not a PONG
  if(nicksOnline!="###")
  {

    bool nicksOnlineChanged = false;
    // Create a case correct nick list from the notification reply
    QStringList nickList=QStringList::split(' ',nicksOnline);
    // Create a lower case nick list from the notification reply
    QStringList nickLowerList=QStringList::split(' ',nicksOnline.lower());
    // Get ISON list from preferences and addressbook.
    QString watchlist=getISONListString();
    // Create a case correct nick list from the watch list.
    QStringList watchList=QStringList::split(' ',watchlist);
    // Create a lower case nick list from the watch list.
    QStringList watchLowerList=QStringList::split(' ',watchlist.lower());
    // Any new watched nicks online?
    unsigned int index;
    for(index=0;index<nickList.count();index++)
    {
      QString nickname = nickList[index];
      if (!isNickOnline(nickname))
      {
        setWatchedNickOnline(nickname);
        nicksOnlineChanged = true;
      }
    }
    // Any watched nicks now offline?
    for (index=0;index<watchList.count();index++)
    {
      QString lcNickName = watchList[index].lower();
      if (nickLowerList.find(lcNickName) == nickLowerList.end())
      {
        QString nickname = watchList[index];
        if (setNickOffline(nickname)) nicksOnlineChanged = true;
      }
    }
    // Note: The list emitted in this signal does not include nicks in joined channels.
    emit nicksNowOnline(this,nickList,nicksOnlineChanged);
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
    QString list=getISONListString();

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
  // start lag measuring mode
  inputFilter.setLagMeasuring(true);
}

// waiting too long for 303 response
void Server::notifyCheckTimeout()
{
  checkTime+=500;
  if(isConnected())
  {
    currentLag=checkTime;
    emit tooLongLag(this,checkTime);
    if(KonversationApplication::preferences.getAutoReconnect() &&
      (checkTime/1000)==KonversationApplication::preferences.getMaximumLagTime())
    {
      m_socket->close();
    }
  }
}

QString Server::getAutoJoinCommand() const
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
    else newNick = getIdentity()->getNickname(tryNickNumber);
  }
  return newNick;
}

void Server::processIncomingData()
{
  if(!inputBuffer.isEmpty())
  {
    QString front(inputBuffer.front());
    inputBuffer.pop_front();
    inputFilter.parseLine(front, mainWindow);
    if(rawLog) rawLog->appendRaw(front.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));
  }

  if(inputBuffer.isEmpty()) {
    incomingTimer.stop();
  }
}

void Server::unlockSending()
{
  sendUnlocked=true;
}

void Server::lockSending()
{
  sendUnlocked=false;
}

void Server::incoming()
{
  if(m_serverGroup.serverByIndex(m_currentServerIndex).SSLEnabled())
    emit sslConnected(this);

  // We read all available bytes here because readyRead() signal will be emitted when there is new data
  // else we will stall when displaying MOTD etc.
  int max_bytes;

  if(!m_serverGroup.serverByIndex(m_currentServerIndex).SSLEnabled())
    max_bytes = m_socket->bytesAvailable();
  else
    max_bytes = 512;

  QByteArray buffer(max_bytes+1);
  int len = 0;

  // Read at max "max_bytes" bytes into "buffer"
  len = m_socket->readBlock(buffer.data(),max_bytes);

  if(len <= 0 ) { // Zero means buffer is empty which shouldn't happen because readyRead signal is emitted
    statusView->appendServerMessage(i18n("Error"),
                                    i18n("There was an error reading the data from the server: %1").
                                    arg(m_socket->errorString()));

    broken(m_socket->error());
    return;
  }

  buffer[len] = 0;

  QCString qcsBuffer = inputBufferIncomplete + QCString(buffer);

  // split buffer to lines
  QValueList<QCString> qcsBufferLines;
  int lastLFposition = -1;
  for( int nextLFposition ; ( nextLFposition = qcsBuffer.find('\n', lastLFposition+1) ) != -1 ; lastLFposition = nextLFposition )
    qcsBufferLines << qcsBuffer.mid(lastLFposition+1, nextLFposition-lastLFposition-1);

  // remember an incompleted line (split by packets)
  inputBufferIncomplete = qcsBuffer.right(qcsBuffer.length()-lastLFposition-1);

  while(!qcsBufferLines.isEmpty())
  {
    QString testString = qcsBufferLines.front();
    bool isUtf8 = KStringHandler::isUtf8((Konversation::removeIrcMarkup(testString)).ascii());

    if(isUtf8)
      inputBuffer << KStringHandler::from8Bit(qcsBufferLines.front());
    else
    {
      // set channel encoding if specified
      // {
      QString senderNick;
      bool isServerMessage = false;
      QString channelKey;
      QTextCodec* tmpCodec = getIdentity()->getCodec();
      // pre-parse to know which channel the message belongs to
      QStringList lineSplit = QStringList::split(" ",tmpCodec->toUnicode(qcsBufferLines.front()));
      if(1 <= lineSplit.count())  // for safe
        if(lineSplit[0][0] == ':')
        {
          if(!lineSplit[0].contains('!'))
            isServerMessage = true;
          else
            senderNick = lineSplit[0].mid(1, lineSplit[0].find('!')-1);
          lineSplit.pop_front();  // remove prefix
        }
      // set channel key
      QString command = lineSplit[0].lower();
      if(isServerMessage)
      {
        if(3 <= lineSplit.count())
        {
          if( command == "332" )  // RPL_TOPIC
            channelKey = lineSplit[2];
          if( command == "372" )  // RPL_MOTD
            channelKey = ":server";
        }
      }
      else
      {
        if(2 <= lineSplit.count())
        {
          // query
          if( ( command == "privmsg" ||
                command == "notice"  ) &&
              lineSplit[1] == getNickname() )
            channelKey = senderNick;
          // channel message
          else if( command == "privmsg" ||
                   command == "notice"  ||
                   command == "join"    ||
                   command == "kick"    ||
                   command == "part"    ||
                   command == "topic"   )
            channelKey = lineSplit[1];
        }
      }
      // check setting
      QString channelEncoding;
      if(!channelKey.isEmpty()) {
        channelEncoding = KonversationApplication::preferences.getChannelEncoding(getServerGroup(), channelKey);
      }
      // }

      QTextCodec* codec;
      if(!channelEncoding.isEmpty())
        codec = IRCCharsets::codecForName(channelEncoding);
      else
        codec = getIdentity()->getCodec();
      inputBuffer << codec->toUnicode(qcsBufferLines.front());
    }
    qcsBufferLines.pop_front();
  }

  // refresh lock timer if it was still locked
  if(!sendUnlocked) lockSending();

  if(!incomingTimer.isActive()) {
    incomingTimer.start(0);
  }
}

void Server::queue(const QString& buffer)
{
  // Only queue lines if we are connected
  if(buffer.length())
  {
    outputBuffer.append(buffer);

    timerInterval*=2;
  }
}

void Server::queueAt(uint pos,const QString& buffer)
{
  // Only queue lines if we are connected
  if(buffer.length() && pos < outputBuffer.count())
  {
    outputBuffer.insert(outputBuffer.at(pos),buffer);

    timerInterval*=2;
  } else {
    queue(buffer);
  }
}

void Server::queueList(const QStringList& buffer)
{
  // Only queue lines if we are connected
  if(buffer.count())
  {
    for(unsigned int i=0;i<buffer.count();i++)
    {
      outputBuffer.append(*buffer.at(i));
      timerInterval*=2;
    } // for
  }
}

void Server::send()
{
  // Check if we are still online
  if(!isConnected() && !outputBuffer[0]) return;

  if(outputBuffer.count() && sendUnlocked)
  {
    // NOTE: It's important to add the linefeed here, so the encoding process does not trash it
    //       for some servers.
    QString outputLine=outputBuffer[0]+"\n";
    QStringList outputLineSplit=QStringList::split(" ",outputBuffer[0]);
    outputBuffer.pop_front();

    // To make lag calculation more precise, we reset the timer here
    if(outputLine.startsWith("ISON") ||
       outputLine.startsWith("PING LAG")) notifySent.start();
    
    // remember the first arg of /WHO to identify responses
    else if(outputLineSplit[0].upper()=="WHO")
    {
      if(2<=outputLineSplit.count())
        inputFilter.addWhoRequest(outputLineSplit[1]);
      else  // no argument (servers recognize it as "*")
        inputFilter.addWhoRequest("*");
    }

    // Don't reconnect if we WANT to quit
    else if(outputLine.startsWith("QUIT")) setDeliberateQuit(true);

    // wrap server socket into a stream
    QTextStream serverStream;
    
    serverStream.setDevice(m_socket);

    // set channel encoding if specified
    QString channelCodecName;
    if(2<=outputLineSplit.count())  // for safe
      if(outputLineSplit[0]=="PRIVMSG" ||
         outputLineSplit[0]=="NOTICE" ||
         outputLineSplit[0]=="KICK" ||
         outputLineSplit[0]=="PART" ||
         outputLineSplit[0]=="TOPIC")
        channelCodecName=KonversationApplication::preferences.getChannelEncoding(getServerGroup(),outputLineSplit[1]);

    // init stream props
    serverStream.setEncoding(QTextStream::Locale);
    QTextCodec* codec = getIdentity()->getCodec();

    if(!channelCodecName.isEmpty()) {
       codec = IRCCharsets::codecForName(channelCodecName);
    }

    // convert encoded data to IRC ascii only when we don't have the same codec locally
    if(QString(QTextCodec::codecForLocale()->name()).lower() != QString(codec->name()).lower())
    {
      serverStream.setCodec(codec);
    }

    serverStream << outputLine;
    if(rawLog) rawLog->appendRaw(outputLine.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));

    // detach server stream
    serverStream.unsetDevice();
  }

  // Flood-Protection
  if(timerInterval>1)
  {
    int time;
    timerInterval/=2;

    if(timerInterval>40) time=4000;
    else time=timerInterval*10+100;

    outgoingTimer.changeInterval(time);
  }
}

void Server::closed()
{
  broken(m_socket->error());
}

void Server::dcopRaw(const QString& command)
{
  if(command.startsWith(KonversationApplication::preferences.getCommandChar())) {
    queue(command.section(KonversationApplication::preferences.getCommandChar(), 1));
  }
  else
    queue(command);
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
      NickInfoPtr nickinfo = obtainNickInfo(target);
      query=addQuery(nickinfo, true);
    }
    if(query) {
      if(!command.isEmpty())
        query->sendQueryText(command);
      else {
	query->adjustFocus();
        getMainWindow()->show();
	KWin::demandAttention(getMainWindow()->winId());
        KWin::activateWindow(getMainWindow()->winId());
      }
    }
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

bool Server::getDeliberateQuit() const
{
  return deliberateQuit;
}

void Server::setDeliberateQuit(bool on)
{
  deliberateQuit=on;
}

QString Server::getNumericalIp(bool followDccSetting)
{
  QHostAddress ip;
  QString sip = getIp(followDccSetting);
  if(sip.isEmpty()) return sip;
  ip.setAddress(sip);

  return QString::number(ip.ip4Addr());
}

// Given a nickname, returns NickInfo object.   0 if not found.
NickInfoPtr Server::getNickInfo(const QString& nickname)
{
  QString lcNickname(nickname.lower());
  if (m_allNicks.contains(lcNickname))
  {
    NickInfoPtr nickinfo = m_allNicks[lcNickname];
    Q_ASSERT(nickinfo);
    return nickinfo;
  }
  else
    return 0;
}

// Given a nickname, returns an existing NickInfo object, or creates a new NickInfo object.
// Returns pointer to the found or created NickInfo object.
NickInfoPtr Server::obtainNickInfo(const QString& nickname)
{
  NickInfoPtr nickInfo = getNickInfo(nickname);
  if (!nickInfo)
  {
    nickInfo = new NickInfo(nickname, this);
    m_allNicks.insert(QString(nickname.lower()), nickInfo);
  }
  return nickInfo;
}

const NickInfoMap* Server::getAllNicks() { return &m_allNicks; }

// Returns the list of members for a channel in the joinedChannels list.
// 0 if channel is not in the joinedChannels list.
// Using code must not alter the list.
const ChannelNickMap *Server::getJoinedChannelMembers(const QString& channelName) const
{
  QString lcChannelName = channelName.lower();
  if (m_joinedChannels.contains(lcChannelName))
    return m_joinedChannels[lcChannelName];
  else
    return 0;
}

// Returns the list of members for a channel in the unjoinedChannels list.
// 0 if channel is not in the unjoinedChannels list.
// Using code must not alter the list.
const ChannelNickMap *Server::getUnjoinedChannelMembers(const QString& channelName) const
{
  QString lcChannelName = channelName.lower();
  if (m_unjoinedChannels.contains(lcChannelName))
    return m_unjoinedChannels[lcChannelName];
  else
    return 0;
}

// Searches the Joined and Unjoined lists for the given channel and returns the member list.
// 0 if channel is not in either list.
// Using code must not alter the list.
const ChannelNickMap *Server::getChannelMembers(const QString& channelName) const
{
  const ChannelNickMap *members = getJoinedChannelMembers(channelName);
  if (members)
    return members;
  else
    return getUnjoinedChannelMembers(channelName);
}

// Returns pointer to the ChannelNick (mode and pointer to NickInfo) for a given channel and nickname.
// 0 if not found.
ChannelNickPtr Server::getChannelNick(const QString& channelName, const QString& nickname)
{
  QString lcChannelName = channelName.lower();
  QString lcNickname = nickname.lower();
  const ChannelNickMap *channelNickMap = getChannelMembers(lcChannelName);
  if (channelNickMap)
  {
    if (channelNickMap->contains(lcNickname))
      return (*channelNickMap)[lcNickname];
    else
      return 0;
  }
  else
  {
    return 0;
  }
}

// Updates a nickname in a channel.  If not on the joined or unjoined lists, and nick
// is in the watch list, adds the channel and nick to the unjoinedChannels list.
// If mode != 99, sets the mode for the nick in the channel.
// Returns the NickInfo object if nick is on any lists, otherwise 0.
ChannelNickPtr Server::setChannelNick(const QString& channelName, const QString& nickname, unsigned int mode)
{
  QString lcNickname = nickname.lower();
  // If already on a list, update mode.
  ChannelNickPtr channelNick = getChannelNick(channelName, lcNickname);
  if (!channelNick)
  {
    // Get watch list from preferences.
    QString watchlist=getWatchListString();
    // Create a lower case nick list from the watch list.
    QStringList watchLowerList=QStringList::split(' ',watchlist.lower());
    // If on the watch list, add channel and nick to unjoinedChannels list.
    if (watchLowerList.find(lcNickname) != watchLowerList.end())
    {
      channelNick = addNickToUnjoinedChannelsList(channelName, nickname);
      channelNick->setMode(mode);
    }
    else return 0;
  }

    if (mode != 99) channelNick->setMode(mode);
    return channelNick;
}

// Returns a list of all the joined channels that a nick is in.
QStringList Server::getNickJoinedChannels(const QString& nickname)
{
  QString lcNickname = nickname.lower();
  QStringList channellist;
  ChannelMembershipMap::Iterator channel;
  for( channel = m_joinedChannels.begin(); channel != m_joinedChannels.end(); ++channel )
  {
    if (channel.data()->contains(lcNickname)) channellist.append(channel.key());
  }
  return channellist;
}

// Returns a list of all the channels (joined or unjoined) that a nick is in.
QStringList Server::getNickChannels(const QString& nickname)
{
  QString lcNickname = nickname.lower();
  QStringList channellist;
  ChannelMembershipMap::Iterator channel;
  for( channel = m_joinedChannels.begin(); channel != m_joinedChannels.end(); ++channel )
  {
    if (channel.data()->contains(lcNickname)) channellist.append(channel.key());
  }
  for( channel = m_unjoinedChannels.begin(); channel != m_unjoinedChannels.end(); ++channel )
  {
    if (channel.data()->contains(lcNickname)) channellist.append(channel.key());
  }
  return channellist;
}


bool Server::isNickOnline(const QString &nickname)
{
  NickInfoPtr nickInfo = getNickInfo(nickname);
  return (nickInfo != 0);
}

QString Server::getIp(bool followDccSetting)
{
  QString ip;

  if(followDccSetting)
  {
    int methodId = KonversationApplication::preferences.getDccMethodToGetOwnIp();

    if(methodId == 1)  // Reply from IRC server
    {
      if(!ownIpByWelcome.isEmpty())
      {
        kdDebug() << "Server::getIp(): using RPL_WELCOME" << endl;
        ip = ownIpByWelcome;
      }
      else if(!ownIpByUserhost.isEmpty())
      {
        kdDebug() << "Server::getIp(): using RPL_USERHOST" << endl;
        ip = ownIpByUserhost;
      }
    }
    else if(methodId == 2 && !KonversationApplication::preferences.getDccSpecificOwnIp().isEmpty())  // user specifies
    {
      KNetwork::KResolverResults res = KNetwork::KResolver::resolve(KonversationApplication::preferences.getDccSpecificOwnIp(), "");
      if(res.error() == KResolver::NoError && res.size() > 0)
      {
        kdDebug() << "Server::getIp(): using IP specified by user" << endl;
        ip = res.first().address().nodeName();
      }
    }
  }

  if(ip.isEmpty())
  {
    kdDebug() << "Server::getIp(): using the network interface" << endl;
    
    // Return our ip using serverSocket
    ip = m_socket->localAddress().nodeName();
  }
  
  kdDebug() << "Server::getIp(): returned: " << ip << endl;
  return ip;
}

Query *Server::addQuery(const NickInfoPtr & nickInfo, bool weinitiated)
{
  QString nickname = nickInfo->getNickname();
  // Only create new query object if there isn't already one with the same name
  Query* query=getQueryByName(nickname);
  if(!query)
  {
    QString lcNickname = nickname.lower();
    query=getMainWindow()->addQuery(this,nickInfo, weinitiated);
    query->setIdentity(getIdentity());

    connect(query,SIGNAL (sendFile(const QString&)),this,SLOT (requestDccSend(const QString &)) );
    connect(this,SIGNAL (serverOnline(bool)),query,SLOT (serverOnline(bool)) );

    // Append query to internal list
    queryList.append(query);

    m_queryNicks.insert(lcNickname, nickInfo);

#ifdef USE_KNOTIFY
    if(!weinitiated) {
      KNotifyClient::event(mainWindow->winId(), "query",
        i18n("%1 has started a conversation (query) with you.").arg(nickname));
    }
#endif
  }

  // try to get hostmask if there's none yet
  if(query->getNickInfo()->getHostmask().isEmpty()) requestUserhost(nickname);
  Q_ASSERT(query);
  return query;
}

void Server::closeQuery(const QString &name)
{
  Query* query=getQueryByName(name);
  removeQuery(query);

  // Update NickInfo.  If no longer on any lists, delete it altogether, but
  // only if not on the watch list.  ISON replies will determine whether the NickInfo
  // is deleted altogether in that case.
  QString lcNickname = name.lower();
  m_queryNicks.remove(lcNickname);
  if (!isWatchedNick(nickname)) deleteNickIfUnlisted(nickname);
}

void Server::closeChannel(const QString& name)
{
  kdDebug() << "Server::closeChannel(" << name << ")" << endl;
  Channel* channelToClose=getChannelByName(name);
  if(channelToClose)
  {
    Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),
      KonversationApplication::preferences.getCommandChar() + "PART", name);
    queue(result.toServer);
  }
}

void Server::requestChannelList()
{
  inputFilter.setAutomaticRequest("LIST", QString::null, true);
  queue("LIST");
}

void Server::requestWhois(const QString& nickname)
{
  inputFilter.setAutomaticRequest("WHOIS", nickname, true);
  queue("WHOIS "+nickname);
}

void Server::requestWho(const QString& channel)
{
  inputFilter.setAutomaticRequest("WHO", channel, true);
  queue("WHO "+channel);
}

void Server::requestUserhost(const QString& nicks)
{
  QStringList nicksList = QStringList::split(" ", nicks);
  for(QStringList::Iterator it=nicksList.begin() ; it!=nicksList.end() ; ++it)
    inputFilter.setAutomaticRequest("USERHOST", *it, true);
  queue("USERHOST "+nicks);
}

void Server::requestBan(const QStringList& users,const QString& channel,const QString& a_option)
{
  QString hostmask;
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

    Konversation::OutputFilterResult result = outputFilter->execBan(mask,channel);
    queue(result.toServer);
  }
}

void Server::requestUnban(const QString& mask,const QString& channel)
{
  Konversation::OutputFilterResult result = outputFilter->execUnban(mask,channel);
  queue(result.toServer);
}

void Server::requestDccSend()
{
  requestDccSend(QString::null);
}

void Server::requestDccSend(const QString &a_recipient)
{
  QString recipient(a_recipient);
  // if we don't have a recipient yet, let the user select one
  if(recipient.isEmpty())
  {
    QStringList nickList;
    Channel* lookChannel=channelList.first();

    // fill nickList with all nicks we know about
    while(lookChannel)
    {
      QPtrList<Nick> nicks=lookChannel->getNickList();
      Nick* lookNick=nicks.first();
      while(lookNick)
      {
        if(!nickList.contains(lookNick->getNickname())) nickList.append(lookNick->getNickname());
        lookNick=nicks.next();
      }
      lookChannel=channelList.next();
    }

    // add Queries as well, but don't insert duplicates
    Query* lookQuery=queryList.first();
    while(lookQuery)
    {
      if(!nickList.contains(lookQuery->getName())) nickList.append(lookQuery->getName());
      lookQuery=queryList.next();
    }

    recipient=DccRecipientDialog::getNickname(getMainWindow(),nickList);
  }
  // do we have a recipient *now*?
  if(!recipient.isEmpty())
  {
    KURL fileURL=KFileDialog::getOpenURL(
                                                   lastDccDir,
                                                   QString::null,
                                                   getMainWindow(),
                                                   i18n("Select File to Send to %1").arg(recipient)
                                                 );
    if(!fileURL.isEmpty())
    {
      lastDccDir=fileURL.directory();
      addDccSend(recipient,fileURL);
    }
  }
}

void Server::addDccSend(const QString &recipient,KURL fileURL, const QString &altFileName, uint fileSize)
{
  emit addDccPanel();

  QString ownIp = getIp(true);

  // We already checked that the file exists in output filter / requestDccSend() resp.
  DccTransferSend* newDcc=new DccTransferSend(getMainWindow()->getDccPanel(),
                                              recipient,
                                              fileURL,  // url to the sending file
                                              ownIp,    // ip
					      altFileName,
					      fileSize);

  connect(newDcc,SIGNAL (sendReady(const QString&,const QString&,const QString&,const QString&,unsigned long)),
    this,SLOT (dccSendRequest(const QString&,const QString&,const QString&,const QString&,unsigned long)) );
  connect(newDcc,SIGNAL (done(const QString&,DccTransfer::DccStatus,const QString&)),this,SLOT (dccSendDone(const QString&,DccTransfer::DccStatus,const QString&)) );
  connect(newDcc,SIGNAL (statusChanged(const DccTransfer* )), this,
         SLOT(dccStatusChanged(const DccTransfer*)) );
  newDcc->start();
}

void Server::addDccGet(const QString &sourceNick, const QStringList &dccArguments)
{
  emit addDccPanel();

  QHostAddress ip;

  ip.setAddress(dccArguments[1].toULong());

  appendStatusMessage(i18n("DCC"),
                      i18n("%1 offers the file \"%2\" (%3 bytes) for download (%4:%5).")
                              .arg(sourceNick)               // name
                              .arg(dccArguments[0])          // file
                              .arg((dccArguments[3].isEmpty()) ? i18n("unknown number of") : dccArguments[3] ) // size
                              .arg(ip.toString())            // ip
                              .arg(dccArguments[2])          // port
                             );

  DccTransferRecv* newDcc=new DccTransferRecv(getMainWindow()->getDccPanel(),
                      sourceNick,
                      KURL(KonversationApplication::preferences.getDccPath()),
                      dccArguments[0],     // name
                      dccArguments[3].isEmpty() ? 0 : dccArguments[3].toULong(),  // size
                      ip.toString(),       // ip
                      dccArguments[2]);    // port

  connect(newDcc,SIGNAL (resumeRequest(const QString&,const QString&,const QString&,KIO::filesize_t)),this,
         SLOT (dccResumeGetRequest(const QString&,const QString&,const QString&,KIO::filesize_t)) );
  connect(newDcc,SIGNAL (done(const QString&,DccTransfer::DccStatus,const QString&)),
              this,SLOT (dccGetDone(const QString&,DccTransfer::DccStatus,const QString&)) );
  connect(newDcc,SIGNAL (statusChanged(const DccTransfer* )), this,
         SLOT(dccStatusChanged(const DccTransfer*)) );

  if(KonversationApplication::preferences.getDccAutoGet()) newDcc->start();
}

void Server::requestKonsolePanel()
{
  emit addKonsolePanel();
}

void Server::requestDccPanel()
{
  emit addDccPanel();
}

void Server::requestCloseDccPanel()
{
  emit closeDccPanel();
}

void Server::requestDccChat(const QString& nickname)
{
  getMainWindow()->addDccChat(getNickname(),nickname,getNumericalIp(true),QStringList(),true);
}

void Server::dccSendRequest(const QString &partner, const QString &fileName, const QString &address, const QString &port, unsigned long size)
{
  kdDebug() << "dccSendRequest sent" << endl;
  Konversation::OutputFilterResult result = outputFilter->sendRequest(partner,fileName,address,port,size);
  queue(result.toServer);
  appendStatusMessage(result.typeString, result.output);
}

void Server::dccResumeGetRequest(const QString &sender, const QString &fileName, const QString &port, KIO::filesize_t startAt)
{
  SHOW;
  Konversation::OutputFilterResult result = outputFilter->resumeRequest(sender,fileName,port,startAt);
  queue(result.toServer);
  appendStatusMessage(result.typeString, result.output);
}

void Server::resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
  SHOW;
  // Check if there actually is a transfer going on on that port
  DccTransferRecv* dccTransfer=static_cast<DccTransferRecv*>(getMainWindow()->getDccPanel()->getTransferByPort(dccArguments[1],DccTransfer::Receive,true));
  if(!dccTransfer)
    // Check if there actually is a transfer going on with that name, could be behind a NAT
    // so the port number may get changed
    // mIRC substitutes this with "file.ext", so we have a problem here with mIRCs behind a NAT
    dccTransfer=static_cast<DccTransferRecv*>(getMainWindow()->getDccPanel()->getTransferByName(dccArguments[0],DccTransfer::Receive,true));

  if(dccTransfer)
  {
    // overcome mIRCs brain-dead "file.ext" substitution
    QString fileName=dccTransfer->getFileName();
    appendStatusMessage(i18n("DCC"),i18n("Resuming file \"%1\", offered by %2 from position %3.").arg(fileName).arg(sourceNick).arg(dccArguments[2]));
    dccTransfer->startResume(dccArguments[2].toULong());
  }
  else
  {
    appendStatusMessage(i18n("Error"),i18n("No DCC download running on port %1.").arg(dccArguments[1]));
  }
}

void Server::resumeDccSendTransfer(const QString &recipient, const QStringList &dccArguments)
{
  // Check if there actually is a transfer going on on that port
  DccTransferSend* dccTransfer=static_cast<DccTransferSend*>(getMainWindow()->getDccPanel()->getTransferByPort(dccArguments[1],DccTransfer::Send));
  if(!dccTransfer)
    // Check if there actually is a transfer going on with that name, could be behind a NAT
    // so the port number may get changed
    // mIRC substitutes this with "file.ext", so we have a problem here with mIRCs behind a NAT
    dccTransfer=static_cast<DccTransferSend*>(getMainWindow()->getDccPanel()->getTransferByName(dccArguments[0],DccTransfer::Send));

  if(dccTransfer && dccTransfer->getStatus() == DccTransfer::WaitingRemote)
  {
    QString fileName=dccTransfer->getFileName();
    if(dccTransfer->setResume(dccArguments[2].toULong()))
    {
      appendStatusMessage(i18n("DCC"),i18n("Resuming file \"%1\", offered by %2 from position %3.").arg(fileName).arg(recipient).arg(dccArguments[2]));
      Konversation::OutputFilterResult result = outputFilter->acceptRequest(recipient,
        fileName, dccArguments[1], dccArguments[2].toUInt());
      queue(result.toServer);
      appendStatusMessage(result.typeString, result.output);
    }
    else
    {
      appendStatusMessage(i18n("Error"),i18n("Received invalid resume request for file \"%1\" (position %2) from %3.").arg(fileName).arg(dccArguments[2]).arg(recipient));
    }
  }
  else
  {
    appendStatusMessage(i18n("Error"),i18n("No DCC upload running on port %1.").arg(dccArguments[1]));
  }
}

void Server::dccGetDone(const QString &fileName, DccTransfer::DccStatus status, const QString &errorMessage)
{
  if(status==DccTransfer::Done)
    appendStatusMessage(i18n("DCC"),i18n("DCC download of file \"%1\" finished.").arg(fileName));
  else if(status==DccTransfer::Failed)
    appendStatusMessage(i18n("DCC"),i18n("DCC download of file \"%1\" failed. reason: %2").arg(fileName).arg(errorMessage));
}

void Server::dccSendDone(const QString &fileName, DccTransfer::DccStatus status, const QString &errorMessage)
{
  if(status==DccTransfer::Done)
    appendStatusMessage(i18n("DCC"),i18n("DCC upload of file \"%1\" finished.").arg(fileName));
  else if(status==DccTransfer::Failed)
    appendStatusMessage(i18n("DCC"),i18n("DCC upload of file \"%1\" failed. reason: %2").arg(fileName).arg(errorMessage));
}

void Server::dccStatusChanged(const DccTransfer *item)
{
  getMainWindow()->getDccPanel()->dccStatusChanged(item);
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
#ifndef USE_MDI
  // Free the query object
  delete query;
#endif
}

void Server::sendJoinCommand(const QString& name)
{
  Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),
    KonversationApplication::preferences.getCommandChar() + "JOIN " + name, QString::null);
  queue(result.toServer);
}

void Server::joinChannel(const QString &name, const QString &hostmask, const QString &/*key*/)
{
  // (re-)join channel, open a new panel if needed
  Channel* channel=getChannelByName(name);
  if(!channel)
  {
    channel=getMainWindow()->addChannel(this,name);
    Q_ASSERT(channel);
    channel->setIdentity(getIdentity());
    channel->setNickname(getNickname());
  //channel->setKey(key);

    channelList.append(channel);

    connect(channel,SIGNAL (sendFile()),this,SLOT (requestDccSend()) );
    connect(this,SIGNAL (serverOnline(bool)),channel,SLOT (serverOnline(bool)) );
  }
  // Move channel from unjoined (if present) to joined list and add our own nickname to the joined list.
  ChannelNickPtr channelNick = addNickToJoinedChannelsList(name, getNickname());

  if ((channelNick->getHostmask() != hostmask ) && !hostmask.isEmpty())
  {
    NickInfoPtr nickInfo = channelNick->getNickInfo();
    nickInfo->setHostmask(hostmask);
  }
  channel->joinNickname(channelNick);
}

void Server::removeChannel(Channel* channel)
{
  // Update NickInfo.
  removeJoinedChannel(channel->getName());

  channelList.removeRef(channel);
}

void Server::updateChannelMode(const QString &updater, const QString &channelName, char mode, bool plus, const QString &parameter)
{

  Channel* channel=getChannelByName(channelName);

  if(channel) //Let the channel be verbose to the screen about the change, and update channelNick
	  channel->updateMode(updater, mode, plus, parameter);
  // TODO: What is mode character for owner?
  // Answer from JOHNFLUX - I think that admin is the same as owner.  Channel.h has owner as "a"
  // "q" is the likely answer.. UnrealIRCd and euIRCd use it.
  // TODO these need to become dynamic
  QString userModes="vhoqa";    // voice halfop op owner admin
  int modePos = userModes.find(mode);
  if (modePos > 0)
  {
    ChannelNickPtr updateeNick = getChannelNick(channelName, parameter);
    if(!updateeNick) {
	  kdDebug() << "in updateChannelMode, could not find updatee nick " << parameter << " for channel " << channelName << endl;
	  kdDebug() << "This could indicate an obscure race condition that is safely being handled (like the mode of someone changed and they quit almost simulatanously, or it could indicate an internal error.";
	  //TODO Do we need to add this nick?
	  return;
    }

    updateeNick->setMode(mode, plus);

    // Note that channel will be moved to joined list if necessary.
    addNickToJoinedChannelsList(channelName, parameter);
  }

}


void Server::updateChannelModeWidgets(const QString &channelName, char mode, const QString &parameter)
{
  Channel* channel=getChannelByName(channelName);
  if(channel) channel->updateModeWidgets(mode,true,parameter);
}

void Server::updateChannelQuickButtons()
{
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->updateQuickButtons(KonversationApplication::preferences.getButtonList());
    channel=channelList.next();
  }
}

// TODO: Maybe use a Signal / Slot mechanism for these things?
void Server::updateFonts()
{
  Channel* channel=channelList.first();
  while(channel)
  {
    channel->updateFonts();
    channel->updateStyleSheet();
    channel=channelList.next();
  }

  Query* query=queryList.first();
  while(query)
  {
    query->updateFonts();
    query=queryList.next();
  }

  statusView->updateFonts();
  getMainWindow()->updateFonts();

  if(rawLog) rawLog->updateFonts();
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

void Server::setShowTopic(bool state)
{
  Channel* channel = channelList.first();
  
  while(channel)
  {
    channel->showTopic(state);
    channel = channelList.next();
  }
}

void Server::setShowNicknameBox(bool state)
{
  Channel* channel = channelList.first();
  
  while(channel)
  {
    channel->setShowNicknameBox(state);
    channel = channelList.next();
  }

  getStatusView()->setShowNicknameBox(state);
}

Channel* Server::getChannelByName(const QString& name)
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

Query* Server::getQueryByName(const QString& name)
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

void Server::addPendingNickList(const QString& channelName,const QStringList& nickList)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->addPendingNickList(nickList);
}

// Adds a nickname to the joinedChannels list.
// Creates new NickInfo if necessary.
// If needed, moves the channel from the unjoined list to the joined list.
// Returns the NickInfo for the nickname.
ChannelNickPtr Server::addNickToJoinedChannelsList(const QString& channelName, const QString& nickname)
{
  bool doChannelJoinedSignal = false;
  bool doWatchedNickChangedSignal = false;
  bool doChannelMembersChangedSignal = false;
  QString lcNickname = nickname.lower();
  // Create NickInfo if not already created.
  NickInfoPtr nickInfo = getNickInfo(nickname);
  if (!nickInfo)
  {
    nickInfo = new NickInfo(nickname, this);
    m_allNicks.insert(lcNickname, nickInfo);
    doWatchedNickChangedSignal = isWatchedNick(nickname);
  }
  // Move the channel from unjoined list (if present) to joined list.
  QString lcChannelName = channelName.lower();
  ChannelNickMap *channel;
  if (m_unjoinedChannels.contains(lcChannelName))
  {
    channel = m_unjoinedChannels[lcChannelName];
    m_unjoinedChannels.remove(lcChannelName);
    m_joinedChannels.insert(lcChannelName, channel);
    doChannelJoinedSignal = true;
  }
  else
  {
    // Create a new list in the joined channels if not already present.
    if (!m_joinedChannels.contains(lcChannelName))
    {
      channel = new ChannelNickMap;
      m_joinedChannels.insert(lcChannelName, channel);
      doChannelJoinedSignal = true;
    }
    else
      channel = m_joinedChannels[lcChannelName];
  }
  // Add NickInfo to channel list if not already in the list.
  ChannelNickPtr channelNick;
  if (!channel->contains(lcNickname))
  {
    channelNick = new ChannelNick(nickInfo, false, false, false, false, false);
    Q_ASSERT(channelNick);
    channel->insert(lcNickname, channelNick);
    doChannelMembersChangedSignal = true;
  }
  channelNick = (*channel)[lcNickname];
  Q_ASSERT(channelNick); //Since we just added it if it didn't exist, it should be guaranteed to exist now
  if (doWatchedNickChangedSignal) emit watchedNickChanged(this, nickname, true);
  if (doChannelJoinedSignal) emit channelJoinedOrUnjoined(this, channelName, true);
  if (doChannelMembersChangedSignal) emit channelMembersChanged(this, channelName, true, false, nickname);
  return channelNick;
}

/** This function should _only_ be called from the ChannelNick class.
 *  This function should also be the only one to emit this signal.
 *  In this class, when channelNick is changed, it emits its own signal, and
 *  calls this function itself.
 */
void Server::emitChannelNickChanged(const ChannelNickPtr channelNick) {
  emit channelNickChanged(this, channelNick);
}
/** This function should _only_ be called from the NickInfo class.
 *  This function should also be the only one to emit this signal.
 *  In this class, when nickInfo is changed, it emits its own signal, and
 *  calls this function itself.
 */
void Server::emitNickInfoChanged(const NickInfoPtr nickInfo) {
  emit nickInfoChanged(this, nickInfo);
}

// Adds a nickname to the unjoinedChannels list.
// Creates new NickInfo if necessary.
// If needed, moves the channel from the joined list to the unjoined list.
// If mode != 99 sets the mode for this nick in this channel.
// Returns the NickInfo for the nickname.
ChannelNickPtr Server::addNickToUnjoinedChannelsList(const QString& channelName, const QString& nickname)
{
  bool doChannelUnjoinedSignal = false;
  bool doWatchedNickChangedSignal = false;
  bool doChannelMembersChangedSignal = false;
  QString lcNickname = nickname.lower();
  // Create NickInfo if not already created.
  NickInfoPtr nickInfo = getNickInfo(nickname);
  if (!nickInfo)
  {
    nickInfo = new NickInfo(nickname, this);
    m_allNicks.insert(QString(nickname.lower()), nickInfo);
    doWatchedNickChangedSignal = isWatchedNick(nickname);
  }
  // Move the channel from joined list (if present) to unjoined list.
  QString lcChannelName = channelName.lower();
  ChannelNickMap *channel;
  if (m_joinedChannels.contains(lcChannelName))
  {
    channel = m_joinedChannels[lcChannelName];
    m_joinedChannels.remove(lcChannelName);
    m_unjoinedChannels.insert(lcChannelName, channel);
    doChannelUnjoinedSignal = true;
  }
  else
  {
    // Create a new list in the unjoined channels if not already present.
    if (!m_unjoinedChannels.contains(lcChannelName))
    {
      channel = new ChannelNickMap;
      m_unjoinedChannels.insert(lcChannelName, channel);
      doChannelUnjoinedSignal = true;
    }
    else
      channel = m_unjoinedChannels[lcChannelName];
  }
  // Add NickInfo to unjoinedChannels list if not already in the list.
  ChannelNickPtr channelNick;
  if (!channel->contains(lcNickname))
  {
    channelNick = new ChannelNick(nickInfo, false, false, false, false, false);
    channel->insert(lcNickname, channelNick);
    doChannelMembersChangedSignal = true;
  }
  channelNick = (*channel)[lcNickname];
  // Set the mode for the nick in this channel.
  if (doWatchedNickChangedSignal) emit watchedNickChanged(this, nickname, true);
  if (doChannelUnjoinedSignal) emit channelJoinedOrUnjoined(this, channelName, false);
  if (doChannelMembersChangedSignal) emit channelMembersChanged(this, channelName, false, false, nickname);
  return channelNick;
}

/**
* If not already online, changes a nick to the online state by creating
* a NickInfo for it and emits various signals and messages for it.
* This method should only be called for nicks on the watch list.
* @param nickname           The nickname that is online.
* @return                   Pointer to NickInfo for nick.
*/
NickInfoPtr Server::setWatchedNickOnline(const QString& nickname)
{
  NickInfoPtr nickInfo = getNickInfo(nickname);
  if (!nickInfo)
  {
    QString lcNickname = nickname.lower();
    nickInfo = new NickInfo(nickname, this);
    m_allNicks.insert(lcNickname, nickInfo);
    emit watchedNickChanged(this, nickname, true);
    Konversation::Addressbook::self()->emitContactPresenceChanged(nickInfo->getAddressee().uid());
    getMainWindow()->appendToFrontmost(i18n("Notify"),
      i18n("%1 is online (%2).").arg(nickname).arg(getServerName()),statusView);

#ifdef USE_KNOTIFY
    KNotifyClient::event(mainWindow->winId(), "notify",
      i18n("%1 is online (%2).").arg(nickname).arg(getServerName()));
#endif
  }
  return nickInfo;
}

bool Server::setNickOffline(const QString& nickname)
{
  QString lcNickname = nickname.lower();
  NickInfoPtr nickInfo = getNickInfo(lcNickname);
  if (nickInfo)
  {
    KABC::Addressee addressee = nickInfo->getAddressee();
    // Delete from query list, if present.
    if (m_queryNicks.contains(lcNickname)) m_queryNicks.remove(lcNickname);
    // Delete the nickname from all channels (joined or unjoined).
    QStringList nickChannels = getNickChannels(lcNickname);
    for (unsigned int index=0; index<nickChannels.count(); index++)
    {
      QString channel = nickChannels[index];
      removeChannelNick(channel, lcNickname);
    }
    // Delete NickInfo.
    if (m_allNicks.contains(lcNickname)) m_allNicks.remove(lcNickname);
    // If the nick was in the watch list, emit various signals and messages.
    if (isWatchedNick(nickname))
    {
      emit watchedNickChanged(this, nickname, false);
      if (!addressee.isEmpty())
        Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid(), 1);
      getMainWindow()->appendToFrontmost(i18n("Notify"),
        i18n("%1 went offline (%2).").arg(nickname).arg(getServerName()),statusView);
#ifdef USE_KNOTIFY
      KNotifyClient::event(mainWindow->winId(), "notify",
        i18n("%1 went offline (%2).").arg(nickname).arg(getServerName()));
#endif
    }
  }
  return (nickInfo != 0);
}

/**
* If nickname is no longer on any channel list, or the query list, delete it altogether.
* Call this routine only if the nick is not on the notify list or is on the notify
* list but is known to be offline.
* @param nickname           The nickname to be deleted.  Case insensitive.
* @return                   True if the nickname is deleted.
*/
bool Server::deleteNickIfUnlisted(QString &nickname)
{
  // Don't delete our own nickinfo.
  if (nickname == getNickname()) return false;
  QString lcNickname = nickname.lower();
  if (!m_queryNicks.contains(lcNickname))
  {
    QStringList nickChannels = getNickChannels(nickname);
    if (nickChannels.isEmpty())
    {
      m_allNicks.remove(lcNickname);
      return true;
    }
  }
  return false;
}

/**
* Remove nickname from a channel (on joined or unjoined lists).
* @param channelName The channel name.  Case insensitive.
* @param nickname    The nickname.  Case insensitive.
*/
void Server::removeChannelNick(const QString& channelName, const QString& nickname)
{
  bool doSignal = false;
  bool joined = false;
  QString lcChannelName = channelName.lower();
  QString lcNickname = nickname.lower();
  ChannelNickMap *channel;
  if (m_joinedChannels.contains(lcChannelName))
  {
    channel = m_joinedChannels[lcChannelName];
    if (channel->contains(lcNickname))
    {
      channel->remove(lcNickname);
      doSignal = true;
      joined = true;
      // Note: Channel should not be empty because user's own nick should still be
      // in it, so do not need to delete empty channel here.
    }
  }
  else
  {
    if (m_unjoinedChannels.contains(lcChannelName))
    {
      channel = m_unjoinedChannels[lcChannelName];
      if (channel->contains(lcNickname))
      {
        channel->remove(lcNickname);
        doSignal = true;
        joined = false;
        // If channel is now empty, delete it.
        // Caution: Any iterators across unjoinedChannels will be come invalid here.
        if (channel->isEmpty()) m_unjoinedChannels.remove(lcChannelName);
      }
    }
  }
  if (doSignal) emit channelMembersChanged(this, channelName, joined, true, nickname);
}

QStringList Server::getWatchList()
{
  if (m_serverISON)
    return m_serverISON->getWatchList();
  else
    return QStringList();
}
QString Server::getWatchListString() { return getWatchList().join(" "); }

QStringList Server::getISONList()
{
  if (m_serverISON)
    return m_serverISON->getISONList();
  else
    return QStringList();
}
QString Server::getISONListString() { return getISONList().join(" "); }

/**
* Return true if the given nickname is on the watch list.
*/
bool Server::isWatchedNick(const QString& nickname)
{
    QStringList watchList = getWatchList();
    return (watchList.contains(nickname.lower()));
}

/**
* Remove channel from the joined list, placing it in the unjoined list.
* All the unwatched nicks are removed from the channel.  If the channel becomes
* empty, it is deleted.
* @param channelName        Name of the channel.  Case sensitive.
*/
void Server::removeJoinedChannel(const QString& channelName)
{
  bool doSignal = false;
  QString watchList = getWatchListString();
  QStringList watchListLower = QStringList::split(' ', watchList.lower());
  QString lcChannelName = channelName.lower();
  // Move the channel nick list from the joined to unjoined lists.
  if (m_joinedChannels.contains(lcChannelName))
  {
    doSignal = true;
    ChannelNickMap* channel = m_joinedChannels[lcChannelName];
    m_joinedChannels.remove(lcChannelName);
    m_unjoinedChannels.insert(lcChannelName, channel);
    // Remove nicks not on the watch list.
    bool allDeleted = true;
    Q_ASSERT(channel);
    if(!channel) return; //already removed.. hmm
    ChannelNickMap::Iterator member;
    for ( member = channel->begin(); member != channel->end() ;)
    {
      QString lcNickname = member.key();
      if (watchListLower.find(lcNickname) == watchListLower.end())
      {
        // Remove the unwatched nickname from the unjoined channel.
        channel->remove(member);
        // If the nick is no longer listed in any channels or query list, delete it altogether.
        deleteNickIfUnlisted(lcNickname);
        member = channel->begin();
      }
      else
      {
        allDeleted = false;
        ++member;
      }
    }
    // If all were deleted, remove the channel from the unjoined list.
    if (allDeleted)
    {
      channel = m_unjoinedChannels[lcChannelName];
      m_unjoinedChannels.remove(lcChannelName);
      delete channel;  // recover memory!
    }
  }
  if (doSignal) emit channelJoinedOrUnjoined(this, channelName, false);
}


// Renames a nickname in all NickInfo lists.
// Returns pointer to the NickInfo object or 0 if nick not found.
void Server::renameNickInfo(NickInfoPtr nickInfo, const QString& newname)
{
  if (nickInfo)
  {
    // Get existing lowercase nickname and rename nickname in the NickInfo object.
    QString lcNickname = nickInfo->getNickname().lower();
    nickInfo->setNickname(newname);
    QString lcNewname = newname.lower();
    // Rename the key in m_allNicks list.
    m_allNicks.remove(lcNickname);
    m_allNicks.insert(lcNewname, nickInfo);
    // Rename key in the joined and unjoined lists.
    QStringList nickChannels = getNickChannels(lcNickname);
    for (unsigned int index=0;index<nickChannels.count();index++)
    {
      const ChannelNickMap *channel = getChannelMembers(nickChannels[index]);
      Q_ASSERT(channel);
      ChannelNickPtr member = (*channel)[lcNickname];
      Q_ASSERT(member);
      const_cast<ChannelNickMap *>(channel)->remove(lcNickname);
      const_cast<ChannelNickMap *>(channel)->insert(lcNewname, member);
    }
    // Rename key in Query list.
    if (m_queryNicks.contains(lcNickname))
    {
      m_queryNicks.remove(lcNickname);
      m_queryNicks.insert(lcNewname, nickInfo);
    }
  } else {
    kdDebug() << "server::renameNickInfo() was called for newname='" << newname << "' but nickInfo is null" << endl;
  }
}

void Server::noMorePendingNicks(const QString& channelName)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel) outChannel->setPendingNicks(false);
}

void Server::addNickToChannel(const QString &channelName,const QString &nickname,const QString &hostmask,
                              bool admin,bool owner,bool op,bool halfop,bool voice)
{
  Channel* outChannel=getChannelByName(channelName);

  // Update NickInfo.
  ChannelNickPtr channelNick = addNickToJoinedChannelsList(channelName, nickname);
  channelNick->setMode(admin,owner,op,halfop,voice);
  if(outChannel) outChannel->addNickname(channelNick);
  NickInfoPtr nickInfo = channelNick->getNickInfo();
  if ((nickInfo->getHostmask() != hostmask) && !hostmask.isEmpty())
  {
    nickInfo->setHostmask(hostmask);
  }
}

Channel* Server::nickJoinsChannel(const QString &channelName, const QString &nickname, const QString &hostmask)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel)
  {

    // OnScreen Message
    if(KonversationApplication::preferences.getOSDShowChannelEvent() && outChannel->notificationsEnabled())
    {
      KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
      konvApp->osd->showOSD(i18n( "(%1) %2 has joined this channel. (%3)" )
                            .arg(channelName).arg(nickname).arg(hostmask));
    }

    // Update NickInfo.
    ChannelNickPtr channelNick = addNickToJoinedChannelsList(channelName, nickname);
    NickInfoPtr nickInfo = channelNick->getNickInfo();
    if ((nickInfo->getHostmask() != hostmask) && !hostmask.isEmpty())
    {
      nickInfo->setHostmask(hostmask);
    }
    outChannel->joinNickname(channelNick);
  }
  
  return outChannel;
}

void Server::addHostmaskToNick(const QString& sourceNick, const QString& sourceHostmask)
{
  // Update NickInfo.
  NickInfoPtr nickInfo=getNickInfo(sourceNick);
  if (nickInfo)
  {
    if ((nickInfo->getHostmask() != sourceHostmask) && !sourceHostmask.isEmpty())
    {
      nickInfo->setHostmask(sourceHostmask);
    }
  }
}

Channel* Server::removeNickFromChannel(const QString &channelName, const QString &nickname, const QString &reason, bool quit)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel)
  {
    // OnScreen Message
    if(KonversationApplication::preferences.getOSDShowChannelEvent() && outChannel->notificationsEnabled())
    {
      KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
      konvApp->osd->showOSD(i18n( "(%1) %2 has left this channel. (%3)" )
                            .arg(channelName).arg(nickname).arg(reason));
    }
    ChannelNickPtr channelNick = getChannelNick(channelName, nickname);
    if(channelNick) outChannel->removeNick(channelNick,reason,quit);
  }

  // Remove the nick from the channel.
  removeChannelNick(channelName, nickname);
  // If not listed in any channel, and not on query list, delete the NickInfo,
  // but only if not on the notify list.  ISON replies will take care of deleting
  // the NickInfo, if on the notify list.
  if (!isWatchedNick(nickname))
  {
    QString nicky = nickname;
    deleteNickIfUnlisted(nicky);
  }
  
  return outChannel;
}

void Server::nickWasKickedFromChannel(const QString &channelName, const QString &nickname, const QString &kicker, const QString &reason)
{
  Channel* outChannel=getChannelByName(channelName);
  if(outChannel)
  {
    ChannelNickPtr channelNick = getChannelNick(channelName, nickname);
    ChannelNickPtr kickerNick = getChannelNick(channelName, kicker);
    if(channelNick) outChannel->kickNick(channelNick, *kickerNick, reason);
  }

  // TODO: Need to update NickInfo, or does that happen in method above?
}

void Server::removeNickFromServer(const QString &nickname,const QString &reason)
{
  Channel* channel=channelList.first();
  while(channel)
  {
    // Check if nick is in this channel or not.
    Channel *nextchannel = channelList.next();
    if( channel->getNickByName( nickname ) )
      removeNickFromChannel(channel->getName(),nickname,reason,true);
    channel=nextchannel;
  }

  // Delete the nick from all channels and then delete the nickinfo,
  // emitting signal if on the watch list.
  setNickOffline(nickname);
}

void Server::renameNick(const QString &nickname, const QString &newNick)
{
  if(nickname.isEmpty() || newNick.isEmpty()) {
    kdDebug() << "server::renameNick called with empty strings!  Trying to rename '" << nickname << "' to '" << newNick << "'" << endl;
    return;
  }
  //Actually do the rename.
  NickInfoPtr nickInfo = getNickInfo(nickname);
  if(!nickInfo) {
    kdDebug() << "server::renameNick called for nickname '" << nickname << "' to '" << newNick << "' but getNickInfo('" << nickname << "') returned no results." << endl;
  }
  else
  {
    renameNickInfo(nickInfo, newNick);
    //The rest of the code below allows the channels to echo to the user to tell them that the nick has changed.

    // Rename the nick in every channel they are in
    Channel* channel=channelList.first();
    while(channel)
    {
      // All we do is notify that the nick has been renamed.. we haven't actually renamed it yet
      // Note that NickPanel has already updated, so pass new nick to getNickByName.
      if(channel->getNickByName(newNick)) channel->nickRenamed(nickname, *nickInfo);
      channel=channelList.next();
    }
  }
  // If this was our own nickchange, tell our server object about it
  if(nickname==getNickname()) setNickname(newNick);
  // If we had a query with this nick, change that name, too

}

void Server::userhost(const QString& nick,const QString& hostmask,bool away,bool /* ircOp */)
{
  addHostmaskToNick(nick,hostmask);
  // remember my IP for DCC things
  if(ownIpByUserhost.isEmpty() && nick==nickname)  // myself
  {
    QString myhost = hostmask.section('@', 1);
    kdDebug() << "Server::userhost(): start resolving the hostname by RPL_USERHOST: " << myhost << endl;
    // Use async lookup else you will be blocking GUI badly
    KNetwork::KResolver::resolveAsync(this,SLOT(gotOwnResolvedHostByUserhost(KResolverResults)),myhost,"0");
  }
  NickInfoPtr nickInfo = getNickInfo(nick);
  if (nickInfo)
  {
    if (nickInfo->isAway() != away)
    {
      nickInfo->setAway(away);
    }
  }
}

void Server::gotOwnResolvedHostByUserhost(KResolverResults res)
{
  if ( res.error() == KResolver::NoError && !res.isEmpty() ) {
    ownIpByUserhost = res.first().address().nodeName();
    kdDebug() << "Server::gotOwnResolvedHostByUserhost(): Success: " << ownIpByUserhost << endl;
  }
  else
    kdDebug() << "Server::gotOwnResolvedHostByUserhost(): Got error: " << ( int )res.error() << endl;
}

void Server::appendServerMessageToChannel(const QString& channel,const QString& type,const QString& message)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->appendServerMessage(type,message);
}

void Server::appendCommandMessageToChannel(const QString& channel,const QString& command,const QString& message)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel) outChannel->appendCommandMessage(command,message);
}

void Server::appendServerMessageToQuery(const QString& queryName,const QString& type,const QString& message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendServerMessage(type,message);
  else kdWarning() << "Server::appendServerMessageToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendCommandMessageToQuery(const QString& queryName,const QString& command,const QString& message)
{
  Query* outQuery=getQueryByName(queryName);
  if(outQuery) outQuery->appendCommandMessage(command,message);
  else kdWarning() << "Server::appendCommandMessageToQuery(" << queryName << "): Query not found!" << endl;
}

void Server::appendStatusMessage(const QString& type,const QString& message)
{
  statusView->appendServerMessage(type,message);
}

void Server::appendMessageToFrontmost(const QString& type,const QString& message)
{
  getMainWindow()->appendToFrontmost(type,message, statusView);
}

void Server::setNickname(const QString &newNickname)
{
  nickname=newNickname;
  statusView->setNickname(newNickname);
}

void Server::setChannelTopic(const QString &channel, const QString &newTopic)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel)
  {
    // encoding stuff is done in send()
    outChannel->setTopic(newTopic);
  }
}

void Server::setChannelTopic(const QString& nickname, const QString &channel, const QString &newTopic) // Overloaded
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel)
  {
    // encoding stuff is done in send()
    outChannel->setTopic(nickname,newTopic);
  }
}

void Server::setTopicAuthor(const QString& channel,const QString& author)
{
  Channel* outChannel=getChannelByName(channel);
  if(outChannel)
  {
    outChannel->setTopicAuthor(author);
  }
}

void Server::endOfWho(const QString& target)
{
  Channel* channel=getChannelByName(target);
  if(channel)
  {
    channel->scheduleAutoWho();
  }
}

bool Server::isNickname(const QString &compare)
{
  return (nickname==compare);
}

QString Server::getNickname() const
{
  return nickname;
}

QString Server::parseWildcards(const QString& toParse,
                               const QString& sender,
                               const QString& channelName,
                               const QString& channelKey,
                               const QString& nick,
                               const QString& parameter)
{
  return parseWildcards(toParse,sender,channelName,channelKey,QStringList::split(' ',nick),parameter);
}

QString Server::parseWildcards(const QString& toParse,
                               const QString& sender,
                               const QString& channelName,
                               const QString& channelKey,
                               const QStringList& nickList,
                               const QString& /*parameter*/)
{
  // TODO: parameter handling.
  //       since parameters are not functional yet

  // make a copy to work with
  QString out(toParse);
  // define default separator and regular expression for definition
  QString separator(" ");
  QRegExp separatorRegExp("%s[^%]*%");

  // separator definition found?
  int pos=out.find(separatorRegExp);
  if(pos!=-1)
  {
    // TODO: This could be better done with .cap() and proper RegExp ...
    // skip "%s" at the beginning
    pos+=2;
    // copy out all text to the next "%" as new separator
    separator=out.mid(pos,out.find("%",pos+1)-pos);
    // remove separator definition from string
    out.replace(separatorRegExp, QString::null);
  }

  out.replace("%u",nickList.join(separator));
  if(!channelName.isEmpty()) out.replace("%c",channelName);
  out.replace("%o",sender);
  if(!channelKey.isEmpty()) out.replace("%k",channelKey);
  if(!m_serverGroup.serverByIndex(m_currentServerIndex).password().isEmpty()) {
    out.replace("%K", m_serverGroup.serverByIndex(m_currentServerIndex).password());
  }

  out.replace("%n","\n");
  // finally replace all "%p" with "%"
  out.replace("%p","%");

  return out;
}

void Server::setIrcName(const QString &newIrcName)
{
  ircName=newIrcName;
}

QString Server::getIrcName() const
{
  return ircName;
}

InputFilter* Server::getInputFilter()
{
  return &inputFilter;
}

Konversation::OutputFilter* Server::getOutputFilter()
{
  return outputFilter;
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

void Server::invitation(const QString& nick,const QString& channel)
{
  if(KonversationApplication::preferences.getAutojoinOnInvite() &&
     KMessageBox::questionYesNo(mainWindow,
                                i18n("You were invited by %1 to join channel %2. "
                                     "Do you accept the invitation?").arg(nick).arg(channel),
                                i18n("Invitation"),
                                KStdGuiItem::yes(),
                                KStdGuiItem::no(),
                                "Invitation")==KMessageBox::Yes)
  {
    sendJoinCommand(channel);
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

void Server::away()
{
  if(!m_isAway)
    startAwayTimer(); //Don't start timer if we have already started it
  m_isAway=true;
  emit awayState(true);

  if(!getIdentity()->getAwayNick().isEmpty() &&
     getIdentity()->getAwayNick() != getNickname()) {
    nonAwayNick = getNickname();
    queue("NICK " + getIdentity()->getAwayNick());
  }

  if(getIdentity()->getInsertRememberLineOnAway())
  {
    emit awayInsertRememberLine();
  }
  // TODO: call renameNickInfo ?
  
  if(getMainWindow()) {
    KAction *action = getMainWindow()->actionCollection()->action("toggle_away");
    if(action) {
      action->setText(i18n("Set &Available Globally"));
      action->setIcon("konversationavailable");
    }
  }
  
}
void Server::setAutoAway() {
  kdDebug() << "going autoaway!" << endl;
  m_isAutoAway = true;
  //note that we now need to tell the server we are away.  m_isAway is set when we get a reply from the server saying we are now away.
  executeMultiServerCommand("away", i18n("Gone away for now.")); //fix this to use a prefered auto-away string.
}
void Server::unAway()
{
  m_isAway=false;
  m_isAutoAway = false;
  emit awayState(false);

  if(!getIdentity()->getAwayNick().isEmpty() && !nonAwayNick.isEmpty()) {
    queue("NICK " + nonAwayNick);
  }

  // TODO: call renameNickInfo ?

  if(getMainWindow()) {
    KAction *action = getMainWindow()->actionCollection()->action("toggle_away");
    if(action) {
      action->setText(i18n("Set &Away Globally"));  //this may be wrong if other servers are still away
      action->setIcon("konversationaway");
    }
  }

}

bool Server::isAChannel(const QString &check)
{
  return (getChannelTypes().contains(check.at(0)) >0);
}

void Server::addRawLog(bool show)
{
  if(!rawLog) rawLog=getMainWindow()->addRawLog(this);

  connect(this,SIGNAL (serverOnline(bool)),rawLog,SLOT (serverOnline(bool)) );

  // bring raw log to front since the main window does not do this for us
  if(show) getMainWindow()->showView(rawLog);
}

// in MDI mode thiis function may only be run from the raw log panel itself!
void Server::closeRawLog()
{
  if(rawLog)
  {
#ifndef USE_MDI
    delete rawLog;
#endif
    rawLog=0;
  }
}

void Server::addChannelListPanel()
{
  if(!channelListPanel)
  {
    channelListPanel = getMainWindow()->addChannelListPanel(this);

    connect(channelListPanel, SIGNAL(refreshChannelList()), this, SLOT(requestChannelList()));
    connect(channelListPanel, SIGNAL(joinChannel(const QString&)), this, SLOT(sendJoinCommand(const QString&)));
    connect(this, SIGNAL(serverOnline(bool)), channelListPanel, SLOT(serverOnline(bool)));
  }
}

void Server::addToChannelList(const QString& channel, int users, const QString& topic)
{
  addChannelListPanel();
  channelListPanel->addToChannelList(channel, users, topic);
}

ChannelListPanel* Server::getChannelListPanel() const { return channelListPanel; }

// in MDI mode this function may only be called from the channel list panel itself!
void Server::closeChannelListPanel()
{
  if(channelListPanel)
  {
#ifndef USE_MDI
    delete channelListPanel;
#endif
    channelListPanel=0;
  }
}

void Server::autoRejoinChannels()
{
  if (channelList.isEmpty()) {
    return;
  }
  QStringList channels;
  QStringList keys;

  for(Channel* ch = channelList.first(); ch; ch = channelList.next()) {
    channels.append(ch->getName());
    keys.append(ch->getKey());
  }

  QString joinString("JOIN "+channels.join(",")+" "+keys.join(","));
  queue(joinString);
}

IdentityPtr Server::getIdentity() const { return m_serverGroup.identity(); }

void Server::setMainWindow(KonversationMainWindow* newMainWindow) { mainWindow=newMainWindow; }
KonversationMainWindow* Server::getMainWindow() const { return mainWindow; }

bool Server::getUseSSL() const
{
  return m_serverGroup.serverByIndex(m_currentServerIndex).SSLEnabled();
}

QString Server::getSSLInfo() const
{
  return static_cast<SSLSocket*>(m_socket)->details();
}

bool Server::connected() { return alreadyConnected; }

void Server::sendMultiServerCommand(const QString& command, const QString& parameter)
{
  emit multiServerCommand(command, parameter);
}

void Server::executeMultiServerCommand(const QString& command, const QString& parameter)
{
  if(command == "away" || command == "back") { //back is the same as away, since paramater is ""
    QString str = KonversationApplication::preferences.getCommandChar() + command;

    if(!parameter.isEmpty() && command == "away") { //you cant have a message with 'back'
      str += " " + parameter;
    }

    Konversation::OutputFilterResult result = outputFilter->parse(getNickname(), str, QString::null);
    queue(result.toServer);
  } else if(command == "msg") {
    sendToAllChannelsAndQueries(parameter);
  } else {
    sendToAllChannelsAndQueries(KonversationApplication::preferences.getCommandChar() + command + " " + parameter);
  }
}

void Server::sendToAllChannelsAndQueries(const QString& text)
{
  // Send a message to all channels we are in
  Channel* channel=channelList.first();

  while(channel)
  {
    channel->sendChannelText(text);
    channel=channelList.next();
  }

  // Send a message to all queries we are in
  Query* query=queryList.first();

  while(query)
  {
    query->sendQueryText(text);
    query=queryList.next();
  }
}

void Server::reconnect() {
  if(!isConnected()) {
    reconnectCounter = 0;
    connectToIRCServer();
  } else {
    getStatusView()->appendServerMessage("Error", i18n("Server already connected."));
  }
}

void Server::connectToNewServer(const QString& server, const QString& port, const QString& password)
{
  KonversationApplication *konvApp = static_cast<KonversationApplication*>(KApplication::kApplication());
  konvApp->quickConnectToServer(server, port,"", password);
}
bool Server::isAway() const {
  return m_isAway;
}
QString Server::awayTime() const
{
  QString retVal;

  if(m_isAway) {
    int diff = QDateTime::currentDateTime().toTime_t() - m_awayTime;
    int num = diff / 3600;

    if(num < 10) {
      retVal = "0" + QString::number(num) + ":";
    } else {
      retVal = QString::number(num) + ":";
    }

    num = (diff % 3600) / 60;

    if(num < 10) {
      retVal += "0";
    }

    retVal += QString::number(num) + ":";

    num = (diff % 3600) % 60;

    if(num < 10) {
      retVal += "0";
    }

    retVal += QString::number(num);
  } else {
    retVal = "00:00:00";
  }

  return retVal;
}

void Server::startAwayTimer()
{
  m_awayTime = QDateTime::currentDateTime().toTime_t();
}

/**
* Given the nickname of nick that is offline (or at least not known to be online),
* returns the addressbook entry (if any) for the nick.
* @param nickname       Desired nickname.  Case insensitive.
* @return               Addressbook entry of the nick or empty if not found.
*/
KABC::Addressee Server::getOfflineNickAddressee(QString& nickname)
{
  if (m_serverISON)
    return m_serverISON->getOfflineNickAddressee(nickname);
  else
    return KABC::Addressee();
}


#include "server.moc"

