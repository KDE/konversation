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
  Copyright (C) 2006-2008 Eli J. MacKenzie <argonel at gmail.com>
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#include "server.h"
#include "ircqueue.h"
#include "query.h"
#include "channel.h"
#include "konversationapplication.h"
#include "connectionmanager.h"
#include "dcccommon.h"
#include "dcctransferpanel.h"
#include "dcctransferpanelitem.h"
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
#include "dcctransfermanager.h"

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
#include <config.h>

int Server::m_availableConnectionId = 0;

Server::Server(QObject* parent, ConnectionSettings& settings) : QObject(parent)
{
    m_connectionId = m_availableConnectionId;
    m_availableConnectionId++;

    setConnectionSettings(settings);

    for (int i=0;i<=_max_queue();i++)
    {
        QValueList<int> r=Preferences::queueRate(i);
        IRCQueue *q=new IRCQueue(this, staticrates[i]); //FIXME these are supposed to be in the rc
        m_queues.append(q);
    }

    m_processingIncoming = false;
    m_identifyMsg = false;
    m_autoIdentifyLock = false;
    m_autoJoin = false;

    m_nickIndices.clear();
    m_nickIndices.append(0);

    m_currentLag = -1;
    m_rawLog = 0;
    m_channelListPanel = 0;
    m_serverISON = 0;
    m_isAway = false;
    m_socket = 0;
    m_prevISONList = QStringList();
    m_bytesReceived = 0;
    m_encodedBytesSent=0;
    m_bytesSent=0;
    m_linesSent=0;
    // TODO fold these into a QMAP, and these need to be reset to RFC values if this server object is reused.
    m_serverNickPrefixModes = "ovh";
    m_serverNickPrefixes = "@+%";
    m_channelPrefixes = "#&";

    setName(QString("server_" + settings.name()).ascii());

    setNickname(settings.initialNick());
    obtainNickInfo(getNickname());

    m_statusView = getViewContainer()->addStatusView(this);

    if (Preferences::rawLog())
        addRawLog(false);

    m_inputFilter.setServer(this);
    m_outputFilter = new Konversation::OutputFilter(this);
    m_scriptLauncher = new ScriptLauncher(this);

    // don't delete items when they are removed
    m_channelList.setAutoDelete(false);
    // For /msg query completion
    m_completeQueryPosition = 0;

    updateAutoJoin(settings.initialChannel());

    if (!getIdentity()->getShellCommand().isEmpty())
        QTimer::singleShot(0, this, SLOT(doPreShellCommand()));
    else
        QTimer::singleShot(0, this, SLOT(connectToIRCServer()));

    initTimers();

    if (getIdentity()->getShellCommand().isEmpty())
        connectSignals();

    updateConnectionState(Konversation::SSNeverConnected);
}

Server::~Server()
{
    //send queued messages
    kdDebug() << "Server::~Server(" << getServerName() << ")" << endl;

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

    if (m_statusView) delete m_statusView;

    closeRawLog();
    closeChannelListPanel();

    m_channelList.setAutoDelete(true);
    m_channelList.clear();

    m_queryList.setAutoDelete(true);
    m_queryList.clear();

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

    //Delete the queues
    for (QValueVector<IRCQueue *>::iterator it=m_queues.begin(); it != m_queues.end(); ++it)
        delete *it;

    emit destroyed(m_connectionId);

    kdDebug() << "~Server done" << endl;
}

void Server::doPreShellCommand()
{

    QString command = getIdentity()->getShellCommand();
    getStatusView()->appendServerMessage(i18n("Info"),"Running preconfigured command...");

    connect(&m_preShellCommand,SIGNAL(processExited(KProcess*)), this, SLOT(preShellCommandExited(KProcess*)));

    QStringList commandList = QStringList::split(" ",command);

    for (QStringList::ConstIterator it = commandList.begin(); it != commandList.end(); ++it)
        m_preShellCommand << *it;

    if (!m_preShellCommand.start()) preShellCommandExited(NULL);
}

void Server::_fetchRates()
{
    for (int i=0;i<=_max_queue();i++)
    {
        QValueList<int> r=Preferences::queueRate(i);
        staticrates[i]=IRCQueue::EmptyingRate(r[0], r[1]*1000,IRCQueue::EmptyingRate::RateType(r[2]));
    }
}

void Server::_stashRates()
{
    for (int i=0;i<=_max_queue();i++)
    {
        QValueList<int> r;
        r.append(staticrates[i].m_rate);
        r.append(staticrates[i].m_interval/1000);
        r.append(int(staticrates[i].m_type));
        Preferences::setQueueRate(i, r);
    }
}

void Server::initTimers()
{
    m_notifyTimer.setName("notify_timer");
    m_incomingTimer.setName("incoming_timer");
}

