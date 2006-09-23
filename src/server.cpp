// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005-2006 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#include <qregexp.h>
#include <qhostaddress.h>
#include <qtextcodec.h>
#include <qdatetime.h>

#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kresolver.h>
#include <ksocketdevice.h>
#include <kaction.h>
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
#include "viewcontainer.h"
#include "statuspanel.h"
#include "rawlog.h"
#include "channellistpanel.h"
#include "scriptlauncher.h"
#include "servergroupsettings.h"
#include "addressbook.h"
#include "serverison.h"
#include "common.h"
#include "notificationhandler.h"
#include "blowfish.h"

#include <config.h>

Server::Server(ViewContainer* viewContainer, int serverGroupId, bool clearQuickServerList)
{
    quickConnect = false;

    m_serverGroup = Preferences::serverGroupById(serverGroupId);

    if (clearQuickServerList)
        m_serverGroup->clearQuickServerList(); // In case we already did a quick connect to this network

    bot = getIdentity()->getBot();
    botPassword = getIdentity()->getPassword();

    init(viewContainer, getIdentity()->getNickname(0), "");
}

Server::Server(ViewContainer* viewContainer,const QString& hostName,const QString& port,
const QString& channel,const QString& _nick, const QString& password,const bool& useSSL)
{
    quickConnect = true;

    QString nick( _nick );

    m_quickServer.setServer(hostName);
    m_quickServer.setPort(port.toInt());
    m_quickServer.setPassword(password);
    m_quickServer.setSSLEnabled(useSSL);

    Konversation::ServerGroupSettingsPtr serverGroupOfServer;

    // If server is in an existing group, use that group (first group if server is in multiple groups)
    if (serverGroupOfServer = Preferences::serverGroupByServer(hostName))
    {
        m_serverGroup = serverGroupOfServer;
        m_serverGroup->clearQuickServerList();
        m_serverGroup->setQuickServerList(m_quickServer);
    }
    else
    {
        m_serverGroup = new Konversation::ServerGroupSettings;
        m_serverGroup->setIdentityId(Preferences::identityByName("Default")->id());
        m_serverGroup->setName(hostName);
        m_serverGroup->addServer(m_quickServer);
    }

    // Happens when we are invoked from an irc:/ url
    if (nick.isEmpty())
        nick = getIdentity()->getNickname(0);

    init (viewContainer, nick, channel);
}

void Server::doPreShellCommand()
{

    QString command = getIdentity()->getShellCommand();
    statusView->appendServerMessage(i18n("Info"),"Running preconfigured command...");

    connect( &preShellCommand,SIGNAL(processExited(KProcess*)),this,SLOT(preShellCommandExited(KProcess*)));

    QStringList commandList = QStringList::split(" ",command);

    for ( QStringList::ConstIterator it = commandList.begin(); it != commandList.end(); ++it )
    {
        preShellCommand << *it;
    }

    if(!preShellCommand.start())                  // Non blocking
    {
        preShellCommandExited(NULL);
    }
}

Server::~Server()
{
    //send queued messages
    kdDebug() << "Server::~Server(" << getServerName() << ")" << endl;
    // Send out the last messages (usually the /QUIT)
    send();

    // Delete helper object.
    delete m_serverISON;
    m_serverISON = 0;
    // clear nicks online
    emit nicksNowOnline(this,QStringList(),true);
    // Make sure no signals get sent to a soon to be dying Server Window
    if (m_socket)
    {
        m_socket->blockSignals(true);
        m_socket->deleteLater();
    }

    closeRawLog();
    closeChannelListPanel();

    channelList.setAutoDelete(true);
    channelList.clear();

    queryList.setAutoDelete(true);
    queryList.clear();

    // Delete all the NickInfos and ChannelNick structures.
    m_allNicks.clear();
    ChannelMembershipMap::ConstIterator it;

    for ( it = m_joinedChannels.begin(); it != m_joinedChannels.end(); ++it )
        delete it.data();
    m_joinedChannels.clear();

    for ( it = m_unjoinedChannels.begin(); it != m_unjoinedChannels.end(); ++it )
        delete it.data();
    m_unjoinedChannels.clear();

    m_queryNicks.clear();

    // notify KonversationApplication that this server is gone
    emit deleted(this);
}

void Server::init(ViewContainer* viewContainer, const QString& nick, const QString& channel)
{
    m_processingIncoming = false;
    m_identifyMsg = false;
    m_currentServerIndex = 0;
    m_tryReconnect = true;
    autoJoin = false;
    tryNickNumber = 0;
    reconnectCounter = 0;
    currentLag = -1;
    rawLog = 0;
    channelListPanel = 0;
    alreadyConnected = false;
    rejoinChannels = false;
    connecting = false;
    m_serverISON = 0;
    m_isAway = false;
    m_socket = 0;
    m_autoIdentifyLock = false;
    keepViewsOpenAfterQuit = false;
    reconnectAfterQuit = false;

    // TODO fold these into a QMAP, and these need to be reset to RFC values if this server object is reused.
    serverNickPrefixModes = "ovh";
    serverNickPrefixes = "@+%";
    channelPrefixes = "#&";

    timerInterval = 1;

    setName(QString("server_" + m_serverGroup->name()).ascii());
    setViewContainer(viewContainer);
    statusView = getViewContainer()->addStatusView(this);
    statusView->setNotificationsEnabled(m_serverGroup->enableNotifications());
    setNickname(nick);
    obtainNickInfo(getNickname());

    if(Preferences::rawLog())
        addRawLog(false);

    inputFilter.setServer(this);
    outputFilter = new Konversation::OutputFilter(this);
    m_scriptLauncher = new ScriptLauncher(this);

    // don't delete items when they are removed
    channelList.setAutoDelete(false);
    // For /msg query completion
    completeQueryPosition = 0;

    updateAutoJoin(channel);

    if(!getIdentity()->getShellCommand().isEmpty())
        doPreShellCommand();
    else
        connectToIRCServer();

    initTimers();

    if(getIdentity()->getShellCommand().isEmpty())
        connectSignals();

    emit serverOnline(false);
    emit connectionChangedState(this, SSDisconnected);
}

void Server::initTimers()
{
    notifyTimer.setName("notify_timer");
    incomingTimer.setName("incoming_timer");
    outgoingTimer.setName("outgoing_timer");
}

void Server::connectSignals()
{

    // Timers
    connect(&incomingTimer, SIGNAL(timeout()), this, SLOT(processIncomingData()));
    connect(&outgoingTimer, SIGNAL(timeout()), this, SLOT(send()));
    connect(&unlockTimer, SIGNAL(timeout()), this, SLOT(unlockSending()));
    connect(&notifyTimer, SIGNAL(timeout()), this, SLOT(notifyTimeout()));
    connect(&m_pingResponseTimer, SIGNAL(timeout()), this, SLOT(updateLongPongLag()));

    // OutputFilter
    connect(outputFilter, SIGNAL(requestDccSend()), this,SLOT(requestDccSend()));
    connect(outputFilter, SIGNAL(requestDccSend(const QString&)), this, SLOT(requestDccSend(const QString&)));
    connect(outputFilter, SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(outputFilter, SIGNAL(reconnectServer()), this, SLOT(reconnect()));
    connect(outputFilter, SIGNAL(disconnectServer()), this, SLOT(disconnect()));
    connect(outputFilter, SIGNAL(openDccSend(const QString &, KURL)), this, SLOT(addDccSend(const QString &, KURL)));
    connect(outputFilter, SIGNAL(requestDccChat(const QString &)), this, SLOT(requestDccChat(const QString &)));
    connect(outputFilter, SIGNAL(connectToServer(const QString&, const QString&, const QString&)),
        this, SLOT(connectToNewServer(const QString&, const QString&, const QString&)));
    connect(outputFilter, SIGNAL(connectToServerGroup(const QString&)),
        this, SLOT(connectToServerGroup(const QString&)));
    connect(outputFilter, SIGNAL(sendToAllChannels(const QString&)), this, SLOT(sendToAllChannels(const QString&)));
    connect(outputFilter, SIGNAL(banUsers(const QStringList&,const QString&,const QString&)),
        this, SLOT(requestBan(const QStringList&,const QString&,const QString&)));
    connect(outputFilter, SIGNAL(unbanUsers(const QString&,const QString&)),
        this, SLOT(requestUnban(const QString&,const QString&)));
    connect(outputFilter, SIGNAL(openRawLog(bool)), this, SLOT(addRawLog(bool)));
    connect(outputFilter, SIGNAL(closeRawLog()), this, SLOT(closeRawLog()));

   // ViewContainer
    connect(this, SIGNAL(showView(ChatWindow*)), getViewContainer(), SLOT(showView(ChatWindow*)));
    connect(this, SIGNAL(addDccPanel()), getViewContainer(), SLOT(addDccPanel()));
    connect(this, SIGNAL(addDccChat(const QString&,const QString&,const QString&,const QStringList&,bool)),
        getViewContainer(), SLOT(addDccChat(const QString&,const QString&,const QString&,const QStringList&,bool)) );
    connect(this, SIGNAL(serverLag(Server*, int)), getViewContainer(), SIGNAL(updateStatusBarLagLabel(Server*, int)));
    connect(this, SIGNAL(tooLongLag(Server*, int)), getViewContainer(), SIGNAL(setStatusBarLagLabelTooLongLag(Server*, int)));
    connect(this, SIGNAL(resetLag()), getViewContainer(), SIGNAL(resetStatusBarLagLabel()));
    connect(outputFilter, SIGNAL(showView(ChatWindow*)), getViewContainer(), SLOT(showView(ChatWindow*)));
    connect(outputFilter, SIGNAL(openKonsolePanel()), getViewContainer(), SLOT(addKonsolePanel()));
    connect(outputFilter, SIGNAL(openChannelList(const QString&, bool)), getViewContainer(), SLOT(openChannelList(const QString&, bool)));
    connect(outputFilter, SIGNAL(closeDccPanel()), getViewContainer(), SLOT(closeDccPanel()));
    connect(outputFilter, SIGNAL(addDccPanel()), getViewContainer(), SLOT(addDccPanel()));
    connect(&inputFilter, SIGNAL(addDccChat(const QString&,const QString&,const QString&,const QStringList&,bool)),
        getViewContainer(), SLOT(addDccChat(const QString&,const QString&,const QString&,const QStringList&,bool)) );

    // Inputfilter
    connect(&inputFilter, SIGNAL(welcome(const QString&)), this, SLOT(connectionEstablished(const QString&)));
    connect(&inputFilter, SIGNAL(notifyResponse(const QString&)), this, SLOT(notifyResponse(const QString&)));
    connect(&inputFilter, SIGNAL(addDccGet(const QString&, const QStringList&)),
        this, SLOT(addDccGet(const QString&, const QStringList&)));
    connect(&inputFilter, SIGNAL(resumeDccGetTransfer(const QString&, const QStringList&)),
        this, SLOT(resumeDccGetTransfer(const QString&, const QStringList&)));
    connect(&inputFilter, SIGNAL(resumeDccSendTransfer(const QString&, const QStringList&)),
        this, SLOT(resumeDccSendTransfer(const QString&, const QStringList&)));
    connect(&inputFilter, SIGNAL(userhost(const QString&,const QString&,bool,bool)),
        this, SLOT(userhost(const QString&,const QString&,bool,bool)) );
    connect(&inputFilter, SIGNAL(topicAuthor(const QString&,const QString&)),
        this, SLOT(setTopicAuthor(const QString&,const QString&)) );
    connect(&inputFilter, SIGNAL(endOfWho(const QString&)),
        this, SLOT(endOfWho(const QString&)) );
    connect(&inputFilter, SIGNAL(invitation(const QString&,const QString&)),
        this,SLOT(invitation(const QString&,const QString&)) );
    connect(&inputFilter, SIGNAL(addToChannelList(const QString&, int, const QString& )),
        this, SLOT(addToChannelList(const QString&, int, const QString& )));
    connect(&inputFilter, SIGNAL(away()), this, SLOT(away()));
    connect(&inputFilter, SIGNAL(unAway()), this, SLOT(unAway()));

    // Status View
    connect(this, SIGNAL(serverOnline(bool)), statusView, SLOT(serverOnline(bool)));

    // Scripts
    connect(outputFilter, SIGNAL(launchScript(const QString&, const QString&)),
        m_scriptLauncher, SLOT(launchScript(const QString&, const QString&)));
    connect(m_scriptLauncher, SIGNAL(scriptNotFound(const QString&)),
        this, SLOT(scriptNotFound(const QString&)));
    connect(m_scriptLauncher, SIGNAL(scriptExecutionError(const QString&)),
        this, SLOT(scriptExecutionError(const QString&)));

}

QString Server::getServerName() const
{
    return m_serverGroup->serverByIndex(m_currentServerIndex).server();
}

int Server::getPort() const
{
    return m_serverGroup->serverByIndex(m_currentServerIndex).port();
}

QString Server::getServerGroup() const
{
    return m_serverGroup->name();
}

int Server::getLag()  const
{
    return currentLag;
}

bool Server::getAutoJoin()  const
{
    return autoJoin;
}

void Server::setAutoJoin(bool on)
{
    autoJoin = on;
}

QString Server::getAutoJoinChannel() const
{
    return autoJoinChannel;
}

void Server::setAutoJoinChannel(const QString &channel)
{
    autoJoinChannel = channel;
}

QString Server::getAutoJoinChannelKey() const
{
    return autoJoinChannelKey;
}

void Server::setAutoJoinChannelKey(const QString &key)
{
    autoJoinChannelKey = key;
}

bool Server::isConnected() const
{
    if (!m_socket)
        return false;

    return (m_socket->state() == KNetwork::KClientSocketBase::Connected);
}

bool Server::isConnecting() const
{
    return connecting;
}

void Server::preShellCommandExited(KProcess* proc)
{

    if (proc && proc->normalExit())
        statusView->appendServerMessage(i18n("Info"),"Process executed successfully!");
    else
        statusView->appendServerMessage(i18n("Warning"),"There was a problem while executing the command!");

    connectToIRCServer();
    connectSignals();
}

void Server::connectToIRCServer()
{
    deliberateQuit = false;
    keepViewsOpenAfterQuit = false;
    reconnectAfterQuit = false;
    m_autoIdentifyLock = false;

    connecting = true;

    ownIpByUserhost = QString();

    outputBuffer.clear();

    if(m_socket)
        m_socket->blockSignals(false);

    // prevent sending queue until server has sent something or the timeout is through
    lockSending();

    if (!isConnected())
    {
        // This is needed to support server groups with mixed SSL and nonSSL servers
        delete m_socket;
        tryNickNumber = 0;

        // connect() will do a async lookup too
        if(!m_serverGroup->serverByIndex(m_currentServerIndex).SSLEnabled())
        {
            m_socket = new KNetwork::KBufferedSocket(QString::null, QString::null, 0L, "serverSocket");
            connect(m_socket,SIGNAL (connected(const KResolverEntry&)),this,SLOT (ircServerConnectionSuccess()));
        }
        else
        {
            m_socket = new SSLSocket(getViewContainer()->getWindow(), 0L, "serverSSLSocket");
            connect(m_socket,SIGNAL (sslInitDone()),this,SLOT (ircServerConnectionSuccess()));
            connect(m_socket,SIGNAL (sslFailure(QString)),this,SIGNAL(sslInitFailure()));
            connect(m_socket,SIGNAL (sslFailure(QString)),this,SLOT(sslError(QString)));
        }

        connect(m_socket,SIGNAL (hostFound()),this,SLOT(lookupFinished()));
        connect(m_socket,SIGNAL (gotError(int)),this,SLOT (broken(int)) );
        connect(m_socket,SIGNAL (readyRead()),this,SLOT (incoming()) );
        connect(m_socket,SIGNAL (readyWrite()),this,SLOT (send()) );
        connect(m_socket,SIGNAL (closed()),this,SLOT(closed()));

        m_socket->connect(m_serverGroup->serverByIndex(m_currentServerIndex).server(),
            QString::number(m_serverGroup->serverByIndex(m_currentServerIndex).port()));

        // set up the connection details
        setPrefixes(serverNickPrefixModes, serverNickPrefixes);
        statusView->appendServerMessage(i18n("Info"),i18n("Looking for server %1:%2...")
            .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server())
            .arg(m_serverGroup->serverByIndex(m_currentServerIndex).port()));
        // reset InputFilter (auto request info, /WHO request info)
        inputFilter.reset();
        emit connectionChangedState(this, SSConnecting);
    }
}

void Server::showSSLDialog()
{
    static_cast<SSLSocket*>(m_socket)->showInfoDialog();
}

// set available channel types according to 005 RPL_ISUPPORT
void Server::setChannelTypes(const QString &pre)
{
    channelPrefixes = pre;
}

QString Server::getChannelTypes() const
{
    return channelPrefixes;
}

// set user mode prefixes according to non-standard 005-Reply (see inputfilter.cpp)
void Server::setPrefixes(const QString &modes, const QString& prefixes)
{
    // NOTE: serverModes is QString::null, if server did not supply the
    // modes which relates to the network's nick-prefixes
    serverNickPrefixModes = modes;
    serverNickPrefixes = prefixes;
}

// return a nickname without possible mode character at the beginning
void Server::mangleNicknameWithModes(QString& nickname,bool& isAdmin,bool& isOwner,
bool& isOp,bool& isHalfop,bool& hasVoice)
{
    isAdmin = false;
    isOwner = false;
    isOp = false;
    isHalfop = false;
    hasVoice = false;

    int modeIndex;

    if(nickname.isEmpty())
    {
        return;
    }

    while((modeIndex = serverNickPrefixes.find(nickname[0])) != -1)
    {
        if(nickname.isEmpty())
            return;
        nickname = nickname.mid(1);
        // cut off the prefix
        bool recognisedMode = false;
        // determine, whether status is like op or like voice
        while((modeIndex)<int(serverNickPrefixes.length()) && !recognisedMode)
        {
            switch(serverNickPrefixes[modeIndex].latin1())
            {
                case '*':                         // admin (EUIRC)
                {
                    isAdmin = true;
                    recognisedMode = true;
                    break;
                }
                case '&':                         // admin (unrealircd)
                {
                    isAdmin = true;
                    recognisedMode = true;
                    break;
                }
                case '!':                         // channel owner (RFC2811)
                {
                    isOwner = true;
                    recognisedMode = true;
                    break;
                }
                case '~':                         // channel owner (unrealircd)
                {
                    isOwner = true;
                    recognisedMode = true;
                    break;
                }
                case '@':                         // channel operator (RFC1459)
                {
                    isOp = true;
                    recognisedMode = true;
                    break;
                }
                case '%':                         // halfop
                {
                    isHalfop = true;
                    recognisedMode = true;
                    break;
                }
                case '+':                         // voiced (RFC1459)
                {
                    hasVoice = true;
                    recognisedMode = true;
                    break;
                }
                default:
                {
                    ++modeIndex;
                    break;
                }
            }                                     //switch to recognise the mode.
        }                                         // loop through the modes to find one recognised
    }                                             // loop through the name
}

void Server::lookupFinished()
{
    // error during lookup
    if(m_serverGroup->serverByIndex(m_currentServerIndex).SSLEnabled() && m_socket->status())
    {
        // inform user about the error
        statusView->appendServerMessage(i18n("Error"),i18n("Server %1 not found.  %2")
            .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server())
            .arg(m_socket->errorString(m_socket->error())));

        m_socket->resetStatus();
        // prevent retrying to connect
        m_tryReconnect = false;
        // broken connection
        broken(m_socket->error());
    }
    else
    {
        statusView->appendServerMessage(i18n("Info"),i18n("Server found, connecting..."));
    }
}