void Server::connectSignals()
{
    // Timers
    connect(&m_incomingTimer, SIGNAL(timeout()), this, SLOT(processIncomingData()));
    connect(&m_notifyTimer, SIGNAL(timeout()), this, SLOT(notifyTimeout()));
    connect(&m_pingResponseTimer, SIGNAL(timeout()), this, SLOT(updateLongPongLag()));

    // OutputFilter
    connect(getOutputFilter(), SIGNAL(requestDccSend()), this,SLOT(requestDccSend()));
    connect(getOutputFilter(), SIGNAL(requestDccSend(const QString&)), this, SLOT(requestDccSend(const QString&)));
    connect(getOutputFilter(), SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(getOutputFilter(), SIGNAL(reconnectServer()), this, SLOT(reconnect()));
    connect(getOutputFilter(), SIGNAL(disconnectServer()), this, SLOT(disconnect()));
    connect(getOutputFilter(), SIGNAL(openDccSend(const QString &, KURL)), this, SLOT(addDccSend(const QString &, KURL)));
    connect(getOutputFilter(), SIGNAL(openDccChat(const QString &)), this, SLOT(openDccChat(const QString &)));
    connect(getOutputFilter(), SIGNAL(sendToAllChannels(const QString&)), this, SLOT(sendToAllChannels(const QString&)));
    connect(getOutputFilter(), SIGNAL(banUsers(const QStringList&,const QString&,const QString&)),
        this, SLOT(requestBan(const QStringList&,const QString&,const QString&)));
    connect(getOutputFilter(), SIGNAL(unbanUsers(const QString&,const QString&)),
        this, SLOT(requestUnban(const QString&,const QString&)));
    connect(getOutputFilter(), SIGNAL(openRawLog(bool)), this, SLOT(addRawLog(bool)));
    connect(getOutputFilter(), SIGNAL(closeRawLog()), this, SLOT(closeRawLog()));
    connect(getOutputFilter(), SIGNAL(encodingChanged()), this, SLOT(updateEncoding()));

    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    connect(getOutputFilter(), SIGNAL(connectTo(Konversation::ConnectionFlag, const QString&,
                const QString&, const QString&, const QString&, const QString&, bool)),
            konvApp->getConnectionManager(), SLOT(connectTo(Konversation::ConnectionFlag,
                const QString&, const QString&, const QString&, const QString&, const QString&, bool)));

    connect(konvApp->dccTransferManager(), SIGNAL(newTransferQueued(DccTransfer*)),
            this, SLOT(slotNewDccTransferItemQueued(DccTransfer*)));

   // ViewContainer
    connect(this, SIGNAL(showView(ChatWindow*)), getViewContainer(), SLOT(showView(ChatWindow*)));
    connect(this, SIGNAL(addDccPanel()), getViewContainer(), SLOT(addDccPanel()));
    connect(this, SIGNAL(addDccChat(const QString&,const QString&,const QStringList&,bool)),
        getViewContainer(), SLOT(addDccChat(const QString&,const QString&,const QStringList&,bool)) );
    connect(this, SIGNAL(serverLag(Server*, int)), getViewContainer(), SIGNAL(updateStatusBarLagLabel(Server*, int)));
    connect(this, SIGNAL(tooLongLag(Server*, int)), getViewContainer(), SIGNAL(setStatusBarLagLabelTooLongLag(Server*, int)));
    connect(this, SIGNAL(resetLag()), getViewContainer(), SIGNAL(resetStatusBarLagLabel()));
    connect(getOutputFilter(), SIGNAL(showView(ChatWindow*)), getViewContainer(), SLOT(showView(ChatWindow*)));
    connect(getOutputFilter(), SIGNAL(openKonsolePanel()), getViewContainer(), SLOT(addKonsolePanel()));
    connect(getOutputFilter(), SIGNAL(openChannelList(const QString&, bool)), getViewContainer(), SLOT(openChannelList(const QString&, bool)));
    connect(getOutputFilter(), SIGNAL(closeDccPanel()), getViewContainer(), SLOT(closeDccPanel()));
    connect(getOutputFilter(), SIGNAL(addDccPanel()), getViewContainer(), SLOT(addDccPanel()));
    connect(&m_inputFilter, SIGNAL(addDccChat(const QString&,const QString&,const QStringList&,bool)),
        getViewContainer(), SLOT(addDccChat(const QString&,const QString&,const QStringList&,bool)) );

    // Inputfilter
    connect(&m_inputFilter, SIGNAL(welcome(const QString&)), this, SLOT(connectionEstablished(const QString&)));
    connect(&m_inputFilter, SIGNAL(notifyResponse(const QString&)), this, SLOT(notifyResponse(const QString&)));
    connect(&m_inputFilter, SIGNAL(startReverseDccSendTransfer(const QString&,const QStringList&)),
        this, SLOT(startReverseDccSendTransfer(const QString&,const QStringList&)));
    connect(&m_inputFilter, SIGNAL(addDccGet(const QString&, const QStringList&)),
        this, SLOT(addDccGet(const QString&, const QStringList&)));
    connect(&m_inputFilter, SIGNAL(resumeDccGetTransfer(const QString&, const QStringList&)),
        this, SLOT(resumeDccGetTransfer(const QString&, const QStringList&)));
    connect(&m_inputFilter, SIGNAL(resumeDccSendTransfer(const QString&, const QStringList&)),
        this, SLOT(resumeDccSendTransfer(const QString&, const QStringList&)));
    connect(&m_inputFilter, SIGNAL(userhost(const QString&,const QString&,bool,bool)),
        this, SLOT(userhost(const QString&,const QString&,bool,bool)) );
    connect(&m_inputFilter, SIGNAL(topicAuthor(const QString&,const QString&)),
        this, SLOT(setTopicAuthor(const QString&,const QString&)) );
    connect(&m_inputFilter, SIGNAL(endOfWho(const QString&)),
        this, SLOT(endOfWho(const QString&)) );
    connect(&m_inputFilter, SIGNAL(invitation(const QString&,const QString&)),
        this,SLOT(invitation(const QString&,const QString&)) );
    connect(&m_inputFilter, SIGNAL(addToChannelList(const QString&, int, const QString& )),
        this, SLOT(addToChannelList(const QString&, int, const QString& )));
    connect(&m_inputFilter, SIGNAL(away()), this, SLOT(away()));
    connect(&m_inputFilter, SIGNAL(unAway()), this, SLOT(unAway()));

    // Status View
    connect(this, SIGNAL(serverOnline(bool)), getStatusView(), SLOT(serverOnline(bool)));

    // Scripts
    connect(getOutputFilter(), SIGNAL(launchScript(const QString&, const QString&)),
        m_scriptLauncher, SLOT(launchScript(const QString&, const QString&)));
    connect(m_scriptLauncher, SIGNAL(scriptNotFound(const QString&)),
        this, SLOT(scriptNotFound(const QString&)));
    connect(m_scriptLauncher, SIGNAL(scriptExecutionError(const QString&)),
        this, SLOT(scriptExecutionError(const QString&)));

    // Stats
    connect(this, SIGNAL(sentStat(int, int)), SLOT(collectStats(int, int)));
}

int Server::getPort()
{
    return getConnectionSettings().server().port();
}

int Server::getLag()  const
{
    return m_currentLag;
}

bool Server::getAutoJoin()  const
{
    return m_autoJoin;
}

void Server::setAutoJoin(bool on)
{
    m_autoJoin = on;
}

QString Server::getAutoJoinChannels() const
{
    return m_autoJoinChannels;
}

void Server::setAutoJoinChannels(const QString& channels)
{
    m_autoJoinChannels = channels;
}

QString Server::getAutoJoinChannelPasswords() const
{
    return m_autoJoinChannelPasswords;
}

void Server::setAutoJoinChannelPasswords(const QString& passwords)
{
    m_autoJoinChannelPasswords = passwords;
}

void Server::preShellCommandExited(KProcess* proc)
{

    if (proc && proc->normalExit())
        getStatusView()->appendServerMessage(i18n("Info"),"Process executed successfully!");
    else
        getStatusView()->appendServerMessage(i18n("Warning"),"There was a problem while executing the command!");

    connectToIRCServer();
    connectSignals();
}

void Server::connectToIRCServer()
{
    if (!isConnected())
    {
        updateConnectionState(Konversation::SSConnecting);

        m_autoIdentifyLock = false;
        m_ownIpByUserhost = QString();

        resetQueues();

        // This is needed to support server groups with mixed SSL and nonSSL servers
        delete m_socket;
        m_socket = 0;
        resetNickSelection();

        // connect() will do a async lookup too
        if(!getConnectionSettings().server().SSLEnabled())
        {
            m_socket = new KNetwork::KBufferedSocket(QString(), QString(), 0L, "serverSocket");
            connect(m_socket, SIGNAL(connected(const KResolverEntry&)), SLOT (ircServerConnectionSuccess()));
        }
        else
        {
            m_socket = new SSLSocket(getViewContainer()->getWindow(), 0L, "serverSSLSocket");
            connect(m_socket, SIGNAL(sslInitDone()), SLOT(ircServerConnectionSuccess()));
            connect(m_socket, SIGNAL(sslFailure(const QString&)), SIGNAL(sslInitFailure()));
            connect(m_socket, SIGNAL(sslFailure(const QString&)), SLOT(sslError(const QString&)));
        }

        m_socket->enableWrite(false);

        connect(m_socket, SIGNAL(hostFound()), SLOT(lookupFinished()));
        connect(m_socket, SIGNAL(gotError(int)), SLOT(broken(int)) );
        connect(m_socket, SIGNAL(readyRead()), SLOT(incoming()));
        connect(m_socket, SIGNAL(closed()), SLOT(closed()));

        m_socket->connect(getConnectionSettings().server().host(), QString::number(getConnectionSettings().server().port()));

        // set up the connection details
        setPrefixes(m_serverNickPrefixModes, m_serverNickPrefixes);
        getStatusView()->appendServerMessage(i18n("Info"),i18n("Looking for server %1:%2...")
            .arg(getConnectionSettings().server().host())
            .arg(getConnectionSettings().server().port()));
        // reset InputFilter (auto request info, /WHO request info)
        m_inputFilter.reset();
    }
    else
        kdDebug() << "connectToIRCServer() called while already connected: This should never happen." << endl;
}

void Server::showSSLDialog()
{
    SSLSocket* sslsocket = dynamic_cast<SSLSocket*>(m_socket);

    if (sslsocket) sslsocket->showInfoDialog();
}

// set available channel types according to 005 RPL_ISUPPORT
void Server::setChannelTypes(const QString &pre)
{
    m_channelPrefixes = pre;
}

QString Server::getChannelTypes() const
{
    return m_channelPrefixes;
}

// set user mode prefixes according to non-standard 005-Reply (see inputfilter.cpp)
void Server::setPrefixes(const QString &modes, const QString& prefixes)
{
    // NOTE: serverModes is QString::null, if server did not supply the
    // modes which relates to the network's nick-prefixes
    m_serverNickPrefixModes = modes;
    m_serverNickPrefixes = prefixes;
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

    if (nickname.isEmpty()) return;

    while ((modeIndex = m_serverNickPrefixes.find(nickname[0])) != -1)
    {
        if(nickname.isEmpty())
            return;
        nickname = nickname.mid(1);
        // cut off the prefix
        bool recognisedMode = false;
        // determine, whether status is like op or like voice
        while((modeIndex)<int(m_serverNickPrefixes.length()) && !recognisedMode)
        {
            switch(m_serverNickPrefixes[modeIndex].latin1())
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
    if(m_socket->status())
    {
        // inform user about the error
        getStatusView()->appendServerMessage(i18n("Error"),i18n("Server %1 not found: %2")
            .arg(getConnectionSettings().server().host())
            .arg(m_socket->errorString(m_socket->error())));

        m_socket->resetStatus();

        // broken connection
        broken(m_socket->error());
    }
    else
        getStatusView()->appendServerMessage(i18n("Info"),i18n("Server found, connecting..."));
}

void Server::ircServerConnectionSuccess()
{
    getConnectionSettings().setReconnectCount(0);

    Konversation::ServerSettings serverSettings = getConnectionSettings().server();

    connect(this, SIGNAL(nicknameChanged(const QString&)), getStatusView(), SLOT(setNickname(const QString&)));
    getStatusView()->appendServerMessage(i18n("Info"),i18n("Connected; logging in..."));

    QString connectString = "USER " +
        getIdentity()->getIdent() +
        " 8 * :" +                                // 8 = +i; 4 = +w
        getIdentity()->getRealName();

    QStringList ql;
    if (!serverSettings.password().isEmpty())
        ql << "PASS " + serverSettings.password();

    ql << "NICK "+getNickname();
    ql << connectString;
    queueList(ql, HighPriority);

    emit nicknameChanged(getNickname());

    m_socket->enableRead(true);
}

void Server::broken(int state)
{
    kdDebug() << "Connection broken (Socket fd " << m_socket->socketDevice()->socket() << ") " << state << "!" << endl;

    m_socket->enableRead(false);
    m_socket->enableWrite(false); //FIXME if we rely on this signal, it should be turned back on somewhere...
    m_socket->blockSignals(true);

    resetQueues();

    m_notifyTimer.stop();
    m_pingResponseTimer.stop();
    m_inputFilter.setLagMeasuring(false);
    m_currentLag = -1;

    // HACK Only show one nick change dialog at connection time
    if (getStatusView())
    {
        KDialogBase* nickChangeDialog = dynamic_cast<KDialogBase*>(
                getStatusView()->child("NickChangeDialog", "KInputDialog"));

        if (nickChangeDialog) nickChangeDialog->cancel();
    }

    emit resetLag();
    emit nicksNowOnline(this,QStringList(),true);

    updateAutoJoin();

    if (getConnectionState() != Konversation::SSDeliberatelyDisconnected)
    {
        static_cast<KonversationApplication*>(kapp)->notificationHandler()->connectionFailure(getStatusView(), getServerName());

        QString error = i18n("Connection to Server %1 lost: %2.")
            .arg(getConnectionSettings().server().host())
            .arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));

        getStatusView()->appendServerMessage(i18n("Error"), error);

        updateConnectionState(Konversation::SSInvoluntarilyDisconnected);
    }
}

void Server::sslError(const QString& reason)
{
    QString error = i18n("Could not connect to %1:%2 using SSL encryption.Maybe the server does not support SSL, or perhaps you have the wrong port? %3")
        .arg(getConnectionSettings().server().host())
        .arg(getConnectionSettings().server().port())
        .arg(reason);
    getStatusView()->appendServerMessage(i18n("SSL Connection Error"),error);

    updateConnectionState(Konversation::SSDeliberatelyDisconnected);
}

// Will be called from InputFilter as soon as the Welcome message was received
void Server::connectionEstablished(const QString& ownHost)
{
    // Some servers don't include the userhost in RPL_WELCOME, so we
    // need to use RPL_USERHOST to get ahold of our IP later on
    if (!ownHost.isEmpty())
        KNetwork::KResolver::resolveAsync(this,SLOT(gotOwnResolvedHostByWelcome(KResolverResults)),ownHost,"0");

    updateConnectionState(Konversation::SSConnected);

    // Make a helper object to build ISON (notify) list and map offline nicks to addressbook.
    // TODO: Give the object a kick to get it started?
    m_serverISON = new ServerISON(this);
    // get first notify very early
    startNotifyTimer(1000);
    // Register with services
    registerWithServices();
    // get own ip by userhost
    requestUserhost(getNickname());

    // Start the PINGPONG match
    QTimer::singleShot(1000 /*1 sec*/, this, SLOT(sendPing()));

    // Recreate away state if we were set away prior to a reconnect.
    if (isAway())
    {
        // Correct server's beliefs about its away state.
        m_isAway = false;
        QString awayReason = m_awayReason.isEmpty() ? i18n("Gone away for now.") : m_awayReason;
        QString command(Preferences::commandChar() + "AWAY " + awayReason);
        Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),command, QString());
        queue(result.toServer, LowPriority);
    }
}