void Server::ircServerConnectionSuccess()
{
    reconnectCounter = 0;
    Konversation::ServerSettings serverSettings = m_serverGroup->serverByIndex(m_currentServerIndex);

    connect(this, SIGNAL(nicknameChanged(const QString&)), statusView, SLOT(setNickname(const QString&)));
    statusView->appendServerMessage(i18n("Info"),i18n("Connected; logging in..."));

    QString connectString = "USER " +
        getIdentity()->getIdent() +
        " 8 * :" +                                // 8 = +i; 4 = +w
        getIdentity()->getRealName();

    if(!serverSettings.password().isEmpty())
        queueAt(0, "PASS " + serverSettings.password());

    queueAt(1,"NICK "+getNickname());
    queueAt(2,connectString);

    emit nicknameChanged(getNickname());

    m_socket->enableRead(true);

    // wait at most 2 seconds for server to send something before sending the queue ourselves
    unlockTimer.start(2000);

    connecting = false;
}

void Server::broken(int state)
{
    m_socket->enableRead(false);
    m_socket->enableWrite(false);
    m_socket->blockSignals(true);

    alreadyConnected = false;
    connecting = false;
    m_autoIdentifyLock = false;
    outputBuffer.clear();

    notifyTimer.stop();
    m_pingResponseTimer.stop();
    inputFilter.setLagMeasuring(false);
    currentLag = -1;

    // HACK Only show one nick change dialog at connection time
    if(getStatusView())
    {
        KDialogBase* nickChangeDialog = dynamic_cast<KDialogBase*>(
                getStatusView()->child("NickChangeDialog", "KInputDialog"));

        if(nickChangeDialog) {
            nickChangeDialog->cancel();
        }
    }

    emit connectionChangedState(this, SSDisconnected);

    emit resetLag();
    emit serverOnline(false);
    emit nicksNowOnline(this,QStringList(),true);

    kdDebug() << "Connection broken (Socket fd " << m_socket->socketDevice()->socket() << ") " << state << "!" << endl;


    if (!deliberateQuit)
    {
        static_cast<KonversationApplication*>(kapp)->notificationHandler()->connectionFailure(statusView, m_serverGroup->serverByIndex(m_currentServerIndex).server());

        ++reconnectCounter;

        if (Preferences::autoReconnect() && reconnectCounter <= Preferences::reconnectCount())
        {
            updateAutoJoin();

            QString error = i18n("Connection to Server %1 lost: %2. Trying to reconnect.")
                .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server())
                .arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));

            statusView->appendServerMessage(i18n("Error"), error);

            QTimer::singleShot(5000, this, SLOT(connectToIRCServer()));
            rejoinChannels = true;
        }
        else if (!Preferences::autoReconnect() || reconnectCounter > Preferences::reconnectCount())
        {
            updateAutoJoin();

            QString error = i18n("Connection to Server %1 failed: %2.")
                .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server())
                .arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));

            statusView->appendServerMessage(i18n("Error"),error);
            reconnectCounter = 0;

            // If this iteration has occurred before we ever were connected,
            // the channel list will be empty and we want to do autojoin.
            // If the channel list is not empty, however, rejoin the old
            // channels.
            if (channelList.isEmpty())
                rejoinChannels = false;
            else
                rejoinChannels = true;

            // Broke on a temp. server, so remove it from serverList; otherwise increment the index
            if (!m_serverGroup->quickServerList().isEmpty())
            {
                m_quickServer =  m_serverGroup->quickServerList().first();
                m_serverGroup->clearQuickServerList();
            }
            else
            {
                m_currentServerIndex++;
            }

            // The next server in the list is actually identical to our original quickserver, so skip it
            if (m_serverGroup->serverByIndex(m_currentServerIndex)==m_quickServer)
            {
                m_currentServerIndex++;
            }

            if(m_currentServerIndex < m_serverGroup->serverList().count())
            {
                error = i18n("Trying server %1 instead.")
                    .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server());
                statusView->appendServerMessage(i18n("Error"),error );

                connectToIRCServer();
            }
            else
            {
                if (Preferences::autoReconnect())
                {
                    error = i18n("Waiting for 2 minutes before another reconnection attempt...");
                    statusView->appendServerMessage(i18n("Info"),error);
                    m_currentServerIndex = 0;
                    QTimer::singleShot(2*60*1000, this, SLOT(connectToIRCServer()));
                }
            }
        }
        else
        {
            QString error = i18n("Connection to Server %1 failed: %2.")
                .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server());
            statusView->appendServerMessage(i18n("Error"),error);
        }
    }                                             // If we quit the connection with the server
    else
    {
        if (keepViewsOpenAfterQuit)
        {
            keepViewsOpenAfterQuit = false;

            statusView->appendServerMessage(i18n("Info"),i18n("Disconnected from server."));

            if (reconnectAfterQuit)
            {
                reconnectAfterQuit = false;

                updateAutoJoin();
                rejoinChannels = true;
                reconnectCounter = 0;
                QTimer::singleShot(3000, this, SLOT(connectToIRCServer()));

            }
        }
        else
        {
            m_serverGroup->clearQuickServerList();
            getViewContainer()->serverQuit(this);
        }
    }
}

void Server::sslError(const QString& reason)
{
    QString error = i18n("Could not connect to %1:%2 using SSL encryption.Maybe the server does not support SSL, or perhaps you have the wrong port? %3")
        .arg(m_serverGroup->serverByIndex(m_currentServerIndex).server())
        .arg(m_serverGroup->serverByIndex(m_currentServerIndex).port())
        .arg(reason);
    statusView->appendServerMessage(i18n("SSL Connection Error"),error);
    m_tryReconnect = false;

}

// Will be called from InputFilter as soon as the Welcome message was received
void Server::connectionEstablished(const QString& ownHost)
{
    // Some servers don't include the userhost in RPL_WELCOME, so we
    // need to use RPL_USERHOST to get ahold of our IP later on
    if (!ownHost.isEmpty())
        KNetwork::KResolver::resolveAsync(this,SLOT(gotOwnResolvedHostByWelcome(KResolverResults)),ownHost,"0");

    emit serverOnline(true);
    emit connectionChangedState(this, SSConnected);

    if(!alreadyConnected)
    {
        alreadyConnected=true;
        // Make a helper object to build ISON (notify) list and map offline nicks to addressbook.
        // TODO: Give the object a kick to get it started?
        m_serverISON = new ServerISON(this);
        // get first notify very early
        startNotifyTimer(1000);
        // Register with services
        registerWithServices();
        // get own ip by userhost
        requestUserhost(nickname);

        // Start the PINGPONG match
        QTimer::singleShot(1000 /*1 sec*/, this, SLOT(sendPing()));

        // Recreate away state if we were set away prior to a reconnect.
        if (isAway())
        {
            // Correct server's beliefs about its away state.
            m_isAway = false;
            QString awayReason = m_awayReason.isEmpty() ? i18n("Gone away for now.") : m_awayReason;
            QString command(Preferences::commandChar() + "AWAY " + awayReason);
            Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),command, QString::null);
            queue(result.toServer);
        }

        if (rejoinChannels)
        {
            rejoinChannels = false;
            autoRejoinChannels();
        }
    }
    else
    {
        kdDebug() << "alreadyConnected == true! How did that happen?" << endl;
    }
}

void Server::registerWithServices()
{
    if(!botPassword.isEmpty() && !bot.isEmpty() && !m_autoIdentifyLock)
    {
        queue("PRIVMSG "+bot+" :identify "+botPassword);

        // Set lock to prevent a second auto-identify attempt
        // Lock is unset if we nickchange
        m_autoIdentifyLock = true;
    }
}

QCString Server::getKeyForRecipient(const QString& recipient) const
{
    return keyMap[recipient];
}

void Server::setKeyForRecipient(const QString& recipient, const QCString& key)
{
    keyMap[recipient] = key;
}

void Server::gotOwnResolvedHostByWelcome(KResolverResults res)
{
    if ( res.error() == KResolver::NoError && !res.isEmpty() )
        ownIpByWelcome = res.first().address().nodeName();
    else
        kdDebug() << "Server::gotOwnResolvedHostByWelcome(): Got error: " << ( int )res.error() << endl;
}

void Server::quitServer()
{
    QString command(Preferences::commandChar()+"QUIT");
    Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),command, QString::null);
    queue(result.toServer);
    if (m_socket) m_socket->enableRead(false);
}

void Server::notifyAction(const QString& nick)
{
    // parse wildcards (toParse,nickname,channelName,nickList,parameter)
    QString out = parseWildcards(Preferences::notifyDoubleClickAction(),
        getNickname(),
        QString::null,
        QString::null,
        nick,
        QString::null);

    // Send all strings, one after another
    QStringList outList = QStringList::split('\n',out);
    for(unsigned int index=0; index<outList.count(); ++index)
    {
        Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),outList[index],QString::null);
        queue(result.toServer);
    }                                             // endfor
}

void Server::notifyResponse(const QString& nicksOnline)
{
    bool nicksOnlineChanged = false;
    // Create a case correct nick list from the notification reply
    QStringList nickList = QStringList::split(' ',nicksOnline);

    QStringList::iterator it;
    QStringList::iterator itEnd = nickList.end();

    // Any new watched nicks online?
    for(it = nickList.begin(); it != itEnd; ++it)
    {
        QString nickname = (*it);

        if (!isNickOnline(nickname))
        {
            setWatchedNickOnline(nickname);
            nicksOnlineChanged = true;
        }
    }

    // Create a lower case nick list from the notification reply
    QStringList nickLowerList = QStringList::split(' ',nicksOnline.lower());
    // Get ISON list from preferences and addressbook.
    QString watchlist = getISONListString();
    // Create a case correct nick list from the watch list.
    QStringList watchList = QStringList::split(' ',watchlist);
    itEnd = watchList.end();

    // Any watched nicks now offline?
    for(it = watchList.begin(); it != itEnd; ++it)
    {
        QString lcNickName = (*it).lower();
        if (nickLowerList.find(lcNickName) == nickLowerList.end())
        {
            QString nickname = (*it);
            if (setNickOffline(nickname))
                nicksOnlineChanged = true;
        }
    }

    // Note: The list emitted in this signal does not include nicks in joined channels.
    emit nicksNowOnline(this, nickList, nicksOnlineChanged);

    // Next round
    startNotifyTimer();
}