void Server::registerWithServices()
{
    if (getIdentity() && !getIdentity()->getBot().isEmpty()
        && !getIdentity()->getPassword().isEmpty()
        && !m_autoIdentifyLock)
    {
        queue("PRIVMSG "+getIdentity()->getBot()+" :identify "+getIdentity()->getPassword(), HighPriority);

        m_autoIdentifyLock = true;
    }
}

QCString Server::getKeyForRecipient(const QString& recipient) const
{
    return m_keyMap[recipient];
}

void Server::setKeyForRecipient(const QString& recipient, const QCString& key)
{
    m_keyMap[recipient] = key;
}

void Server::gotOwnResolvedHostByWelcome(KResolverResults res)
{
    if (res.error() == KResolver::NoError && !res.isEmpty())
        m_ownIpByWelcome = res.first().address().nodeName();
    else
        kdDebug() << "Server::gotOwnResolvedHostByWelcome(): Got error: " << ( int )res.error() << endl;
}

void Server::quitServer()
{
    // Make clear this is deliberate even if the QUIT never actually goes through the queue
    // (i.e. this is not redundant with _send_internal()'s updateConnectionState() call for
    // a QUIT).
    updateConnectionState(Konversation::SSDeliberatelyDisconnected);

    QString command(Preferences::commandChar()+"QUIT");
    Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),command, QString());
    queue(result.toServer, HighPriority);

    m_socket->enableRead(false);

    flushQueues();

    m_socket->close();

    getStatusView()->appendServerMessage(i18n("Info"), i18n("Disconnected from %1.").arg(getConnectionSettings().server().host()));
}

void Server::notifyAction(const QString& nick)
{
    // parse wildcards (toParse,nickname,channelName,nickList,parameter)
    QString out = parseWildcards(Preferences::notifyDoubleClickAction(),
        getNickname(),
        QString(),
        QString(),
        nick,
        QString());

    // Send all strings, one after another
    QStringList outList = QStringList::split('\n',out);
    for (unsigned int index=0; index<outList.count(); ++index)
    {
        Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),outList[index],QString());
        queue(result.toServer);
    }                                             // endfor
}

void Server::notifyResponse(const QString& nicksOnline)
{
    bool nicksOnlineChanged = false;
    QStringList actualList = QStringList::split(' ',nicksOnline);
    QString lcActual = ' ' + nicksOnline + ' ';
    QString lcPrevISON = ' ' + (m_prevISONList.join(" ")) + ' ';

    QStringList::iterator it;

    //Are any nicks gone offline
    for (it = m_prevISONList.begin(); it != m_prevISONList.end(); ++it)
    {
        if (lcActual.find(' ' + (*it) + ' ', 0, false) == -1)
        {
            setNickOffline(*it);
            nicksOnlineChanged = true;
        }
    }

    //Are any nicks gone online
    for (it = actualList.begin(); it != actualList.end(); ++it)
    {
        if (lcPrevISON.find(' ' + (*it) + ' ', 0, false) == -1) {
            setWatchedNickOnline(*it);
            nicksOnlineChanged = true;
        }
    }

    // Note: The list emitted in this signal *does* include nicks in joined channels.
    emit nicksNowOnline(this, actualList, nicksOnlineChanged);

    m_prevISONList = actualList;

    // Next round
    startNotifyTimer();
}

void Server::startNotifyTimer(int msec)
{
    // make sure the timer gets started properly in case we have reconnected
    m_notifyTimer.stop();

    if (msec == 0) msec = Preferences::notifyDelay()*1000;

    // start the timer in one shot mode
    m_notifyTimer.start(msec, true);
}

void Server::notifyTimeout()
{
    // Notify delay time is over, send ISON request if desired
    if (Preferences::useNotify())
    {
        // But only if there actually are nicks in the notify list
        QString list = getISONListString();

        if (!list.isEmpty()) queue("ISON "+list, LowPriority);

    }
}

void Server::autoCommandsAndChannels()
{
    if (getServerGroup() && !getServerGroup()->connectCommands().isEmpty())
    {
        QString connectCommands = getServerGroup()->connectCommands();

        if (!getNickname().isEmpty())
            connectCommands.replace("%nick", getNickname());

        QStringList connectCommandsList = QStringList::split(";", connectCommands);
        QStringList::iterator iter;

        for (iter = connectCommandsList.begin(); iter != connectCommandsList.end(); ++iter)
        {
            QString output(*iter);
            output = output.simplifyWhiteSpace();
            getOutputFilter()->replaceAliases(output);
            Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),output,QString());
            queue(result.toServer);
        }
    }

    if (getAutoJoin()) queue(getAutoJoinCommand());
}

QString Server::getAutoJoinCommand() const
{
    // Multichannel joins
    QStringList channels = QStringList::split(' ', m_autoJoinChannels);
    QStringList passwords = QStringList::split(' ', m_autoJoinChannelPasswords);

    QString autoString("JOIN "+channels.join(",")+' '+passwords.join(","));

    return autoString;
}

/** Create a set of indices into the nickname list of the current identity based on the current nickname.
 *
 * The index list is only used if the current nickname is not available. If the nickname is in the identity,
 * we do not want to retry it. If the nickname is not in the identity, it is considered to be at position -1.
 */
void Server::resetNickSelection()
{
    m_nickIndices.clear();
    //for equivalence testing in case the identity gets changed underneath us
    m_referenceNicklist = getIdentity()->getNicknameList();
    //where in this identities nicklist will we have started?
    int start = m_referenceNicklist.findIndex(getNickname());
    int len = m_referenceNicklist.count();

    //we first use this list of indices *after* we've already tried the current nick, which we don't want
    //to retry if we wrapped, so exclude its index here
    //if it wasn't in the list, we get -1 back, so then we *want* to include 0
    for (int i=start+1; i<len; i++)
        m_nickIndices.append(i);
    //now, from the beginning of the list, to the item before start
    for (int i=0; i<start; i++)
        m_nickIndices.append(i);
    //cause it to try to get an invalid nick number
    m_nickIndices.append(len);
}

QString Server::getNextNickname()
{
     //if the identity changed underneath us (likely impossible), start over
    if (m_referenceNicklist != getIdentity()->getNicknameList())
        resetNickSelection();

    QString newNick = getIdentity()->getNickname(m_nickIndices.front());
    m_nickIndices.pop_front();

    if (newNick.isNull())
    {
        QString inputText = i18n("No nicknames from the \"%1\" identity were accepted by the connection \"%2\".\nPlease enter a new one or press Cancel to disconnect:").arg(getIdentity()->getName()).arg(getDisplayName());
        newNick = KInputDialog::getText(i18n("Nickname error"), inputText,
                                        QString(), 0, getStatusView(), "NickChangeDialog");
    }

    return newNick;
}

void Server::processIncomingData()
{
    m_incomingTimer.stop();

    if (!m_inputBuffer.isEmpty() && !m_processingIncoming)
    {
        m_processingIncoming = true;
        QString front(m_inputBuffer.front());
        m_inputBuffer.pop_front();
        if (m_rawLog)
        {
            QString toRaw = front;
            m_rawLog->appendRaw("&gt;&gt; " + toRaw.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace(QRegExp("\\s"), "&nbsp;"));
        }
        m_inputFilter.parseLine(front);
        m_processingIncoming = false;

        if (!m_inputBuffer.isEmpty()) m_incomingTimer.start(0);
    }
}

void Server::incoming()
{
    if (getConnectionSettings().server().SSLEnabled())
        emit sslConnected(this);

    // We read all available bytes here because readyRead() signal will be emitted when there is new data
    // else we will stall when displaying MOTD etc.
    int max_bytes = m_socket->bytesAvailable();

    QByteArray buffer(max_bytes+1);
    int len = 0;

    // Read at max "max_bytes" bytes into "buffer"
    len = m_socket->readBlock(buffer.data(),max_bytes);

    if (len <= 0 && getConnectionSettings().server().SSLEnabled())
        return;

    if (len <= 0) // Zero means buffer is empty which shouldn't happen because readyRead signal is emitted
    {
        getStatusView()->appendServerMessage(i18n("Error"),
            i18n("There was an error reading the data from the server: %1").
            arg(m_socket->errorString()));

        broken(m_socket->error());
        return;
    }

    buffer[len] = 0;

    QCString qcsBuffer = m_inputBufferIncomplete + QCString(buffer);

    // split buffer to lines
    QValueList<QCString> qcsBufferLines;
    int lastLFposition = -1;
    for( int nextLFposition ; ( nextLFposition = qcsBuffer.find('\n', lastLFposition+1) ) != -1 ; lastLFposition = nextLFposition )
        qcsBufferLines << qcsBuffer.mid(lastLFposition+1, nextLFposition-lastLFposition-1);

    // remember the incomplete line (split by packets)
    m_inputBufferIncomplete = qcsBuffer.right(qcsBuffer.length()-lastLFposition-1);

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
            m_inputBuffer << QString::fromUtf8(front);
        else
        {
            // check setting
            QString channelEncoding;
            if( !channelKey.isEmpty() )
            {
                channelEncoding = Preferences::channelEncoding(getDisplayName(), channelKey);
            }
            // END set channel encoding if specified

            if( !channelEncoding.isEmpty() )
                codec = Konversation::IRCCharsets::self()->codecForName(channelEncoding);

            // if channel encoding is utf-8 and the string is definitely not utf-8
            // then try latin-1
            if ( !isUtf8 && codec->mibEnum() == 106 )
                codec = QTextCodec::codecForMib( 4 /* iso-8859-1 */ );

            m_inputBuffer << codec->toUnicode(front);
        }
        qcsBufferLines.pop_front();
        m_bytesReceived+=m_inputBuffer.back().length();
    }

    if( !m_incomingTimer.isActive() && !m_processingIncoming )
        m_incomingTimer.start(0);
}

/** Calculate how long this message premable will be.

    This is necessary because the irc server will clip messages so that the
    client receives a maximum of 512 bytes at once.
*/
int Server::getPreLength(const QString& command, const QString& dest)
{
    NickInfo* info = getNickInfo(getNickname());
    int hostMaskLength = 0;

    if(info)
        hostMaskLength = info->getHostmask().length();

    //:Sho_!i=ehs1@konversation/developer/hein PRIVMSG #konversation :and then back to it

    //<colon>$nickname<!>$hostmask<space>$command<space>$destination<space><colon>$message<cr><lf>
    int x= 512 - 8 - (m_nickname.length() + hostMaskLength + command.length() + dest.length());

    return x;
}

int Server::_send_internal(QString outputLine)
{
    QStringList outputLineSplit=QStringList::split(" ", outputLine);
    // NOTE: It's important to add the linefeed here, so the encoding process does not trash it
    //       for some servers.
    if (!outputLine.endsWith("\n"))
        outputLine+='\n';

    //Lets cache the uppercase command so we don't miss or reiterate too much
    QString outboundCommand(outputLineSplit[0].upper());

    // remember the first arg of /WHO to identify responses
    if (!outputLineSplit.isEmpty() && outboundCommand=="WHO")
    {
        if (outputLineSplit.count()>=2)
            m_inputFilter.addWhoRequest(outputLineSplit[1]);
        else // no argument (servers recognize it as "*")
            m_inputFilter.addWhoRequest("*");
    }

    else if (outboundCommand=="QUIT")
        updateConnectionState(Konversation::SSDeliberatelyDisconnected);

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
        {
            channelCodecName=Preferences::channelEncoding(getDisplayName(),outputLineSplit[1]);
        }
    }

    QTextCodec* codec = getIdentity()->getCodec();

    if(!channelCodecName.isEmpty())
    {
        codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);
    }

    // Some codecs don't work with a negative value. This is a bug in Qt 3.
    // ex.: JIS7, eucJP, SJIS
    //int outlen=-1;
    int outlen=outputLine.length();

    QCString encoded=codec->fromUnicode(outputLine, outlen);

    // Blowfish
    if(outboundCommand=="PRIVMSG" || outboundCommand=="TOPIC")
    {
        Konversation::encrypt(outputLineSplit[1],outputLine,this);
    }

    Q_LONG sout = m_socket->writeBlock(encoded, outlen);

    if (m_rawLog) m_rawLog->appendRaw("&lt;&lt; " + outputLine.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));

    return sout;
}

void Server::toServer(QString&s, IRCQueue* q)
{

    int sizesent = _send_internal(s);
    emit sentStat(s.length(), sizesent, q); //tell the queues what we sent
    //tell everyone else
    emit sentStat(s.length(), sizesent);
}

void Server::collectStats(int bytes, int encodedBytes)
{
    m_bytesSent += bytes;
    m_encodedBytesSent += encodedBytes;
    m_linesSent++;
}