void Server::startNotifyTimer(int msec)
{
    // make sure the timer gets started properly in case we have reconnected
    notifyTimer.stop();

    if(msec == 0)
        msec = Preferences::notifyDelay()*1000;

    // start the timer in one shot mode
    notifyTimer.start(msec, true);
}

void Server::notifyTimeout()
{
    // Notify delay time is over, send ISON request if desired
    if(Preferences::useNotify())
    {
        // But only if there actually are nicks in the notify list
        QString list = getISONListString();

        if(!list.isEmpty())
        {
            queue("ISON "+list);
        }
    }
}

void Server::autoCommandsAndChannels()
{
    if (!m_serverGroup->connectCommands().isEmpty())
    {
        QString connectCommands = m_serverGroup->connectCommands();

        if (!getNickname().isEmpty())
            connectCommands.replace("%nick", getNickname());

        QStringList connectCommandsList = QStringList::split(";", connectCommands);
        QStringList::iterator iter;

        for(iter = connectCommandsList.begin(); iter != connectCommandsList.end(); ++iter)
        {
            QString output(*iter);
            output = output.simplifyWhiteSpace();
            Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),output,QString::null);
            queue(result.toServer);
        }
    }

    if (getAutoJoin() && !rejoinChannels)
        queue(getAutoJoinCommand());
}

QString Server::getAutoJoinCommand() const
{
    // Multichannel joins
    QStringList channels = QStringList::split(' ',autoJoinChannel);
    QStringList keys = QStringList::split(' ',autoJoinChannelKey);

    QString autoString("JOIN "+channels.join(",")+' '+keys.join(","));

    return autoString;
}

QString Server::getNextNickname()
{
    QString newNick = getIdentity()->getNickname(++tryNickNumber);

    if (newNick.isNull())
    {
        QString inputText = i18n("No nicknames from the \"%1\" identity were accepted by the connection \"%2\".\nPlease enter a new one or press Cancel to disconnect:").arg(getIdentity()->getName()).arg(getServerGroup());
        newNick = KInputDialog::getText(i18n("Nickname error"), inputText,
                                        QString::null, 0, getStatusView(), "NickChangeDialog");
    }

    return newNick;
}

void Server::processIncomingData()
{
    incomingTimer.stop();

    if(!inputBuffer.isEmpty() && !m_processingIncoming)
    {
        m_processingIncoming = true;
        QString front(inputBuffer.front());
        inputBuffer.pop_front();
        if(rawLog)
        {
            QString toRaw = front;
            rawLog->appendRaw("&gt;&gt; " + toRaw.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));
        }
        inputFilter.parseLine(front);
        m_processingIncoming = false;

        if(!inputBuffer.isEmpty())
        {
            incomingTimer.start(0);
        }
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
    if(m_serverGroup->serverByIndex(m_currentServerIndex).SSLEnabled())
        emit sslConnected(this);

    // We read all available bytes here because readyRead() signal will be emitted when there is new data
    // else we will stall when displaying MOTD etc.
    int max_bytes = m_socket->bytesAvailable();

    QByteArray buffer(max_bytes+1);
    int len = 0;

    // Read at max "max_bytes" bytes into "buffer"
    len = m_socket->readBlock(buffer.data(),max_bytes);

    if( len <=0 && m_serverGroup->serverByIndex(m_currentServerIndex).SSLEnabled() )
        return;

    if( len <= 0 ) // Zero means buffer is empty which shouldn't happen because readyRead signal is emitted
    {
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

    // remember the incomplete line (split by packets)
    inputBufferIncomplete = qcsBuffer.right(qcsBuffer.length()-lastLFposition-1);

    while(!qcsBufferLines.isEmpty())
    {
        // Pre parsing is needed in case encryption/decryption is needed
        // BEGIN set channel encoding if specified
        QString senderNick;
        bool isServerMessage = false;
        QString channelKey;
        QTextCodec* codec = getIdentity()->getCodec();
        QCString front = qcsBufferLines.front();

        QStringList lineSplit = QStringList::split(" ",codec->toUnicode(front));

        if( lineSplit.count() >= 1 )
        {
            if( lineSplit[0][0] == ':' )          // does this message have a prefix?
            {
                if( !lineSplit[0].contains('!') ) // is this a server(global) message?
                    isServerMessage = true;
                else
                    senderNick = lineSplit[0].mid(1, lineSplit[0].find('!')-1);

                lineSplit.pop_front();            // remove prefix
            }
        }

        // BEGIN pre-parse to know where the message belongs to
        QString command = lineSplit[0].lower();
        if( isServerMessage )
        {
            if( lineSplit.count() >= 3 )
            {
                if( command == "332" )            // RPL_TOPIC
                    channelKey = lineSplit[2];
                if( command == "372" )            // RPL_MOTD
                    channelKey = ":server";
            }
        }
        else                                      // NOT a global message
        {
            if( lineSplit.count() >= 2 )
            {
                // query
                if( ( command == "privmsg" ||
                    command == "notice"  ) &&
                    lineSplit[1] == getNickname() )
                {
                    channelKey = senderNick;
                }
                // channel message
                else if( command == "privmsg" ||
                    command == "notice"  ||
                    command == "join"    ||
                    command == "kick"    ||
                    command == "part"    ||
                    command == "topic"   )
                {
                    channelKey = lineSplit[1];
                }
            }
        }
        // END pre-parse to know where the message belongs to

        // Decrypt if necessary
        if(command == "privmsg")
            Konversation::decrypt(channelKey,front,this);
        else if(command == "332")
        {
            Konversation::decryptTopic(channelKey,front,this);
        }

        bool isUtf8 = Konversation::isUtf8(front);

        if( isUtf8 )
            inputBuffer << QString::fromUtf8(front);
        else
        {
            // check setting
            QString channelEncoding;
            if( !channelKey.isEmpty() )
            {
                channelEncoding = Preferences::channelEncoding(getServerGroup(), channelKey);
            }
            // END set channel encoding if specified

            if( !channelEncoding.isEmpty() )
                codec = Konversation::IRCCharsets::self()->codecForName(channelEncoding);

            // if channel encoding is utf-8 and the string is definitely not utf-8
            // then try latin-1
            if ( !isUtf8 && codec->mibEnum() == 106 )
                codec = QTextCodec::codecForMib( 4 /* iso-8859-1 */ );

            inputBuffer << codec->toUnicode(front);
        }
        qcsBufferLines.pop_front();
    }

    // refresh lock timer if it was still locked
    if( !sendUnlocked )
        unlockSending();

    if( !incomingTimer.isActive() && !m_processingIncoming )
        incomingTimer.start(0);
}

void Server::queue(const QString& buffer)
{
    // Only queue lines if we are connected
    if(!buffer.isEmpty())
    {
        outputBuffer.append(buffer);

        timerInterval*=2;

        if(!outgoingTimer.isActive())
        {
            outgoingTimer.start(1);
        }
    }
}

void Server::queueAt(uint pos,const QString& buffer)
{
    if(buffer.isEmpty())
        return;

    if(pos < outputBuffer.count())
    {
        outputBuffer.insert(outputBuffer.at(pos),buffer);

        timerInterval*=2;
    }
    else
    {
        queue(buffer);
    }

    if(!outgoingTimer.isActive())
    {
        outgoingTimer.start(1);
    }
}

void Server::queueList(const QStringList& buffer)
{
    // Only queue lines if we are connected
    if(!buffer.isEmpty())
    {
        for(unsigned int i=0;i<buffer.count();i++)
        {
            outputBuffer.append(*buffer.at(i));
            timerInterval*=2;
        }                                         // for

        if(!outgoingTimer.isActive())
        {
            outgoingTimer.start(1);
        }
    }
}

/** Calculate how long this message premable will be.

    This is necessary because the irc server will clip messages so that the
    client receives a maximum of 512 bytes at once.
*/
int Server::getPreLength(const QString& command, const QString& dest)
{
    int hostMaskLength=getNickInfo(nickname)->getHostmask().length();

    //:Sho_!i=ehs1@konversation/developer/hein PRIVMSG #konversation :and then back to it

    //<colon>$nickname<!>$hostmask<space>$command<space>$destination<space><colon>$message<cr><lf>
    int x= 512 - 8 - (nickname.length() + hostMaskLength + command.length() + dest.length());

    return x;
}

void Server::send()
{
    // Check if we are still online
    if(!isConnected() || outputBuffer.isEmpty())
    {
        return;
    }

    if(!outputBuffer.isEmpty() && sendUnlocked)
    {
        // NOTE: It's important to add the linefeed here, so the encoding process does not trash it
        //       for some servers.
        QString outputLine=outputBuffer[0]+'\n';
        QStringList outputLineSplit=QStringList::split(" ",outputBuffer[0]);
        outputBuffer.pop_front();

        //Lets cache the uppercase command so we don't miss or reiterate too much
        QString outboundCommand(outputLineSplit[0].upper());

        // remember the first arg of /WHO to identify responses
        if(!outputLineSplit.isEmpty() && outboundCommand=="WHO")
        {
            if(outputLineSplit.count()>=2)
                inputFilter.addWhoRequest(outputLineSplit[1]);
            else // no argument (servers recognize it as "*")
                inputFilter.addWhoRequest("*");
        }
        // Don't reconnect if we WANT to quit
        else if(outboundCommand=="QUIT")
        {
            deliberateQuit = true;
        }

        // wrap server socket into a stream
        QTextStream serverStream;

        serverStream.setDevice(m_socket);

        // set channel encoding if specified
        QString channelCodecName;

        if(outputLineSplit.count()>2) //"for safe" <-- so no encoding if no data
        {
        if(outboundCommand=="PRIVMSG"   //PRIVMSG target :message
           || outboundCommand=="NOTICE" //NOTICE target :message
           || outboundCommand=="KICK"   //KICK target :message
           || outboundCommand=="PART"   //PART target :message
           || outboundCommand=="TOPIC"  //TOPIC target :message
            )
            { //KV << outboundCommand << " queued" <<endl;
                channelCodecName=Preferences::channelEncoding(getServerGroup(),outputLineSplit[1]);
            }
        }

        // init stream props
        serverStream.setEncoding(QTextStream::Locale);
        QTextCodec* codec = getIdentity()->getCodec();

        if(!channelCodecName.isEmpty())
        {
            codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);
        }

        // convert encoded data to IRC ascii only when we don't have the same codec locally
        if(QString(QTextCodec::codecForLocale()->name()).lower() != QString(codec->name()).lower())
        {
            serverStream.setCodec(codec);
        }

        // Blowfish
        if(outboundCommand=="PRIVMSG" || outboundCommand=="TOPIC")
        {
            Konversation::encrypt(outputLineSplit[1],outputLine,this);
        }

        serverStream << outputLine;
        if(rawLog) rawLog->appendRaw("&lt;&lt; " + outputLine.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));

        // detach server stream
        serverStream.unsetDevice();
    }

    if(outputBuffer.isEmpty()) {
        outgoingTimer.stop();
        timerInterval = 1;
    }

    // Flood-Protection
    if(timerInterval > 1)
    {
        int time;
        timerInterval /= 2;

        if(timerInterval > 40)
        {
            time = 4000;
        }
        else
        {
            time = 1;
        }

        outgoingTimer.changeInterval(time);
    }
}

void Server::closed()
{
    broken(m_socket->error());
}

void Server::dcopRaw(const QString& command)
{
    if(command.startsWith(Preferences::commandChar()))
    {
        queue(command.section(Preferences::commandChar(), 1));
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
        class Query* query=getQueryByName(target);
        if(query==0)
        {
            NickInfoPtr nickinfo = obtainNickInfo(target);
            query=addQuery(nickinfo, true);
        }
        if(query)
        {
            if(!command.isEmpty())
                query->sendQueryText(command);
            else
            {
                query->adjustFocus();
                getViewContainer()->getWindow()->show();
                KWin::demandAttention(getViewContainer()->getWindow()->winId());
                KWin::activateWindow(getViewContainer()->getWindow()->winId());
            }
        }
    }
}

void Server::dcopInfo(const QString& string)
{
    appendMessageToFrontmost(i18n("DCOP"),string);
}

void Server::ctcpReply(const QString &receiver,const QString &text)
{
    queue("NOTICE "+receiver+" :"+'\x01'+text+'\x01');
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
    ChannelMembershipMap::ConstIterator channel;
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
    ChannelMembershipMap::ConstIterator channel;
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
        int methodId = Preferences::dccMethodToGetOwnIp();

        if(methodId == 1)                         // Reply from IRC server
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
                                                  // user specifies
        else if(methodId == 2 && !Preferences::dccSpecificOwnIp().isEmpty())
        {
            KNetwork::KResolverResults res = KNetwork::KResolver::resolve(Preferences::dccSpecificOwnIp(), "");
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

class Query *Server::addQuery(const NickInfoPtr & nickInfo, bool weinitiated)
{
    QString nickname = nickInfo->getNickname();
    // Only create new query object if there isn't already one with the same name
    class Query* query=getQueryByName(nickname);
    if(!query)
    {
        QString lcNickname = nickname.lower();
        query=getViewContainer()->addQuery(this,nickInfo, weinitiated);
        query->setIdentity(getIdentity());

        connect(query,SIGNAL (sendFile(const QString&)),this,SLOT (requestDccSend(const QString &)) );
        connect(this,SIGNAL (serverOnline(bool)),query,SLOT (serverOnline(bool)) );

        // Append query to internal list
        queryList.append(query);

        m_queryNicks.insert(lcNickname, nickInfo);

        if(!weinitiated)
        {
            static_cast<KonversationApplication*>(kapp)->notificationHandler()->query(query, nickname);
        }
    }

    // try to get hostmask if there's none yet
    if(query->getNickInfo()->getHostmask().isEmpty()) requestUserhost(nickname);
    Q_ASSERT(query);
    return query;
}

void Server::closeQuery(const QString &name)
{
    class Query* query=getQueryByName(name);
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
    Channel* channelToClose = getChannelByName(name);

    if(channelToClose)
    {
        Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),
            Preferences::commandChar() + "PART", name);
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
    for(QStringList::ConstIterator it=nicksList.begin() ; it!=nicksList.end() ; ++it)
        inputFilter.setAutomaticRequest("USERHOST", *it, true);
    queue("USERHOST "+nicks);
}

void Server::requestTopic(const QString& channel)
{
    inputFilter.setAutomaticRequest("TOPIC", channel, true);
    queue("TOPIC "+channel);
}

void Server::resolveUserhost(const QString& nickname)
{
    inputFilter.setAutomaticRequest("WHOIS", nickname, true);
    inputFilter.setAutomaticRequest("DNS", nickname, true);
    queue("WHOIS "+nickname);
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
                    mask=targetNick->getNickname()+'!'+hostmask;

                    // adapt ban mask to the option given
                    if(option=="host")
                        mask="*!*@*."+hostmask.section('.',1);
                    else if(option=="domain")
                        mask="*!*@"+hostmask.section('@',1);
                    else if(option=="userhost")
                        mask="*!"+hostmask.section('@',0,0)+"@*."+hostmask.section('.',1);
                    else if(option=="userdomain")
                        mask="*!"+hostmask.section('@',0,0)+'@'+hostmask.section('@',1);
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

void Server::sendURIs(const QStrList& uris, const QString& nick)
{
    for (QStrListIterator it(uris) ; *it; ++it)
        addDccSend(nick,KURL(*it));
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
        class Query* lookQuery=queryList.first();
        while(lookQuery)
        {
            if(!nickList.contains(lookQuery->getName())) nickList.append(lookQuery->getName());
            lookQuery=queryList.next();
        }

        recipient=DccRecipientDialog::getNickname(getViewContainer()->getWindow(),nickList);
    }
    // do we have a recipient *now*?
    if(!recipient.isEmpty())
    {
        KURL::List fileURLs=KFileDialog::getOpenURLs(
            lastDccDir,
            QString::null,
            getViewContainer()->getWindow(),
            i18n("Select File(s) to Send to %1").arg(recipient)
        );
        KURL::List::iterator it;
        for ( it = fileURLs.begin() ; it != fileURLs.end() ; ++it )
        {
            lastDccDir = (*it).directory();
            addDccSend( recipient, *it );
        }
    }
}

void Server::addDccSend(const QString &recipient,KURL fileURL, const QString &altFileName, uint fileSize)
{
    emit addDccPanel();

    QString ownIp = getIp(true);

    // We already checked that the file exists in output filter / requestDccSend() resp.
    DccTransferSend* newDcc = new DccTransferSend( getViewContainer()->getDccPanel(),
                                                   recipient,
                                                   fileURL,  // url of the sending file
                                                   ownIp,
                                                   altFileName,
                                                   fileSize );

    connect(newDcc,SIGNAL (sendReady(const QString&,const QString&,const QString&,const QString&,unsigned long)),
        this,SLOT (dccSendRequest(const QString&,const QString&,const QString&,const QString&,unsigned long)) );
    connect(newDcc,SIGNAL (done(const DccTransfer*)),this,SLOT (dccSendDone(const DccTransfer*)) );
    connect(newDcc,SIGNAL (statusChanged(const DccTransfer*,int,int)), this,
        SLOT(dccStatusChanged(const DccTransfer*,int,int)) );

    newDcc->start();

    appendMessageToFrontmost( i18n( "DCC" ),
                              i18n( "Asking %1 to accept upload of \"%2\" (%3)..." )
                              .arg( newDcc->getPartnerNick(),
                                    newDcc->getFileName(),
                                    ( newDcc->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( newDcc->getFileSize() ) ) );
}

void Server::addDccGet(const QString &sourceNick, const QStringList &dccArguments)
{
    emit addDccPanel();

    QHostAddress ip;

    ip.setAddress(dccArguments[1].toULong());

    DccTransferRecv* newDcc=new DccTransferRecv(getViewContainer()->getDccPanel(),
        sourceNick,
        KURL(Preferences::dccPath()),
        dccArguments[0],                          // name
                                                  // size
        dccArguments[3].isEmpty() ? 0 : dccArguments[3].toULong(),
        ip.toString(),                            // ip
        dccArguments[2] );                        // port

    connect(newDcc,SIGNAL (resumeRequest(const QString&,const QString&,const QString&,KIO::filesize_t)),this,
        SLOT (dccResumeGetRequest(const QString&,const QString&,const QString&,KIO::filesize_t)) );
    connect(newDcc,SIGNAL (done(const DccTransfer*)),
        this,SLOT (dccGetDone(const DccTransfer*)) );
    connect(newDcc,SIGNAL (statusChanged(const DccTransfer*,int,int)), this,
        SLOT(dccStatusChanged(const DccTransfer*,int,int)) );

    appendMessageToFrontmost( i18n( "DCC" ),
                              i18n( "%1 offers to send you \"%2\" (%3)..." )
                              .arg( newDcc->getPartnerNick(),
                                    newDcc->getFileName(),
                                    ( newDcc->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( newDcc->getFileSize() ) ) );

    if(Preferences::dccAutoGet()) newDcc->start();
}

void Server::requestDccChat(const QString& nickname)
{
    emit addDccChat(getNickname(),nickname,getNumericalIp(true),QStringList(),true);
}

void Server::dccSendRequest(const QString &partner, const QString &fileName, const QString &address, const QString &port, unsigned long size)
{
    kdDebug() << "dccSendRequest sent" << endl;
    Konversation::OutputFilterResult result = outputFilter->sendRequest(partner,fileName,address,port,size);
    queue(result.toServer);
}

void Server::dccResumeGetRequest(const QString &sender, const QString &fileName, const QString &port, KIO::filesize_t startAt)
{
    Konversation::OutputFilterResult result;

    if (fileName.contains(" ") > 0)
        result = outputFilter->resumeRequest(sender,"\""+fileName+"\"",port,startAt);
    else
        result = outputFilter->resumeRequest(sender,fileName,port,startAt);

    queue(result.toServer);
}

void Server::resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
    // Check if there actually is a transfer going on on that port
    DccTransferRecv* dccTransfer=static_cast<DccTransferRecv*>(getViewContainer()->getDccPanel()->getTransferByPort(dccArguments[1],DccTransfer::Receive,true));
    if(!dccTransfer)
        // Check if there actually is a transfer going on with that name, could be behind a NAT
        // so the port number may get changed
        // mIRC substitutes this with "file.ext", so we have a problem here with mIRCs behind a NAT
        dccTransfer=static_cast<DccTransferRecv*>(getViewContainer()->getDccPanel()->getTransferByName(dccArguments[0],DccTransfer::Receive,true));

    if(dccTransfer)
    {
        // overcome mIRCs brain-dead "file.ext" substitution
        appendMessageToFrontmost( i18n( "DCC" ),
                                  i18n( "%1 = file name, %2 = nickname of sender, %3 = percentage of file size, %4 = file size",
                                        "Resuming download of \"%1\" from %2 starting at %3% of %4..." )
                                  .arg( dccTransfer->getFileName(),
                                        sourceNick,
                                        QString::number( dccTransfer->getProgress() ),
                                        ( dccTransfer->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( dccTransfer->getFileSize() ) ) );
        dccTransfer->startResume(dccArguments[2].toULong());
    }
    else
    {
        appendMessageToFrontmost(i18n("Error"),i18n("No DCC download running on port %1.").arg(dccArguments[1]));
    }
}

void Server::resumeDccSendTransfer(const QString &recipient, const QStringList &dccArguments)
{
    // Check if there actually is a transfer going on on that port
    DccTransferSend* dccTransfer=static_cast<DccTransferSend*>(getViewContainer()->getDccPanel()->getTransferByPort(dccArguments[1],DccTransfer::Send));
    if(!dccTransfer)
        // Check if there actually is a transfer going on with that name, could be behind a NAT
        // so the port number may get changed
        // mIRC substitutes this with "file.ext", so we have a problem here with mIRCs behind a NAT
        dccTransfer=static_cast<DccTransferSend*>(getViewContainer()->getDccPanel()->getTransferByName(dccArguments[0],DccTransfer::Send));

    if(dccTransfer && dccTransfer->getStatus() == DccTransfer::WaitingRemote)
    {
        QString fileName=dccTransfer->getFileName();
        if(dccTransfer->setResume(dccArguments[2].toULong()))
        {
            appendMessageToFrontmost( i18n( "DCC" ),
                                      i18n( "%1 = file name, %2 = nickname of recipient, %3 = percentage of file size, %4 = file size",
                                            "Resuming upload of \"%1\" to %2 starting at %3% of %4...")
                                      .arg( fileName,
                                            recipient,
                                            QString::number(dccTransfer->getProgress()),
                                            ( dccTransfer->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( dccTransfer->getFileSize() ) ) );
            Konversation::OutputFilterResult result = outputFilter->acceptRequest(recipient,
                fileName, dccArguments[1], dccArguments[2].toUInt());
            queue(result.toServer);
            //appendMessageToFrontmost(result.typeString, result.output);
        }
        else
        {
            appendMessageToFrontmost(i18n("Error"),i18n("%1 = file name, %2 = nickname",
                "Received invalid resume request for \"%1\" from %2.").arg(fileName, recipient));
        }
    }
    else
    {
        appendMessageToFrontmost(i18n("Error"),i18n("No DCC upload running on port %1.").arg(dccArguments[1]));
    }
}

void Server::dccGetDone(const DccTransfer* item)
{
    if(item->getStatus()==DccTransfer::Done)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of sender",
            "Download of \"%1\" from %2 finished.").arg(item->getFileName(), item->getPartnerNick()));
    else if(item->getStatus()==DccTransfer::Failed)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of sender",
            "Download of \"%1\" from %2 failed. Reason: %3.").arg(item->getFileName(), item->getPartnerNick(), item->getStatusDetail()));
}