bool Server::validQueue(QueuePriority priority)
{
   if (priority >=0 && priority <= _max_queue())
       return true;
   return false;
}

bool Server::queue(const QString& line, QueuePriority priority)
{
    if (!line.isEmpty() && validQueue(priority))
    {
        IRCQueue& out=*m_queues[priority];
        out.enqueue(line);
        return true;
    }
    return false;
}

bool Server::queueList(const QStringList& buffer, QueuePriority priority)
{
    if (buffer.isEmpty() || !validQueue(priority))
        return false;

    IRCQueue& out=*(m_queues[priority]);

    for(unsigned int i=0;i<buffer.count();i++)
    {
        QString line=*buffer.at(i);
        if (!line.isEmpty())
            out.enqueue(line);
    }
    return true;
}

void Server::resetQueues()
{
    for (int i=0;i<=_max_queue();i++)
        m_queues[i]->reset();
}

//this could flood you off, but you're leaving anyway...
void Server::flushQueues()
{
    int cue;
    do
    {
        cue=-1;
        int wait=0;
        for (int i=1;i<=_max_queue();i++) //slow queue can rot
        {
            IRCQueue *queue=m_queues[i];
            //higher queue indices have higher priorty, higher queue priority wins tie
            if (!queue->isEmpty() && queue->currentWait()>=wait)
            {
                cue=i;
                wait=queue->currentWait();
            }
        }
        if (cue>-1)
            m_queues[cue]->sendNow();
    } while (cue>-1);
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
    QString lcNickname = nickname.lower();
    const ChannelNickMap *channelNickMap = getChannelMembers(channelName);
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

QString Server::getOwnIpByNetworkInterface()
{
    return m_socket->localAddress().nodeName();
}

QString Server::getOwnIpByServerMessage()
{
    if(!m_ownIpByWelcome.isEmpty())
        return m_ownIpByWelcome;
    else if(!m_ownIpByUserhost.isEmpty())
        return m_ownIpByUserhost;
    else
        return QString();
}

class Query* Server::addQuery(const NickInfoPtr & nickInfo, bool weinitiated)
{
    QString nickname = nickInfo->getNickname();
    // Only create new query object if there isn't already one with the same name
    class Query* query=getQueryByName(nickname);

    if (!query)
    {
        QString lcNickname = nickname.lower();
        query = getViewContainer()->addQuery(this, nickInfo, weinitiated);

        connect(query, SIGNAL(sendFile(const QString&)),this, SLOT(requestDccSend(const QString&)));
        connect(this, SIGNAL(serverOnline(bool)), query, SLOT(serverOnline(bool)));

        // Append query to internal list
        m_queryList.append(query);

        m_queryNicks.insert(lcNickname, nickInfo);

        if (!weinitiated)
            static_cast<KonversationApplication*>(kapp)->notificationHandler()->query(query, nickname);
    }

    // try to get hostmask if there's none yet
    if (query->getNickInfo()->getHostmask().isEmpty()) requestUserhost(nickname);

    Q_ASSERT(query);

    return query;
}

void Server::closeQuery(const QString &name)
{
    class Query* query = getQueryByName(name);
    removeQuery(query);

    // Update NickInfo.  If no longer on any lists, delete it altogether, but
    // only if not on the watch list.  ISON replies will determine whether the NickInfo
    // is deleted altogether in that case.
    QString lcNickname = name.lower();
    m_queryNicks.remove(lcNickname);
    if (!isWatchedNick(name)) deleteNickIfUnlisted(name);
}

void Server::closeChannel(const QString& name)
{
    kdDebug() << "Server::closeChannel(" << name << ")" << endl;
    Channel* channelToClose = getChannelByName(name);

    if(channelToClose)
    {
        Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),
            Preferences::commandChar() + "PART", name);
        queue(result.toServer);
    }
}

void Server::requestChannelList()
{
    m_inputFilter.setAutomaticRequest("LIST", QString(), true);
    queue(QString("LIST"));
}

void Server::requestWhois(const QString& nickname)
{
    m_inputFilter.setAutomaticRequest("WHOIS", nickname, true);
    queue("WHOIS "+nickname, LowPriority);
}

void Server::requestWho(const QString& channel)
{
    m_inputFilter.setAutomaticRequest("WHO", channel, true);
    queue("WHO "+channel, LowPriority);
}

void Server::requestUserhost(const QString& nicks)
{
    QStringList nicksList = QStringList::split(" ", nicks);
    for(QStringList::ConstIterator it=nicksList.begin() ; it!=nicksList.end() ; ++it)
        m_inputFilter.setAutomaticRequest("USERHOST", *it, true);
    queue("USERHOST "+nicks, LowPriority);
}

void Server::requestTopic(const QString& channel)
{
    m_inputFilter.setAutomaticRequest("TOPIC", channel, true);
    queue("TOPIC "+channel, LowPriority);
}

void Server::resolveUserhost(const QString& nickname)
{
    m_inputFilter.setAutomaticRequest("WHOIS", nickname, true);
    m_inputFilter.setAutomaticRequest("DNS", nickname, true);
    queue("WHOIS "+nickname, LowPriority); //FIXME when is this really used?
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
                QString hostmask=targetNick->getChannelNick()->getHostmask();
                // if we found the hostmask, add it to the ban mask
                if(!hostmask.isEmpty())
                {
                    mask=targetNick->getChannelNick()->getNickname()+'!'+hostmask;

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

        Konversation::OutputFilterResult result = getOutputFilter()->execBan(mask,channel);
        queue(result.toServer);
    }
}

void Server::requestUnban(const QString& mask,const QString& channel)
{
    Konversation::OutputFilterResult result = getOutputFilter()->execUnban(mask,channel);
    queue(result.toServer);
}

void Server::requestDccSend()
{
    requestDccSend(QString());
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
        Channel* lookChannel=m_channelList.first();

        // fill nickList with all nicks we know about
        while (lookChannel)
        {
            QPtrList<Nick> nicks=lookChannel->getNickList();
            Nick* lookNick=nicks.first();
            while(lookNick)
            {
                if(!nickList.contains(lookNick->getChannelNick()->getNickname())) nickList.append(lookNick->getChannelNick()->getNickname());
                lookNick=nicks.next();
            }
            lookChannel=m_channelList.next();
        }

        // add Queries as well, but don't insert duplicates
        class Query* lookQuery=m_queryList.first();
        while(lookQuery)
        {
            if(!nickList.contains(lookQuery->getName())) nickList.append(lookQuery->getName());
            lookQuery=m_queryList.next();
        }

        recipient=DccRecipientDialog::getNickname(getViewContainer()->getWindow(),nickList);
    }
    // do we have a recipient *now*?
    if(!recipient.isEmpty())
    {
        KURL::List fileURLs=KFileDialog::getOpenURLs(
            ":lastDccDir",
            QString(),
            getViewContainer()->getWindow(),
            i18n("Select File(s) to Send to %1").arg(recipient)
        );
        KURL::List::iterator it;
        for ( it = fileURLs.begin() ; it != fileURLs.end() ; ++it )
        {
            addDccSend( recipient, *it );
        }
    }
}

void Server::slotNewDccTransferItemQueued(DccTransfer* transfer)
{
    if (transfer->getConnectionId() == connectionId() )
    {
        kdDebug() << "Server::slotNewDccTranfserItemQueued(): connecting slots for " << transfer->getFileName() << " [" << transfer->getType() << "]" << endl;
        if ( transfer->getType() == DccTransfer::Receive )
        {
            connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( dccGetDone( DccTransfer* ) ) );
            connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( dccStatusChanged( DccTransfer*, int, int ) ) );
        }
        else
        {
            connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( dccSendDone( DccTransfer* ) ) );
            connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( dccStatusChanged( DccTransfer*, int, int ) ) );
        }
    }
}

void Server::addDccSend(const QString &recipient,KURL fileURL, const QString &altFileName, uint fileSize)
{
    if (!fileURL.isValid()) return;

    emit addDccPanel();

    // We already checked that the file exists in output filter / requestDccSend() resp.
    DccTransferSend* newDcc = KonversationApplication::instance()->dccTransferManager()->newUpload();

    newDcc->setConnectionId( connectionId() );

    newDcc->setPartnerNick( recipient );
    newDcc->setFileURL( fileURL );
    if ( !altFileName.isEmpty() )
        newDcc->setFileName( altFileName );
    if ( fileSize != 0 )
        newDcc->setFileSize( fileSize );

    if ( newDcc->queue() )
        newDcc->start();
}