void Server::dccSendDone(const DccTransfer* item)
{
    if(item->getStatus()==DccTransfer::Done)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of recipient",
            "Upload of \"%1\" to %2 finished.").arg(item->getFileName(), item->getPartnerNick()));
    else if(item->getStatus()==DccTransfer::Failed)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of recipient",
            "Upload of \"%1\" to %2 failed. Reason: %3.").arg(item->getFileName(), item->getPartnerNick(), item->getStatusDetail()));
}

void Server::dccStatusChanged(const DccTransfer *item, int newStatus, int oldStatus)
{
    getViewContainer()->getDccPanel()->dccStatusChanged(item);

    if ( item->getType() == DccTransfer::Send )
    {
        // when resuming, a message about the receiver's acceptance has been shown already, so suppress this message
        if ( newStatus == DccTransfer::Sending && oldStatus == DccTransfer::WaitingRemote && !item->isResumed() )
            appendMessageToFrontmost( i18n( "DCC" ), i18n( "%1 = file name, %2 nickname of recipient",
                "Sending \"%1\" to %2...").arg( item->getFileName(), item->getPartnerNick() ) );
    }
    else
    {
        if ( newStatus == DccTransfer::Receiving && !item->isResumed() )
        {
            appendMessageToFrontmost( i18n( "DCC" ),
                                        i18n( "%1 = file name, %2 = file size, %3 = nickname of sender", "Downloading \"%1\" (%2) from %3...")
                                        .arg( item->getFileName(),
                                            ( item->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( item->getFileSize() ),
                                            item->getPartnerNick() ) );
        }
    }
}

void Server::removeQuery(class Query* query)
{
    // Traverse through list to find the query
    class Query* lookQuery=queryList.first();
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
    query->deleteLater();
}

void Server::sendJoinCommand(const QString& name, const QString& password)
{
    Konversation::OutputFilterResult result = outputFilter->parse(getNickname(),
        Preferences::commandChar() + "JOIN " + name + ' ' + password, QString::null);
    queue(result.toServer);
}

void Server::joinChannel(const QString& name, const QString& hostmask)
{
    // (re-)join channel, open a new panel if needed
    Channel* channel = getChannelByName(name);

    if(!channel)
    {
        channel=getViewContainer()->addChannel(this,name);
        Q_ASSERT(channel);
        channel->setIdentity(getIdentity());
        channel->setNickname(getNickname());
        channel->indicateAway(isAway());
        Konversation::ChannelSettings channelSettings = m_serverGroup->channelByNameFromHistory(name);
        channel->setNotificationsEnabled(channelSettings.enableNotifications());

        channelList.append(channel);
        m_serverGroup->appendChannelHistory(channelSettings);

        connect(channel,SIGNAL (sendFile()),this,SLOT (requestDccSend()) );
        connect(this,SIGNAL (serverOnline(bool)),channel,SLOT (serverOnline(bool)) );
        connect(this, SIGNAL(nicknameChanged(const QString&)), channel, SLOT(setNickname(const QString&)));
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

    Konversation::ChannelSettings channelSettings = m_serverGroup->channelByNameFromHistory(channel->getName());
    channelSettings.setNotificationsEnabled(channel->notificationsEnabled());
    m_serverGroup->appendChannelHistory(channelSettings);

    channelList.removeRef(channel);
}

void Server::updateChannelMode(const QString &updater, const QString &channelName, char mode, bool plus, const QString &parameter)
{

    Channel* channel=getChannelByName(channelName);

    if(channel)                                   //Let the channel be verbose to the screen about the change, and update channelNick
        channel->updateMode(updater, mode, plus, parameter);
    // TODO: What is mode character for owner?
    // Answer from JOHNFLUX - I think that admin is the same as owner.  Channel.h has owner as "a"
    // "q" is the likely answer.. UnrealIRCd and euIRCd use it.
    // TODO these need to become dynamic
    QString userModes="vhoqa";                    // voice halfop op owner admin
    int modePos = userModes.find(mode);
    if (modePos > 0)
    {
        ChannelNickPtr updateeNick = getChannelNick(channelName, parameter);
        if(!updateeNick)
        {
/*
            if(parameter.isEmpty())
            {
                kdDebug() << "in updateChannelMode, a nick with no-name has had their mode '" << mode << "' changed to (" <<plus << ") in channel '" << channelName << "' by " << updater << ".  How this happened, I have no idea.  Please report this message to irc #konversation if you want to be helpful." << endl << "Ignoring the error and continuing." << endl;
                                                  //this will get their attention.
                kdDebug() << kdBacktrace() << endl;
            }
            else
            {
                kdDebug() << "in updateChannelMode, could not find updatee nick " << parameter << " for channel " << channelName << endl;
                kdDebug() << "This could indicate an obscure race condition that is safely being handled (like the mode of someone changed and they quit almost simulatanously, or it could indicate an internal error." << endl;
            }
*/
            //TODO Do we need to add this nick?
            return;
        }

        updateeNick->setMode(mode, plus);

        // Note that channel will be moved to joined list if necessary.
        addNickToJoinedChannelsList(channelName, parameter);
    }

    // Update channel ban list.
    if (mode == 'b')
    {
        if (plus)
        {
            QDateTime when;
            addBan(channelName, QString("%1 %2 %3").arg(parameter).arg(updater).arg(QDateTime::currentDateTime().toTime_t()));
        } else {
            removeBan(channelName, parameter);
        }
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
        channel->updateQuickButtons(Preferences::quickButtonList());
        channel=channelList.next();
    }
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

class Query* Server::getQueryByName(const QString& name)
{
    // Convert wanted query name to lowercase
    QString wanted=name;
    wanted=wanted.lower();

    // Traverse through list to find the query with "name"
    class Query* lookQuery=queryList.first();
    while(lookQuery)
    {
        if(lookQuery->getName().lower()==wanted) return lookQuery;
        lookQuery=queryList.next();
    }
    // No query by that name found? Must be a new query request. Return 0
    return 0;
}

void Server::resetNickList(const QString& channelName)
{
    Channel* outChannel=getChannelByName(channelName);
    if(outChannel) outChannel->resetNickList();
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
    // if nickinfo already exists update nickname, in case we created the nickinfo based
    // on e.g. an incorrectly capitalized ISON request
    else
        nickInfo->setNickname(nickname);

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
    Q_ASSERT(channelNick);                        //Since we just added it if it didn't exist, it should be guaranteed to exist now
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
void Server::emitChannelNickChanged(const ChannelNickPtr channelNick)
{
    emit channelNickChanged(this, channelNick);
}

/** This function should _only_ be called from the NickInfo class.
 *  This function should also be the only one to emit this signal.
 *  In this class, when nickInfo is changed, it emits its own signal, and
 *  calls this function itself.
 */
void Server::emitNickInfoChanged(const NickInfoPtr nickInfo)
{
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
        appendMessageToFrontmost(i18n("Notify"),"<a href=\"#"+nickname+"\">"+
            i18n("%1 is online (%2).").arg(nickname).arg(getServerName())+"</a>",statusView);

        static_cast<KonversationApplication*>(kapp)->notificationHandler()->nickOnline(getStatusView(), nickname);
    }
    nickInfo->setPrintedOnline(true);
    return nickInfo;
}

bool Server::setNickOffline(const QString& nickname)
{
    QString lcNickname = nickname.lower();
    NickInfoPtr nickInfo = getNickInfo(lcNickname);
    bool wasOnline = nickInfo->getPrintedOnline();

    if (nickInfo && wasOnline)
    {
        KABC::Addressee addressee = nickInfo->getAddressee();
        // Delete from query list, if present.
        if (m_queryNicks.contains(lcNickname)) m_queryNicks.remove(lcNickname);
        // Delete the nickname from all channels (joined or unjoined).
        QStringList nickChannels = getNickChannels(lcNickname);
        QStringList::iterator itEnd = nickChannels.end();

        for(QStringList::iterator it = nickChannels.begin(); it != itEnd; ++it)
        {
            QString channel = (*it);
            removeChannelNick(channel, lcNickname);
        }

        // Delete NickInfo.
        if (m_allNicks.contains(lcNickname)) m_allNicks.remove(lcNickname);
        // If the nick was in the watch list, emit various signals and messages.
        if (isWatchedNick(nickname))
        {
            emit watchedNickChanged(this, nickname, false);

            if (!addressee.isEmpty())
            {
                Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid(), 1);
            }

            appendMessageToFrontmost(i18n("Notify"), i18n("%1 went offline (%2).").arg(nickname).arg(getServerName()),statusView);

            static_cast<KonversationApplication*>(kapp)->notificationHandler()->nickOffline(getStatusView(), nickname);
        }

        nickInfo->setPrintedOnline(false);
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
    // no nickinfo ISON for the time being
    return Preferences::notifyListByGroupName(getServerGroup());
    if (m_serverISON)
        return m_serverISON->getWatchList();
    else
        return QStringList();
}

QString Server::getWatchListString() { return getWatchList().join(" "); }

QStringList Server::getISONList()
{
    // no nickinfo ISON for the time being
    return Preferences::notifyListByGroupName(getServerGroup());
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
    QStringList watchListLower = getWatchList();
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
        if(!channel) return;                      //already removed.. hmm
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
            delete channel;                       // recover memory!
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
        nickInfo->setIdentified(false);
        QString lcNewname = newname.lower();
        // Rename the key in m_allNicks list.
        m_allNicks.remove(lcNickname);
        m_allNicks.insert(lcNewname, nickInfo);
        // Rename key in the joined and unjoined lists.
        QStringList nickChannels = getNickChannels(lcNickname);
        QStringList::iterator itEnd = nickChannels.end();

        for(QStringList::iterator it = nickChannels.begin(); it != itEnd; ++it)
        {
            const ChannelNickMap *channel = getChannelMembers(*it);
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
    }
    else
    {
        kdDebug() << "server::renameNickInfo() was called for newname='" << newname << "' but nickInfo is null" << endl;
    }
}

Channel* Server::nickJoinsChannel(const QString &channelName, const QString &nickname, const QString &hostmask)
{
    Channel* outChannel=getChannelByName(channelName);
    if(outChannel)
    {
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
        if(channelNick)
        {
          outChannel->kickNick(channelNick, *kickerNick, reason);
          // Tell Nickinfo
          removeChannelNick(channelName,nickname);
        }
    }
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

    Query* query=getQueryByName(nickname);
    if(query)
    {
      query->quitNick(reason);
    }

    // Delete the nick from all channels and then delete the nickinfo,
    // emitting signal if on the watch list.
    setNickOffline(nickname);
}

void Server::renameNick(const QString &nickname, const QString &newNick)
{
    if(nickname.isEmpty() || newNick.isEmpty())
    {
        kdDebug() << "server::renameNick called with empty strings!  Trying to rename '" << nickname << "' to '" << newNick << "'" << endl;
        return;
    }

    // If this was our own nickchange, tell our server object about it
    if(nickname == getNickname())
    {
        setNickname(newNick);

        // We may get a request from nickserv, so remove the auto-identify lock
        m_autoIdentifyLock = false;
    }

    //Actually do the rename.
    NickInfoPtr nickInfo = getNickInfo(nickname);

    if(!nickInfo)
    {
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
    // If we had a query with this nick, change that name, too

}

void Server::userhost(const QString& nick,const QString& hostmask,bool away,bool /* ircOp */)
{
    addHostmaskToNick(nick,hostmask);
    // remember my IP for DCC things
                                                  // myself
    if( ownIpByUserhost.isEmpty() && nick==nickname )
    {
        QString myhost = hostmask.section('@', 1);
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
    if ( res.error() == KResolver::NoError && !res.isEmpty() )
    {
        ownIpByUserhost = res.first().address().nodeName();
    }
    else
        kdDebug() << "Server::gotOwnResolvedHostByUserhost(): Got error: " << ( int )res.error() << endl;
}

void Server::appendServerMessageToChannel(const QString& channel,const QString& type,const QString& message)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
        outChannel->appendServerMessage(type,message);
}

void Server::appendCommandMessageToChannel(const QString& channel,const QString& command,const QString& message)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
        outChannel->appendCommandMessage(command,message);
}

void Server::appendStatusMessage(const QString& type,const QString& message)
{
    statusView->appendServerMessage(type,message);
}

void Server::appendMessageToFrontmost(const QString& type,const QString& message, bool parseURL)
{
    getViewContainer()->appendToFrontmost(type,message, statusView, parseURL);
}

void Server::setNickname(const QString &newNickname)
{
    nickname = newNickname;
    m_loweredNickname = newNickname.lower();
    emit nicknameChanged(newNickname);
}

void Server::setChannelTopic(const QString &channel, const QString &newTopic)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
    {
        // encoding stuff is done in send()
        outChannel->setTopic(newTopic);
    }
}

                                                  // Overloaded
void Server::setChannelTopic(const QString& nickname, const QString &channel, const QString &newTopic)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
    {
        // encoding stuff is done in send()
        outChannel->setTopic(nickname,newTopic);
    }
}

void Server::setTopicAuthor(const QString& channel,const QString& author)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
        outChannel->setTopicAuthor(author);
}