void Server::addDccGet(const QString &sourceNick, const QStringList &dccArguments)
{
    emit addDccPanel();

    DccTransferRecv* newDcc = KonversationApplication::instance()->dccTransferManager()->newDownload();

    newDcc->setConnectionId( connectionId() );

    newDcc->setPartnerNick( sourceNick );
    newDcc->setPartnerIp( DccCommon::numericalIpToTextIp( dccArguments[1] ) );
    newDcc->setPartnerPort( dccArguments[2] );
    if ( dccArguments[2] == "0" && dccArguments.count() == 5)  // Reverse DCC
        newDcc->setReverse( true, dccArguments[4] );

    newDcc->setFileName( dccArguments[0] );
    newDcc->setFileSize( dccArguments[3].isEmpty() ? 0 : dccArguments[3].toULong() );

    if ( newDcc->queue() )
    {
        QString showfile = newDcc->getFileName();

        if(showfile.startsWith("\"") && showfile.endsWith("\""))
            showfile = showfile.mid(1, showfile.length() - 2);

        appendMessageToFrontmost( i18n( "DCC" ),
                                  i18n( "%1 offers to send you \"%2\" (%3)..." )
                                  .arg( newDcc->getPartnerNick(),
                                        showfile,
                                        ( newDcc->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( newDcc->getFileSize() ) ) );

        if(Preferences::dccAutoGet())
            newDcc->start();
    }
}

void Server::openDccChat(const QString& nickname)
{
    emit addDccChat(getNickname(),nickname,QStringList(),true);
}

void Server::requestDccChat(const QString& partnerNick, const QString& numericalOwnIp, const QString& ownPort)
{
    queue(QString("PRIVMSG %1 :\001DCC CHAT chat %2 %3\001").arg(partnerNick).arg(numericalOwnIp).arg(ownPort));
}

void Server::dccSendRequest(const QString &partner, const QString &fileName, const QString &address, const QString &port, unsigned long size)
{
    Konversation::OutputFilterResult result = getOutputFilter()->sendRequest(partner,fileName,address,port,size);
    queue(result.toServer);

    QString showfile = fileName;

    if(showfile.startsWith("\"") && showfile.endsWith("\""))
        showfile = showfile.mid(1, showfile.length() - 2);

    appendMessageToFrontmost( i18n( "DCC" ),
                              i18n( "Asking %1 to accept upload of \"%2\" (%3)..." )
                              .arg( partner,
                                    showfile,
                                    ( size == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( size ) ) );
}

void Server::dccPassiveSendRequest(const QString& recipient,const QString& fileName,const QString& address,unsigned long size,const QString& token)
{
    Konversation::OutputFilterResult result = getOutputFilter()->passiveSendRequest(recipient,fileName,address,size,token);
    queue(result.toServer);
}

void Server::dccResumeGetRequest(const QString &sender, const QString &fileName, const QString &port, KIO::filesize_t startAt)
{
    Konversation::OutputFilterResult result;

    if (fileName.contains(" ") > 0)
        result = getOutputFilter()->resumeRequest(sender,"\""+fileName+"\"",port,startAt);
    else
        result = getOutputFilter()->resumeRequest(sender,fileName,port,startAt);

    queue(result.toServer);
}

void Server::dccReverseSendAck(const QString& partnerNick,const QString& fileName,const QString& ownAddress,const QString& ownPort,unsigned long size,const QString& reverseToken)
{
    Konversation::OutputFilterResult result = getOutputFilter()->acceptPassiveSendRequest(partnerNick,fileName,ownAddress,ownPort,size,reverseToken);
    queue(result.toServer);
}

void Server::startReverseDccSendTransfer(const QString& sourceNick,const QStringList& dccArguments)
{
    DccTransferManager* dtm = KonversationApplication::instance()->dccTransferManager();

    if ( dtm->startReverseSending( connectionId(), sourceNick,
                                   dccArguments[0],  // filename
                                   DccCommon::numericalIpToTextIp( dccArguments[1] ),  // partner IP
                                   dccArguments[2],  // partner port
                                   dccArguments[3].toInt(),  // filesize
                                   dccArguments[4]  // Reverse DCC token
         ) == 0 )
    {
        QString showfile = dccArguments[0];

        if(showfile.startsWith("\"") && showfile.endsWith("\""))
            showfile = showfile.mid(1, showfile.length() - 2);

        // DTM could not find a matched item
        appendMessageToFrontmost( i18n( "Error" ),
                                  i18n( "%1 = file name, %2 = nickname",
                                        "Received invalid passive DCC send acceptance message for \"%1\" from %2." )
                                  .arg( showfile,
                                        sourceNick ) );

    }

}

void Server::resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
    DccTransferManager* dtm = KonversationApplication::instance()->dccTransferManager();

    QString fileName( dccArguments[0] );
    QString ownPort( dccArguments[1] );
    unsigned long position = dccArguments[2].toULong();

    DccTransferRecv* dccTransfer = dtm->resumeDownload( connectionId(), sourceNick, fileName, ownPort, position );

    QString showfile = fileName;

    if(showfile.startsWith("\"") && showfile.endsWith("\""))
        showfile = showfile.mid(1, showfile.length() - 2);

    if ( dccTransfer )
    {
        appendMessageToFrontmost( i18n( "DCC" ),
                                  i18n( "%1 = file name, %2 = nickname of sender, %3 = percentage of file size, %4 = file size",
                                        "Resuming download of \"%1\" from %2 starting at %3% of %4..." )
                                  .arg( showfile,
                                        sourceNick,
                                        QString::number( dccTransfer->getProgress() ),
                                        ( dccTransfer->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( dccTransfer->getFileSize() ) ) );
    }
    else
    {
        appendMessageToFrontmost( i18n( "Error" ),
                                  i18n( "%1 = file name, %2 = nickname",
                                        "Received invalid resume acceptance message for \"%1\" from %2." )
                                  .arg( showfile,
                                        sourceNick ) );
    }
}

void Server::resumeDccSendTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
    DccTransferManager* dtm = KonversationApplication::instance()->dccTransferManager();

    QString fileName( dccArguments[0] );
    QString ownPort( dccArguments[1] );
    unsigned long position = dccArguments[2].toULong();

    DccTransferSend* dccTransfer = dtm->resumeUpload( connectionId(), sourceNick, fileName, ownPort, position );

    QString showfile = fileName;

    if(showfile.startsWith("\"") && showfile.endsWith("\""))
        showfile = showfile.mid(1, showfile.length() - 2);

    if ( dccTransfer )
    {
        appendMessageToFrontmost( i18n( "DCC" ),
                                  i18n( "%1 = file name, %2 = nickname of recipient, %3 = percentage of file size, %4 = file size",
                                        "Resuming upload of \"%1\" to %2 starting at %3% of %4...")
                                  .arg( showfile,
                                        sourceNick,
                                        QString::number(dccTransfer->getProgress()),
                                        ( dccTransfer->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( dccTransfer->getFileSize() ) ) );

        // FIXME: this operation should be done by DccTransferManager
        Konversation::OutputFilterResult result = getOutputFilter()->acceptResumeRequest( sourceNick, fileName, ownPort, position );
        queue( result.toServer );

    }
    else
    {
        appendMessageToFrontmost( i18n( "Error" ),
                                  i18n( "%1 = file name, %2 = nickname",
                                        "Received invalid resume request for \"%1\" from %2." )
                                  .arg( showfile,
                                        sourceNick ) );
    }
}

void Server::dccGetDone(DccTransfer* item)
{
    if (!item)
        return;

    QString showfile = item->getFileName();

    if(showfile.startsWith("\"") && showfile.endsWith("\""))
        showfile = showfile.mid(1, showfile.length() - 2);

    if(item->getStatus()==DccTransfer::Done)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of sender",
            "Download of \"%1\" from %2 finished.").arg(showfile, item->getPartnerNick()));
    else if(item->getStatus()==DccTransfer::Failed)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of sender",
            "Download of \"%1\" from %2 failed. Reason: %3.").arg(showfile,
            item->getPartnerNick(), item->getStatusDetail()));
}