void Server::endOfWho(const QString& target)
{
    Channel* channel = getChannelByName(target);
    if(channel)
        channel->scheduleAutoWho();
}

bool Server::isNickname(const QString &compare) const
{
    return (nickname == compare);
}

QString Server::getNickname() const
{
    return nickname;
}

QString Server::loweredNickname() const
{
    return m_loweredNickname;
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
    // TODO: parameter handling, since parameters are not functional yet

    // store the parsed version
    QString out;

    // default separator
    QString separator(" ");

    int index = 0, found = 0;
    QChar toExpand;

    while ((found = toParse.find('%',index)) != -1)
    {
                                                  // append part before the %
        out.append(toParse.mid(index,found-index));
        index = found + 1;                        // skip the part before, including %
        if (index >= (int)toParse.length())
            break;                                // % was the last char (not valid)
        toExpand = toParse.at(index++);
        if (toExpand == 's')
        {
            found = toParse.find('%',index);
            if (found == -1)                      // no other % (not valid)
                break;
            separator = toParse.mid(index,found-index);
            index = found + 1;                    // skip separator, including %
        }
        else if (toExpand == 'u')
        {
            out.append(nickList.join(separator));
        }
        else if (toExpand == 'c')
        {
            if(!channelName.isEmpty())
                out.append(channelName);
        }
        else if (toExpand == 'o')
        {
            out.append(sender);
        }
        else if (toExpand == 'k')
        {
            if(!channelKey.isEmpty())
                out.append(channelKey);
        }
        else if (toExpand == 'K')
        {
            if(!m_serverGroup->serverByIndex(m_currentServerIndex).password().isEmpty())
                out.append(m_serverGroup->serverByIndex(m_currentServerIndex).password());
        }
        else if (toExpand == 'n')
        {
            out.append("\n");
        }
        else if (toExpand == 'p')
        {
            out.append("%");
        }
    }

                                                  // append last part
    out.append(toParse.mid(index,toParse.length()-index));
    return out;
}

void Server::setIrcName(const QString &newIrcName)
{
    ircName = newIrcName;
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
    Channel* channel = channelList.first();
    while(channel)
    {
        channel->sendChannelText(text);
        channel = channelList.next();
    }
}

void Server::invitation(const QString& nick,const QString& channel)
{
    if(Preferences::autojoinOnInvite() &&
        KMessageBox::questionYesNo(getViewContainer()->getWindow(),
        i18n("You were invited by %1 to join channel %2. "
        "Do you accept the invitation?").arg(nick).arg(channel),
        i18n("Invitation"),
        i18n("Join"),
        i18n("Ignore"),
        "Invitation")==KMessageBox::Yes)
    {
        sendJoinCommand(channel);
    }
}

void Server::scriptNotFound(const QString& name)
{
    appendMessageToFrontmost(i18n("DCOP"),i18n("Error: Could not find script \"%1\".").arg(name));
}

void Server::scriptExecutionError(const QString& name)
{
    appendMessageToFrontmost(i18n("DCOP"),i18n("Error: Could not execute script \"%1\". Check file permissions.").arg(name));
}

void Server::away()
{
    if(!m_isAway)
        startAwayTimer();                         //Don't start timer if we have already started it

    m_isAway=true;
    emit awayState(true);

    if(!getIdentity()->getAwayNick().isEmpty() && getIdentity()->getAwayNick() != getNickname())
    {
        nonAwayNick = getNickname();
        queue("NICK " + getIdentity()->getAwayNick());
    }

    if(getIdentity()->getInsertRememberLineOnAway())
        emit awayInsertRememberLine(this);

    // TODO: call renameNickInfo ?

    if(getViewContainer()->getWindow())
    {
        KAction *action = getViewContainer()->getWindow()->actionCollection()->action("toggle_away");
        if(action)
        {
            action->setText(i18n("Set &Available Globally"));
            action->setIcon("konversationavailable");
        }
    }

}

void Server::unAway()
{
    m_isAway = false;
    m_awayReason = QString::null;
    emit awayState(false);

    if(!getIdentity()->getAwayNick().isEmpty() && !nonAwayNick.isEmpty())
        queue("NICK " + nonAwayNick);

    // TODO: call renameNickInfo ?

    if(getViewContainer()->getWindow())
    {
        KAction *action = getViewContainer()->getWindow()->actionCollection()->action("toggle_away");
        if(action)
        {
                                                  //this may be wrong if other servers are still away
            action->setText(i18n("Set &Away Globally"));
            action->setIcon("konversationaway");
        }
    }

}

void Server::setAwayReason(const QString& reason)
{
    m_awayReason = reason;
}

bool Server::isAChannel(const QString &channel) const
{
    return (getChannelTypes().contains(channel.at(0)) > 0);
}

void Server::addRawLog(bool show)
{
    if (!rawLog)
        rawLog=getViewContainer()->addRawLog(this);

    connect(this,SIGNAL (serverOnline(bool)),rawLog,SLOT (serverOnline(bool)) );

    // bring raw log to front since the main window does not do this for us
    if (show) emit showView(rawLog);
}

void Server::closeRawLog()
{
    if(rawLog)
    {
        delete rawLog;
        rawLog = 0;
    }
}

ChannelListPanel* Server::addChannelListPanel()
{
    if(!channelListPanel)
    {
        channelListPanel = getViewContainer()->addChannelListPanel(this);

        connect(channelListPanel, SIGNAL(refreshChannelList()), this, SLOT(requestChannelList()));
        connect(channelListPanel, SIGNAL(joinChannel(const QString&)), this, SLOT(sendJoinCommand(const QString&)));
        connect(this, SIGNAL(serverOnline(bool)), channelListPanel, SLOT(serverOnline(bool)));
    }

    return channelListPanel;
}

void Server::addToChannelList(const QString& channel, int users, const QString& topic)
{
    addChannelListPanel();
    channelListPanel->addToChannelList(channel, users, topic);
}

ChannelListPanel* Server::getChannelListPanel() const
{
    return channelListPanel;
}

void Server::closeChannelListPanel()
{
    if(channelListPanel)
    {
        delete channelListPanel;
        channelListPanel = 0;
    }
}

void Server::updateAutoJoin(const QString& channel)
{
    Konversation::ChannelList tmpList = m_serverGroup->channelList();

    if (quickConnect && channel.isEmpty())
    {
        setAutoJoin(false);
        return;
    }

    if(!channel.isEmpty())
        tmpList.push_front(Konversation::ChannelSettings(channel));

    if(!tmpList.isEmpty())
    {
        setAutoJoin(true);

        QString channels;
        QString keys;

        if (tmpList.count()>1)
        {
            Konversation::ChannelList::iterator it;
            for(it = tmpList.begin(); it != tmpList.end(); ++it)
            {
                if(it != tmpList.begin())
                {
                    channels += ',';
                    keys += ',';
                }

                channels += (*it).name();
                keys += ((*it).password().isEmpty() ? QString(".") : (*it).password());
            }
        }
        else
        {
            channels = tmpList.first().name();
            keys = (tmpList.first().password().isEmpty() ? QString("") : tmpList.first().password());
        }

        setAutoJoinChannel(channels);
        setAutoJoinChannelKey(keys);
    }
    else
    {
        setAutoJoin(false);
    }
}

void Server::autoRejoinChannels()
{
    if (channelList.isEmpty())
        return;

    QStringList channels;
    QStringList keys;

    for (Channel* ch = channelList.first(); ch; ch = channelList.next())
    {
        channels.append(ch->getName());
        keys.append(ch->getKey());
    }

    QString joinString("JOIN "+channels.join(",")+' '+keys.join(","));
    queue(joinString);
}

IdentityPtr Server::getIdentity() const
{
    return m_serverGroup->identity();
}

void Server::setViewContainer(ViewContainer* newViewContainer)
{
    m_viewContainerPtr = newViewContainer;
}

ViewContainer* Server::getViewContainer() const
{
    return m_viewContainerPtr;
}

bool Server::getUseSSL() const
{
    return m_serverGroup->serverByIndex(m_currentServerIndex).SSLEnabled();
}

QString Server::getSSLInfo() const
{
    return static_cast<SSLSocket*>(m_socket)->details();
}

bool Server::connected() const
{
    return alreadyConnected;
}

void Server::sendMultiServerCommand(const QString& command, const QString& parameter)
{
    emit multiServerCommand(command, parameter);
}

void Server::executeMultiServerCommand(const QString& command, const QString& parameter)
{
    if(command == "away" || command == "back")    //back is the same as away, since paramater is ""
    {
        QString str = Preferences::commandChar() + command;

                                                  //you cant have a message with 'back'
        if(!parameter.isEmpty() && command == "away")
            str += ' ' + parameter;

        Konversation::OutputFilterResult result = outputFilter->parse(getNickname(), str, QString::null);
        queue(result.toServer);
    }
    else if(command == "msg")
    {
        sendToAllChannelsAndQueries(parameter);
    }
    else
    {
        sendToAllChannelsAndQueries(Preferences::commandChar() + command + ' ' + parameter);
    }
}

void Server::sendToAllChannelsAndQueries(const QString& text)
{
    // Send a message to all channels we are in
    Channel* channel = channelList.first();

    while(channel)
    {
        channel->sendChannelText(text);
        channel=channelList.next();
    }

    // Send a message to all queries we are in
    class Query* query = queryList.first();

    while(query)
    {
        query->sendQueryText(text);
        query=queryList.next();
    }
}

void Server::reconnect()
{
    if (isConnected() && !connecting)
    {
        keepViewsOpenAfterQuit = true;
        reconnectAfterQuit = true;
        quitServer();
        send();
    }
    else if (!isConnected())
    {
        reconnectCounter = 0;
        connectToIRCServer();
    }
}

void Server::disconnect()
{
    if (isConnected())
    {
        keepViewsOpenAfterQuit = true;
        quitServer();
        send();
    }
}

void Server::connectToServerGroup(const QString& serverGroup)
{
    KonversationApplication *konvApp = static_cast<KonversationApplication*>(KApplication::kApplication());
    konvApp->connectToServerGroup(serverGroup);
}

void Server::connectToNewServer(const QString& server, const QString& port, const QString& password)
{
    KonversationApplication *konvApp = static_cast<KonversationApplication*>(KApplication::kApplication());
    konvApp->quickConnectToServer(server, port,"", "", password);
}

bool Server::isAway() const
{
    return m_isAway;
}

QString Server::awayTime() const
{
    QString retVal;

    if(m_isAway)
    {
        int diff = QDateTime::currentDateTime().toTime_t() - m_awayTime;
        int num = diff / 3600;

        if(num < 10)
        {
            retVal = '0' + QString::number(num) + ':';
        }
        else
        {
            retVal = QString::number(num) + ':';
        }

        num = (diff % 3600) / 60;

        if(num < 10)
        {
            retVal += '0';
        }

        retVal += QString::number(num) + ':';

        num = (diff % 3600) % 60;

        if(num < 10)
        {
            retVal += '0';
        }

        retVal += QString::number(num);
    }
    else
    {
        retVal = "00:00:00";
    }

    return retVal;
}

void Server::startAwayTimer()
{
    m_awayTime = QDateTime::currentDateTime().toTime_t();
}

KABC::Addressee Server::getOfflineNickAddressee(QString& nickname)
{
    if (m_serverISON)
        return m_serverISON->getOfflineNickAddressee(nickname);
    else
        return KABC::Addressee();
}

void Server::enableIdentifyMsg(bool enabled)
{
    m_identifyMsg = enabled;
}

bool Server::identifyMsgEnabled()
{
    return m_identifyMsg;
}

void Server::addBan(const QString &channel, const QString &ban)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
    {
        outChannel->addBan(ban);
    }
}

void Server::removeBan(const QString &channel, const QString &ban)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
    {
        outChannel->removeBan(ban);
    }
}

void Server::sendPing()
{
    //WHO ourselves once a minute in case the irc server has changed our
    //hostmask, such as what happens when a Freenode cloak is activated.
    //It might be more intelligent to only do this when there is text
    //in the inputbox. Kinda changes this into a "do minutely"
    //queue :-) 
    getInputFilter()->setAutomaticRequest("WHO", nickname, true);
    queueAt(0, "WHO " + nickname);

    queueAt(0, "PING LAG" + QTime::currentTime().toString("hhmmss"));
    m_lagTime.start();
    inputFilter.setLagMeasuring(true);
    m_pingResponseTimer.start(1000 /*1 sec*/);
}

void Server::pongReceived()
{
    currentLag = m_lagTime.elapsed();
    inputFilter.setLagMeasuring(false);
    m_pingResponseTimer.stop();

    emit serverLag(this, currentLag);

    // Send another PING in 60 seconds
    QTimer::singleShot(60000 /*60 sec*/, this, SLOT(sendPing()));
}

void Server::updateLongPongLag()
{
    if(isConnected())
    {
        currentLag = m_lagTime.elapsed();
        emit tooLongLag(this, currentLag);
        // kdDebug() << "Current lag: " << currentLag << endl;

        if (currentLag > (Preferences::maximumLagTime() * 1000))
            m_socket->close();
    }
}

#include "server.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