void Server::dccSendDone(DccTransfer* item)
{
    if (!item)
        return;

    QString showfile = item->getFileName();

    if(showfile.startsWith("\"") && showfile.endsWith("\""))
        showfile = showfile.mid(1, showfile.length() - 2);

    if(item->getStatus()==DccTransfer::Done)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of recipient",
            "Upload of \"%1\" to %2 finished.").arg(showfile, item->getPartnerNick()));
    else if(item->getStatus()==DccTransfer::Failed)
        appendMessageToFrontmost(i18n("DCC"),i18n("%1 = file name, %2 = nickname of recipient",
            "Upload of \"%1\" to %2 failed. Reason: %3.").arg(showfile, item->getPartnerNick(),
            item->getStatusDetail()));
}

void Server::dccStatusChanged(DccTransfer *item, int newStatus, int oldStatus)
{
    if(!item)
        return;

    QString showfile = item->getFileName();

    if(showfile.startsWith("\"") && showfile.endsWith("\""))
        showfile = showfile.mid(1, showfile.length() - 2);

    if ( item->getType() == DccTransfer::Send )
    {
        // when resuming, a message about the receiver's acceptance has been shown already, so suppress this message
        if ( newStatus == DccTransfer::Transferring && oldStatus == DccTransfer::WaitingRemote && !item->isResumed() )
            appendMessageToFrontmost( i18n( "DCC" ), i18n( "%1 = file name, %2 nickname of recipient",
                "Sending \"%1\" to %2...").arg( showfile, item->getPartnerNick() ) );
    }
    else  // type == Receive
    {
        if ( newStatus == DccTransfer::Transferring && !item->isResumed() )
        {
            appendMessageToFrontmost( i18n( "DCC" ),
                                        i18n( "%1 = file name, %2 = file size, %3 = nickname of sender", "Downloading \"%1\" (%2) from %3...")
                                        .arg( showfile,
                                            ( item->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( item->getFileSize() ),
                                            item->getPartnerNick() ) );
        }
    }
}

void Server::removeQuery(class Query* query)
{
    // Traverse through list to find the query
    class Query* lookQuery = m_queryList.first();

    while (lookQuery)
    {
        // Did we find our query?
        if (lookQuery == query)
        {
            // Remove it from the query list
            m_queryList.remove(lookQuery);
            // break out of the loop
            lookQuery = 0;
        }
        // else select next query
        else lookQuery = m_queryList.next();
    }

    query->deleteLater();
}

void Server::sendJoinCommand(const QString& name, const QString& password)
{
    Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),
        Preferences::commandChar() + "JOIN " + name + ' ' + password, QString());
    queue(result.toServer);
}

void Server::joinChannel(const QString& name, const QString& hostmask)
{
    // (re-)join channel, open a new panel if needed
    Channel* channel = getChannelByName(name);

    if (!channel)
    {
        channel=getViewContainer()->addChannel(this,name);
        Q_ASSERT(channel);
        channel->setIdentity(getIdentity());
        channel->setNickname(getNickname());
        channel->indicateAway(isAway());

        if (getServerGroup())
        {
            Konversation::ChannelSettings channelSettings = getServerGroup()->channelByNameFromHistory(name);
            channel->setNotificationsEnabled(channelSettings.enableNotifications());
            getServerGroup()->appendChannelHistory(channelSettings);
        }

        m_channelList.append(channel);

        connect(channel,SIGNAL (sendFile()),this,SLOT (requestDccSend()) );
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

    if (getServerGroup())
    {
        Konversation::ChannelSettings channelSettings = getServerGroup()->channelByNameFromHistory(channel->getName());
        channelSettings.setNotificationsEnabled(channel->notificationsEnabled());
        getServerGroup()->appendChannelHistory(channelSettings);
    }

    m_channelList.removeRef(channel);
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
    Channel* channel=m_channelList.first();

    while (channel)
    {
        channel->updateQuickButtons(Preferences::quickButtonList());
        channel = m_channelList.next();
    }
}

Channel* Server::getChannelByName(const QString& name)
{
    // Convert wanted channel name to lowercase
    QString wanted=name;
    wanted=wanted.lower();

    // Traverse through list to find the channel named "name"
    Channel* lookChannel =m_channelList.first();
    while (lookChannel)
    {
        if (lookChannel->getName().lower()==wanted) return lookChannel;
        lookChannel = m_channelList.next();
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
    class Query* lookQuery=m_queryList.first();
    while(lookQuery)
    {
        if(lookQuery->getName().lower()==wanted) return lookQuery;
        lookQuery=m_queryList.next();
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
        m_allNicks.insert(lcNickname, nickInfo);
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
    }

    emit watchedNickChanged(this, nickname, true);
    KABC::Addressee addressee = nickInfo->getAddressee();
    if (!addressee.isEmpty()) Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid());

    appendMessageToFrontmost(i18n("Notify"),"<a href=\"#"+nickname+"\">"+
        i18n("%1 is online (%2).").arg(nickname).arg(getServerName())+"</a>", getStatusView());

    static_cast<KonversationApplication*>(kapp)->notificationHandler()->nickOnline(getStatusView(), nickname);

    nickInfo->setPrintedOnline(true);
    return nickInfo;
}

void Server::setWatchedNickOffline(const QString& nickname, const NickInfoPtr nickInfo)
{
    if (nickInfo) {
        KABC::Addressee addressee = nickInfo->getAddressee();
        if (!addressee.isEmpty()) Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid(), 1);
    }

    emit watchedNickChanged(this, nickname, false);

    appendMessageToFrontmost(i18n("Notify"), i18n("%1 went offline (%2).").arg(nickname).arg(getServerName()), getStatusView());

    static_cast<KonversationApplication*>(kapp)->notificationHandler()->nickOffline(getStatusView(), nickname);

}

bool Server::setNickOffline(const QString& nickname)
{
    QString lcNickname = nickname.lower();
    NickInfoPtr nickInfo = getNickInfo(lcNickname);
    bool wasOnline = nickInfo->getPrintedOnline();

    if (nickInfo && wasOnline)
    {
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
        if (isWatchedNick(nickname)) setWatchedNickOffline(nickname, nickInfo);

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
bool Server::deleteNickIfUnlisted(const QString &nickname)
{
    QString lcNickname = nickname.lower();
    // Don't delete our own nickinfo.
    if (lcNickname == loweredNickname()) return false;

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
    return Preferences::notifyListByGroupName(getDisplayName());
    if (m_serverISON)
        return m_serverISON->getWatchList();
    else
        return QStringList();
}

QString Server::getWatchListString() { return getWatchList().join(" "); }

QStringList Server::getISONList()
{
    // no nickinfo ISON for the time being
    return Preferences::notifyListByGroupName(getDisplayName());
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
    // Get watch list from preferences.
    QString watchlist= ' ' + getWatchListString() + ' ';
    // Search case-insensitivly
    return (watchlist.find(' ' + nickname + ' ', 0, 0) != -1);
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
        QString lcNickname = nickInfo->loweredNickname();
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

        if(channelNick)
        {
          outChannel->kickNick(channelNick, kicker, reason);
          // Tell Nickinfo
          removeChannelNick(channelName,nickname);
        }
    }
}

void Server::removeNickFromServer(const QString &nickname,const QString &reason)
{
    Channel* channel = m_channelList.first();
    while (channel)
    {
        // Check if nick is in this channel or not.
        if(channel->getNickByName(nickname))
            removeNickFromChannel(channel->getName(),nickname,reason,true);

        channel = m_channelList.next();
    }

    Query* query=getQueryByName(nickname);
    if (query) query->quitNick(reason);

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
    if (nickname == getNickname())
    {
        setNickname(newNick);

        // We may get a request from nickserv, so remove the auto-identify lock.
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
        Channel* channel=m_channelList.first();
        while (channel)
        {
            // All we do is notify that the nick has been renamed.. we haven't actually renamed it yet
            // Note that NickPanel has already updated, so pass new nick to getNickByName.
            if (channel->getNickByName(newNick)) channel->nickRenamed(nickname, *nickInfo);
            channel = m_channelList.next();
        }
        //Watched nicknames stuff
        if (isWatchedNick(nickname)) setWatchedNickOffline(nickname, 0);
    }
    // If we had a query with this nick, change that name, too

}

void Server::userhost(const QString& nick,const QString& hostmask,bool away,bool /* ircOp */)
{
    addHostmaskToNick(nick, hostmask);
    // remember my IP for DCC things
                                                  // myself
    if (m_ownIpByUserhost.isEmpty() && nick == getNickname())
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
        m_ownIpByUserhost = res.first().address().nodeName();
    else
        kdDebug() << "Server::gotOwnResolvedHostByUserhost(): Got error: " << ( int )res.error() << endl;
}

void Server::appendServerMessageToChannel(const QString& channel,const QString& type,const QString& message)
{
    Channel* outChannel = getChannelByName(channel);
    if (outChannel) outChannel->appendServerMessage(type,message);
}

void Server::appendCommandMessageToChannel(const QString& channel,const QString& command,const QString& message, bool highlight)
{
    Channel* outChannel = getChannelByName(channel);
    if (outChannel)
    {
        outChannel->appendCommandMessage(command,message,true,true,!highlight);
    }
    else
    {
        appendStatusMessage(command, QString("%1 %2").arg(channel).arg(message));
    }
}

void Server::appendStatusMessage(const QString& type,const QString& message)
{
    getStatusView()->appendServerMessage(type,message);
}

void Server::appendMessageToFrontmost(const QString& type,const QString& message, bool parseURL)
{
    getViewContainer()->appendToFrontmost(type, message, getStatusView(), parseURL);
}

void Server::setNickname(const QString &newNickname)
{
    m_nickname = newNickname;
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
    return (m_nickname == compare);
}

QString Server::getNickname() const
{
    return m_nickname;
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
            if(getConnectionSettings().server().password().isEmpty())
                out.append(getConnectionSettings().server().password());
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

void Server::sendToAllChannels(const QString &text)
{
    // Send a message to all channels we are in
    Channel* channel = m_channelList.first();
    while (channel)
    {
        channel->sendChannelText(text);
        channel = m_channelList.next();
    }
}

void Server::invitation(const QString& nick,const QString& channel)
{
    if(KMessageBox::questionYesNo(getViewContainer()->getWindow(),
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
        m_nonAwayNick = getNickname();
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
    m_awayReason = QString();
    emit awayState(false);

    if(!getIdentity()->getAwayNick().isEmpty() && !m_nonAwayNick.isEmpty())
        queue("NICK " + m_nonAwayNick);

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
    if (!m_rawLog) m_rawLog = getViewContainer()->addRawLog(this);

    connect(this, SIGNAL(serverOnline(bool)), m_rawLog, SLOT(serverOnline(bool)));

    // bring raw log to front since the main window does not do this for us
    if (show) emit showView(m_rawLog);
}

void Server::closeRawLog()
{
    if (m_rawLog) delete m_rawLog;
}

ChannelListPanel* Server::addChannelListPanel()
{
    if(!m_channelListPanel)
    {
        m_channelListPanel = getViewContainer()->addChannelListPanel(this);

        connect(m_channelListPanel, SIGNAL(refreshChannelList()), this, SLOT(requestChannelList()));
        connect(m_channelListPanel, SIGNAL(joinChannel(const QString&)), this, SLOT(sendJoinCommand(const QString&)));
        connect(this, SIGNAL(serverOnline(bool)), m_channelListPanel, SLOT(serverOnline(bool)));
    }

    return m_channelListPanel;
}

void Server::addToChannelList(const QString& channel, int users, const QString& topic)
{
    addChannelListPanel();
    m_channelListPanel->addToChannelList(channel, users, topic);
}

ChannelListPanel* Server::getChannelListPanel() const
{
    return m_channelListPanel;
}

void Server::closeChannelListPanel()
{
    if (m_channelListPanel) delete m_channelListPanel;
}

void Server::updateAutoJoin(Konversation::ChannelSettings channel)
{
    if (!channel.name().isEmpty())
    {
        setAutoJoin(true);

        setAutoJoinChannels(channel.name());
        setAutoJoinChannelPasswords(channel.password());

        return;
    }

    Konversation::ChannelList tmpList;

    if (m_channelList.isEmpty() && getServerGroup())
        tmpList = getServerGroup()->channelList();
    else
    {
        QPtrListIterator<Channel> it(m_channelList);
        Channel* channel;

        while ((channel = it.current()) != 0)
        {
            ++it;
            tmpList << channel->channelSettings();
        }
    }

    if (!tmpList.isEmpty())
    {
        setAutoJoin(true);

        QStringList channels;
        QStringList passwords;

        Konversation::ChannelList::iterator it;

        for (it = tmpList.begin(); it != tmpList.end(); ++it)
        {
            channels << (*it).name();
            passwords << ((*it).password().isEmpty() ? "." : (*it).password());
        }

        if (passwords.last() == ".") passwords.pop_back();

        setAutoJoinChannels(channels.join(","));
        setAutoJoinChannelPasswords(passwords.join(","));
    }
    else
        setAutoJoin(false);
}

ViewContainer* Server::getViewContainer() const
{
    KonversationApplication* konvApp = static_cast<KonversationApplication *>(kapp);
    return konvApp->getMainWindow()->getViewContainer();
}

bool Server::getUseSSL() const
{
    SSLSocket* sslsocket = dynamic_cast<SSLSocket*>(m_socket);

    return (sslsocket != 0);
}

QString Server::getSSLInfo() const
{
    SSLSocket* sslsocket = dynamic_cast<SSLSocket*>(m_socket);

    if(sslsocket)
        return sslsocket->details();

    return QString();
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

        Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(), str, QString());
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
    Channel* channel = m_channelList.first();

    while (channel)
    {
        channel->sendChannelText(text);
        channel = m_channelList.next();
    }

    // Send a message to all queries we are in
    class Query* query = m_queryList.first();

    while (query)
    {
        query->sendQueryText(text);
        query = m_queryList.next();
    }
}

bool Server::isSocketConnected() const
{
    if (!m_socket) return false;

    return (m_socket->state() == KNetwork::KClientSocketBase::Connected);
}

void Server::updateConnectionState(Konversation::ConnectionState state)
{
    if (state != m_connectionState)
    {
        m_connectionState = state;

        if (m_connectionState == Konversation::SSConnected)
            emit serverOnline(true);
        else if (m_connectionState != Konversation::SSConnecting)
            emit serverOnline(false);

        emit connectionStateChanged(this, state);
    }
}

void Server::reconnect()
{
    if (!isConnecting())
    {
        if (isSocketConnected()) quitServer();

        // Use asynchronous invocation so that the broken() that the above
        // quitServer might cause is delivered before connectToIRCServer
        // sets SSConnecting and broken() announces a deliberate disconnect
        // due to the failure allegedly occuring during SSConnecting.
        QTimer::singleShot(0, this, SLOT(connectToIRCServer()));
    }
}

void Server::disconnect()
{
    if (isSocketConnected()) quitServer();
}

bool Server::isAway() const
{
    return m_isAway;
}

QString Server::awayTime() const
{
    QString retVal;

    if (m_isAway)
    {
        int diff = QDateTime::currentDateTime().toTime_t() - m_awayTime;
        int num = diff / 3600;

        if (num < 10)
            retVal = '0' + QString::number(num) + ':';
        else
            retVal = QString::number(num) + ':';

        num = (diff % 3600) / 60;

        if (num < 10) retVal += '0';

        retVal += QString::number(num) + ':';

        num = (diff % 3600) % 60;

        if (num < 10) retVal += '0';

        retVal += QString::number(num);
    }
    else
        retVal = "00:00:00";

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
    QStringList ql;
    ql << "PING LAG" + QTime::currentTime().toString("hhmmss");
    getInputFilter()->setAutomaticRequest("WHO", getNickname(), true);
    ql << "WHO " + getNickname();
    queueList(ql, HighPriority);

    m_lagTime.start();
    m_inputFilter.setLagMeasuring(true);
    m_pingResponseTimer.start(1000 /*1 sec*/);
}

void Server::pongReceived()
{
    m_currentLag = m_lagTime.elapsed();
    m_inputFilter.setLagMeasuring(false);
    m_pingResponseTimer.stop();

    emit serverLag(this, m_currentLag);

    // Send another PING in 60 seconds
    QTimer::singleShot(60000 /*60 sec*/, this, SLOT(sendPing()));
}

void Server::updateLongPongLag()
{
    if (isSocketConnected())
    {
        m_currentLag = m_lagTime.elapsed();
        emit tooLongLag(this, m_currentLag);
        // kdDebug() << "Current lag: " << currentLag << endl;

        if (m_currentLag > (Preferences::maximumLagTime() * 1000))
            m_socket->close();
    }
}

void Server::updateEncoding()
{
    if(getViewContainer() && getViewContainer()->getFrontView())
        getViewContainer()->updateViewEncoding(getViewContainer()->getFrontView());
}

#include "server.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
