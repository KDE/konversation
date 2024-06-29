// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2005-2016 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eli J. MacKenzie <argonel at gmail.com>
    SPDX-FileCopyrightText: 2005-2008 Eike Hein <hein@kde.org>
*/

#include "server.h"

#include "ircqueue.h"
#include "query.h"
#include "channel.h"
#include "application.h"
#include "connectionmanager.h"
#include "dcccommon.h"
#include "dccfiledialog.h"
#include "transfermanager.h"
#include "transfersend.h"
#include "transferrecv.h"
#include "chat.h"
#include "recipientdialog.h"
#include "nick.h"
#include "irccharsets.h"
#include "viewcontainer.h"
#include "rawlog.h"
#include "channellistpanel.h"
#include "scriptlauncher.h"
#include "serverison.h"
#include "notificationhandler.h"
#include "awaymanager.h"
#include "ircinput.h"
#include "konversation_log.h"

#include <KLocalizedString>
#include <KShell>
#include <KIO/SslUi>

#include <QTextCodec>
#include <QSslSocket>
#include <QStringListModel>
#include <QInputDialog>
#include <QNetworkProxy>
#include <QSslKey>

using namespace Konversation;

int Server::m_availableConnectionId = 0;

Server::Server(QObject* parent, ConnectionSettings& settings) : QObject(parent)
{
    m_connectionId = m_availableConnectionId;
    m_availableConnectionId++;

    setConnectionSettings(settings);

    m_connectionState = Konversation::SSNeverConnected;

    m_recreationScheduled = false;

    m_delayedConnectTimer = new QTimer(this);
    m_delayedConnectTimer->setSingleShot(true);
    connect(m_delayedConnectTimer, &QTimer::timeout, this, &Server::connectToIRCServer);

    m_reconnectImmediately = false;

    for (int i=0; i <= Application::instance()->countOfQueues(); i++)
    {
        //QList<int> r=Preferences::queueRate(i);
        auto *q=new IRCQueue(this, Application::instance()->staticrates[i]); //FIXME these are supposed to be in the rc
        m_queues.append(q);
    }

    m_processingIncoming = false;
    m_identifyMsg = false;
    m_capRequested = 0;
    m_capAnswered = 0;
    m_capEndDelayed = false;
    m_autoJoin = false;

    m_capabilities = NoCapabilies;
    m_whoRequestsDisabled = false;
    initCapablityNames();

    m_nickIndices.clear();
    m_nickIndices.append(0);

    m_nickListModel = new QStringListModel(this);

    m_currentLag = -1;
    m_rawLog = nullptr;
    m_channelListPanel = nullptr;
    m_serverISON = nullptr;
    m_away = false;
    m_socket = nullptr;
    m_prevISONList = QStringList();
    m_bytesReceived = 0;
    m_encodedBytesSent=0;
    m_bytesSent=0;
    m_linesSent=0;
    // TODO fold these into a QMAP, and these need to be reset to RFC values if this server object is reused.
    m_serverNickPrefixModes = QStringLiteral("ovh");
    m_serverNickPrefixes = QStringLiteral("@+%");
    m_banAddressListModes = QLatin1Char('b'); // {RFC-1459, draft-brocklesby-irc-isupport} -> pick one
    m_channelPrefixes = QStringLiteral("#&");
    m_modesCount = 3;
    m_sslErrorLock = false;
    m_topicLength = -1;

    setObjectName(QLatin1String("server_") + m_connectionSettings.name());

    setNickname(m_connectionSettings.initialNick());
    obtainNickInfo(getNickname());

    m_statusView = getViewContainer()->addStatusView(this);

    if (Preferences::self()->rawLog())
        addRawLog(false);

    m_inputFilter.setServer(this);
    m_outputFilter = new Konversation::OutputFilter(this);

    // For /msg query completion
    m_completeQueryPosition = 0;

    updateAutoJoin(m_connectionSettings.oneShotChannelList());

    if (!getIdentity()->getShellCommand().isEmpty())
        QTimer::singleShot(0, this, &Server::doPreShellCommand);
    else
        QTimer::singleShot(0, this, &Server::connectToIRCServer);

    initTimers();

    if (getIdentity()->getShellCommand().isEmpty())
        connectSignals();
    // TODO FIXME this disappeared in a merge, ensure it should have
    updateConnectionState(Konversation::SSNeverConnected);

    m_nickInfoChangedTimer = new QTimer(this);
    m_nickInfoChangedTimer->setSingleShot(true);
    m_nickInfoChangedTimer->setInterval(3000);
    connect(m_nickInfoChangedTimer, &QTimer::timeout, this, &Server::sendNickInfoChangedSignals);

    m_channelNickChangedTimer = new QTimer(this);
    m_channelNickChangedTimer->setSingleShot(true);
    m_channelNickChangedTimer->setInterval(1000);
    connect(m_channelNickChangedTimer, &QTimer::timeout, this, &Server::sendChannelNickChangedSignals);
}

Server::~Server()
{
    //send queued messages
    qCDebug(KONVERSATION_LOG) << "Server::~Server(" << getServerName() << ")";

    // Delete helper object.
    delete m_serverISON;
    m_serverISON = nullptr;

    // clear nicks online
    Q_EMIT nicksNowOnline(this,QStringList(),true);

    // Make sure no signals get sent to a soon to be dying Server Window
    if (m_socket)
    {
        m_socket->blockSignals(true);
        m_socket->deleteLater();
    }

    delete m_statusView;

    closeRawLog();
    closeChannelListPanel();

    if (m_recreationScheduled)
    {
        Konversation::ChannelList channelList;

        channelList.reserve(m_channelList.size());
        for (Channel* channel : std::as_const(m_channelList)) {
            channelList << channel->channelSettings();
        }

        m_connectionSettings.setOneShotChannelList(channelList);
    }

    qDeleteAll(m_channelList);
    m_channelList.clear();
    m_loweredChannelNameHash.clear();

    qDeleteAll(m_queryList);
    m_queryList.clear();

    purgeData();

    //Delete the queues
    qDeleteAll(m_queues);

    Q_EMIT destroyed(m_connectionId);

    if (m_recreationScheduled)
    {
        qRegisterMetaType<ConnectionSettings>("ConnectionSettings");
        qRegisterMetaType<Konversation::ConnectionFlag>("Konversation::ConnectionFlag");

        Application* konvApp = Application::instance();

        QMetaObject::invokeMethod(konvApp->getConnectionManager(), "connectTo", Qt::QueuedConnection,
            Q_ARG(Konversation::ConnectionFlag, Konversation::CreateNewConnection),
            Q_ARG(ConnectionSettings, m_connectionSettings));
    }

    qCDebug(KONVERSATION_LOG) << "~Server done";
}

void Server::purgeData()
{
    // Delete all the NickInfos and ChannelNick structures.
    m_allNicks.clear();

    qDeleteAll(m_joinedChannels);
    m_joinedChannels.clear();

    qDeleteAll(m_unjoinedChannels);
    m_unjoinedChannels.clear();

    m_queryNicks.clear();
    delete m_serverISON;
    m_serverISON = nullptr;

}

//... so called to match the ChatWindow derivatives.
bool Server::closeYourself(bool askForConfirmation)
{
    m_statusView->closeYourself(askForConfirmation);

    return true;
}

void Server::cycle()
{
    m_recreationScheduled = true;

    m_statusView->closeYourself();
}

void Server::doPreShellCommand()
{
    KShell::Errors e;
    QStringList command = KShell::splitArgs(getIdentity()->getShellCommand(), KShell::TildeExpand, &e);
    if (e != KShell::NoError)
    {
        //FIXME The flow needs to be refactored, add a finally-like method that does the ready-to-connect stuff
        // "The pre-connect shell command could not be understood!");
        preShellCommandExited(m_preShellCommand.exitCode(), m_preShellCommand.exitStatus());
    }
    else
    {
        // FIXME add i18n, and in preShellCommandExited and preShellCommandError
        getStatusView()->appendServerMessage(i18n("Info"), i18nc("The command mentioned is executed in a shell prior to connecting.", "Running pre-connect shell command..."));

        connect(&m_preShellCommand, QOverload<int,QProcess::ExitStatus>::of(&KProcess::finished),
                this, &Server::preShellCommandExited);
        connect(&m_preShellCommand, &KProcess::errorOccurred, this, &Server::preShellCommandError);

        m_preShellCommand.setProgram(command);
        m_preShellCommand.start();
        // NOTE: isConnecting is tested in connectToIRCServer so there's no guard here
        if (m_preShellCommand.state() == QProcess::NotRunning)
            preShellCommandExited(m_preShellCommand.exitCode(), m_preShellCommand.exitStatus());
    }
}

void Server::initTimers()
{
    m_notifyTimer.setObjectName(QStringLiteral("notify_timer"));
    m_notifyTimer.setSingleShot(true);
    m_incomingTimer.setObjectName(QStringLiteral("incoming_timer"));
    m_pingSendTimer.setSingleShot(true);
}

void Server::connectSignals()
{
    // Timers
    connect(&m_incomingTimer, &QTimer::timeout, this, &Server::processIncomingData);
    connect(&m_notifyTimer, &QTimer::timeout, this, &Server::notifyTimeout);
    connect(&m_pingResponseTimer, &QTimer::timeout, this, &Server::updateLongPongLag);
    connect(&m_pingSendTimer, &QTimer::timeout, this, &Server::sendPing);

    // OutputFilter
    connect(getOutputFilter(), QOverload<>::of(&OutputFilter::requestDccSend),
            this, QOverload<>::of(&Server::requestDccSend), Qt::QueuedConnection);
    connect(getOutputFilter(),QOverload<const QString&>::of(&OutputFilter::requestDccSend),
            this, QOverload<const QString&>::of(&Server::requestDccSend), Qt::QueuedConnection);
    connect(getOutputFilter(), &OutputFilter::multiServerCommand,
        this, &Server::sendMultiServerCommand);
    connect(getOutputFilter(), &OutputFilter::reconnectServer, this, &Server::reconnectServer);
    connect(getOutputFilter(), &OutputFilter::disconnectServer, this, &Server::disconnectServer);
    connect(getOutputFilter(), &OutputFilter::quitServer, this, &Server::quitServer);
    connect(getOutputFilter(), &OutputFilter::openDccSend,
            this, [this](const QString& recipient, const QUrl& url) { addDccSend(recipient, url); }, Qt::QueuedConnection);
    connect(getOutputFilter(), &OutputFilter::openDccChat, this, &Server::openDccChat, Qt::QueuedConnection);
    connect(getOutputFilter(), &OutputFilter::openDccWBoard, this, &Server::openDccWBoard, Qt::QueuedConnection);
    connect(getOutputFilter(), &OutputFilter::acceptDccGet,
        this, &Server::acceptDccGet);
    connect(getOutputFilter(), &OutputFilter::sendToAllChannels, this, &Server::sendToAllChannels);
    connect(getOutputFilter(), &OutputFilter::banUsers,
        this, &Server::requestBan);
    connect(getOutputFilter(), &OutputFilter::unbanUsers,
        this, &Server::requestUnban);
    connect(getOutputFilter(), &OutputFilter::openRawLog, this, &Server::addRawLog);
    connect(getOutputFilter(), &OutputFilter::closeRawLog, this, &Server::closeRawLog);
    connect(getOutputFilter(), &OutputFilter::encodingChanged, this, &Server::updateEncoding);

    Application* konvApp = Application::instance();
    connect(getOutputFilter(), QOverload<Konversation::ConnectionFlag, const QString& , const QString&, const QString&, const QString&, const QString&, bool>::of(&OutputFilter::connectTo),
         konvApp->getConnectionManager(), QOverload<Konversation::ConnectionFlag, const QString& , const QString&, const QString&, const QString&, const QString&, bool>::of(&ConnectionManager::connectTo));
    connect(konvApp->getDccTransferManager(), &DCC::TransferManager::newDccTransferQueued,
            this, &Server::slotNewDccTransferItemQueued);

   // ViewContainer
    connect(this, &Server::showView, getViewContainer(), &ViewContainer::showView);
    connect(this, &Server::addDccPanel, getViewContainer(), &ViewContainer::addDccPanel);
    connect(this, QOverload<Konversation::DCC::Chat*>::of(&Server::addDccChat),
            getViewContainer(), &ViewContainer::addDccChat, Qt::QueuedConnection);
    connect(this, &Server::serverLag, getViewContainer(), &ViewContainer::updateStatusBarLagLabel);
    connect(this, &Server::tooLongLag, getViewContainer(), &ViewContainer::setStatusBarLagLabelTooLongLag);
    connect(this, &Server::resetLag, getViewContainer(), &ViewContainer::resetStatusBarLagLabel);
    connect(getOutputFilter(), &OutputFilter::showView, getViewContainer(), &ViewContainer::showView);
    connect(getOutputFilter(), &OutputFilter::openKonsolePanel, getViewContainer(), &ViewContainer::addKonsolePanel);
    connect(getOutputFilter(), &OutputFilter::openChannelList, this, &Server::requestOpenChannelListPanel);
    connect(getOutputFilter(), &OutputFilter::closeDccPanel, getViewContainer(), &ViewContainer::closeDccPanel);
    connect(getOutputFilter(), &OutputFilter::addDccPanel, getViewContainer(), &ViewContainer::addDccPanel);

    // Inputfilter - queued connections should be used for slots that have blocking UI
    connect(&m_inputFilter, &InputFilter::addDccChat,
            this, QOverload<const QString&, const QStringList&>::of(&Server::addDccChat), Qt::QueuedConnection);
    connect(&m_inputFilter, &InputFilter::rejectDccChat,
            this, &Server::rejectDccChat);
    connect(&m_inputFilter, &InputFilter::startReverseDccChat,
            this, &Server::startReverseDccChat);
    connect(&m_inputFilter, &InputFilter::welcome, this, &Server::capCheckIgnored);
    connect(&m_inputFilter, &InputFilter::welcome, this, &Server::connectionEstablished);
    connect(&m_inputFilter, &InputFilter::notifyResponse, this, &Server::notifyResponse);
    connect(&m_inputFilter, &InputFilter::startReverseDccSendTransfer,
        this, &Server::startReverseDccSendTransfer);
    connect(&m_inputFilter, &InputFilter::addDccGet,
            this, &Server::addDccGet, Qt::QueuedConnection);
    connect(&m_inputFilter, &InputFilter::resumeDccGetTransfer,
        this, &Server::resumeDccGetTransfer);
    connect(&m_inputFilter, &InputFilter::resumeDccSendTransfer,
        this, &Server::resumeDccSendTransfer);
    connect(&m_inputFilter, &InputFilter::rejectDccSendTransfer,
        this, &Server::rejectDccSendTransfer);
    connect(&m_inputFilter, &InputFilter::userhost,
        this, &Server::userhost );
    connect(&m_inputFilter, &InputFilter::topicAuthor,
        this, &Server::setTopicAuthor );
    connect(&m_inputFilter, &InputFilter::endOfWho,
        this, &Server::endOfWho );
    connect(&m_inputFilter, &InputFilter::endOfNames,
        this, &Server::endOfNames );
    connect(&m_inputFilter, &InputFilter::invitation,
        this,&Server::invitation );
    connect(&m_inputFilter, &InputFilter::addToChannelList,
        this, &Server::addToChannelList);

    // Status View
    connect(this, &Server::serverOnline, getStatusView(), &StatusPanel::serverOnline);

        // Scripts
    connect(getOutputFilter(), &OutputFilter::launchScript,
        konvApp->getScriptLauncher(), &ScriptLauncher::launchScript);
    connect(konvApp->getScriptLauncher(), &ScriptLauncher::scriptNotFound,
        this, &Server::scriptNotFound);
    connect(konvApp->getScriptLauncher(), &ScriptLauncher::scriptExecutionError,
        this, &Server::scriptExecutionError);

    connect(Preferences::self(), &Preferences::notifyListStarted,
        this, &Server::notifyListStarted, Qt::QueuedConnection);
}

int Server::getPort() const
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

void Server::preShellCommandExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)

    if (exitStatus == QProcess::NormalExit)
        getStatusView()->appendServerMessage(i18n("Info"), i18n("Pre-shell command executed successfully!"));
    else
    {
        QString errorText = i18nc("An error message from KDE or Qt is appended.", "There was a problem while executing the command: ") % m_preShellCommand.errorString();
        getStatusView()->appendServerMessage(i18n("Warning"), errorText);
    }

    connectToIRCServer();
    connectSignals();
}

void Server::preShellCommandError(QProcess::ProcessError error)
{
    Q_UNUSED(error)

    QString errorText = i18nc("An error message from KDE or Qt is appended.", "There was a problem while executing the command: ") % m_preShellCommand.errorString();
    getStatusView()->appendServerMessage(i18n("Warning"), errorText);

    connectToIRCServer();
    connectSignals();
}

void Server::connectToIRCServer()
{
    if (!isConnected() && !isConnecting())
    {
        if (m_sslErrorLock)
        {
            qCDebug(KONVERSATION_LOG) << "Refusing to connect while SSL lock from previous connection attempt is being held.";

            return;
        }

        // Reenable check when it works reliably for all backends
//         if(Solid::Networking::status() != Solid::Networking::Connected)
//         {
//             updateConnectionState(Konversation::SSInvoluntarilyDisconnected);
//             return;
//         }

        updateConnectionState(Konversation::SSConnecting);

        m_ownIpByUserhost.clear();

        resetQueues();

        // This is needed to support server groups with mixed SSL and nonSSL servers
        delete m_socket;
        m_socket = nullptr;
        if (m_referenceNicklist != getIdentity()->getNicknameList())
            m_nickListModel->setStringList(getIdentity()->getNicknameList());
        resetNickSelection();

        m_socket = new QSslSocket();
        m_socket->setObjectName(QStringLiteral("serverSocket"));

        connect(m_socket, &QAbstractSocket::errorOccurred,
                this, &Server::broken);

        connect(m_socket, &QIODevice::readyRead, this, &Server::incoming);
        connect(m_socket, &QAbstractSocket::disconnected, this, &Server::closed);
        connect(m_socket, &QAbstractSocket::hostFound, this, &Server::hostFound);

        getStatusView()->appendServerMessage(i18n("Info"),i18n("Looking for server %1 (port %2)...",
            getConnectionSettings().server().host(),
            QString::number(getConnectionSettings().server().port())));

        if(getConnectionSettings().server().bypassProxy()) {
            m_socket->setProxy(QNetworkProxy::NoProxy);
        }

        // connect() will do a async lookup too
        if (getConnectionSettings().server().SSLEnabled()
            || getIdentity()->getAuthType() == QLatin1String("saslexternal")
            || getIdentity()->getAuthType() == QLatin1String("pemclientcert"))
        {
            connect(m_socket, &QSslSocket::encrypted, this, &Server::socketConnected);
            connect(m_socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
                    this, &Server::sslError);

            if (getIdentity()->getAuthType() == QLatin1String("saslexternal")
                || getIdentity()->getAuthType() == QLatin1String("pemclientcert"))
            {
                m_socket->setLocalCertificate(getIdentity()->getPemClientCertFile().toLocalFile());
                m_socket->setPrivateKey(getIdentity()->getPemClientCertFile().toLocalFile());

                if (m_socket->privateKey().isNull())
                {
                    m_socket->setPrivateKey(getIdentity()->getPemClientCertFile().toLocalFile(), QSsl::Ec);
                }
            }

            m_socket->setProtocol(QSsl::SecureProtocols);
            // QIODevice::Unbuffered, see m_socket->connectToHost() call below
            m_socket->connectToHostEncrypted(getConnectionSettings().server().host(),
                                             getConnectionSettings().server().port(),
                                             (QIODevice::ReadWrite | QIODevice::Unbuffered));
        }
        else
        {
            connect(m_socket, &QAbstractSocket::connected, this, &Server::socketConnected);
            // From KTcpSocket::connectToHost():
            // There are enough layers of buffers between us and the network, and there is a quirk
            // in QIODevice that can make it try to readData() twice per read() call if buffered and
            // reaData() does not deliver enough data the first time. Like when the other side is
            // simply not sending any more data...
            // This can *apparently* lead to long delays sometimes which stalls applications.
            // Do not want.
            m_socket->connectToHost(getConnectionSettings().server().host(),
                                    getConnectionSettings().server().port(),
                                    (QIODevice::ReadWrite | QIODevice::Unbuffered));
        }

        // set up the connection details
        setPrefixes(m_serverNickPrefixModes, m_serverNickPrefixes);
        // reset InputFilter (auto request info, /WHO request info)
        m_inputFilter.reset();
    }
    else
        qCDebug(KONVERSATION_LOG) << "connectToIRCServer() called while already connected: This should never happen. (" << (isConnecting() << 1) + isConnected() << ')';
}

void Server::connectToIRCServerIn(uint delay)
{
    m_delayedConnectTimer->setInterval(delay * 1000);
    m_delayedConnectTimer->start();

    updateConnectionState(Konversation::SSScheduledToConnect);
}

void Server::showSSLDialog()
{
        //TODO
        /*
          SSLSocket* sslsocket = dynamic_cast<SSLSocket*>(m_socket);

          if (sslsocket) sslsocket->showInfoDialog();
        */
}

void Server::rebuildTargetPrefixMatcher()
{
    m_targetMatcher.setPattern(QLatin1String("^([") + getServerNickPrefixes() + QLatin1String("]*)([") + getChannelTypes() + QLatin1String("])(.*)"));
}

// set available channel types according to 005 RPL_ISUPPORT
void Server::setChannelTypes(const QString &pre)
{
    m_channelPrefixes = pre;
    rebuildTargetPrefixMatcher();

    if (getConnectionSettings().reconnectCount() == 0) {
        updateAutoJoin(m_connectionSettings.oneShotChannelList());
    } else {
        updateAutoJoin();
    }
}

QString Server::getChannelTypes() const
{
    return m_channelPrefixes;
}

// set max number of channel modes with parameter according to 005 RPL_ISUPPORT
void Server::setModesCount(int count)
{
    m_modesCount = count;
}

int Server::getModesCount() const
{
    return m_modesCount;
}

// set user mode prefixes according to non-standard 005-Reply (see inputfilter.cpp)
void Server::setPrefixes(const QString &modes, const QString& prefixes)
{
    // NOTE: serverModes is QString(), if server did not supply the
    // modes which relates to the network's nick-prefixes
    m_serverNickPrefixModes = modes;
    m_serverNickPrefixes = prefixes;
    rebuildTargetPrefixMatcher();
}

QString Server::getServerNickPrefixes() const
{
    return m_serverNickPrefixes;
}

void Server::setChanModes(const QString& modes)
{
    QStringList abcd = modes.split(QLatin1Char(','));
    m_banAddressListModes = abcd.value(0);
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

    while ((modeIndex = m_serverNickPrefixes.indexOf(nickname[0])) != -1)
    {
        if(nickname.isEmpty())
            return;
        nickname.remove(0, 1);
        // cut off the prefix
        bool recognisedMode = false;
        // determine, whether status is like op or like voice
        while(modeIndex < m_serverNickPrefixes.length() && !recognisedMode)
        {
            switch(m_serverNickPrefixes[modeIndex].toLatin1())
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

void Server::hostFound()
{
    getStatusView()->appendServerMessage(i18n("Info"),i18n("Server found, connecting..."));
}

void Server::socketConnected()
{
    Q_EMIT sslConnected(this);
    getConnectionSettings().setReconnectCount(0);

    requestAvailableCapabilies();

    QStringList ql;

    if (getIdentity() && getIdentity()->getAuthType() == QLatin1String("serverpw")
        && !getIdentity()->getAuthPassword().isEmpty())
    {
        ql << QStringLiteral("PASS :") + getIdentity()->getAuthPassword();
    }
    else if (!getConnectionSettings().server().password().isEmpty())
        ql << QStringLiteral("PASS :") + getConnectionSettings().server().password();

    ql << QStringLiteral("NICK ") + getNickname();
    ql << QStringLiteral("USER ") + getIdentity()->getIdent() + QStringLiteral(" 8 * :") /* 8 = +i; 4 = +w */ +  getIdentity()->getRealName();

    queueList(ql, HighPriority);

    connect(this, &Server::nicknameChanged, getStatusView(), &StatusPanel::setNickname);
    setNickname(getNickname());
}

void Server::requestAvailableCapabilies ()
{
    m_capRequested = 0;
    m_capAnswered = 0;
    m_capEndDelayed = false;
    m_capabilities = NoCapabilies;
    getStatusView()->appendServerMessage(i18n("Info"),i18n("Negotiating capabilities with server..."));
    m_inputFilter.setAutomaticRequest(QStringLiteral("CAP LS"), QString(), true);
    queue(QStringLiteral("CAP LS 302"), HighPriority);
}

void Server::capInitiateNegotiation(const QString &availableCaps)
{
    QStringList requestCaps;
    const QStringList capsList = availableCaps.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList nameValue;

    for (const QString &cap : capsList) {
        nameValue = cap.split(QLatin1Char('='));

        if (nameValue.isEmpty())
            continue;

        if(nameValue.at(0) == QLatin1String("sasl"))
        {
            QString authCommand;

            if (getIdentity()) {
                // A username is optional SASL EXTERNAL and a client cert substitutes
                // for the password.
                if (getIdentity()->getAuthType() == QLatin1String("saslexternal")) {
                    authCommand = QStringLiteral("EXTERNAL");
                // PLAIN on the other hand requires both.
                } else if (getIdentity()->getAuthType() == QLatin1String("saslplain")
                    && !getIdentity()->getSaslAccount().isEmpty()
                    && !getIdentity()->getAuthPassword().isEmpty()) {
                    authCommand = QStringLiteral("PLAIN");
                }
            }

            if(!authCommand.isEmpty())
            {
                QStringList supportedSaslTypes;
                if(nameValue.size() > 1)
                    supportedSaslTypes = nameValue.at(1).split(QLatin1Char(','));

                supportedSaslTypes.removeDuplicates();

                if(!supportedSaslTypes.isEmpty() && !supportedSaslTypes.contains(authCommand))
                    getStatusView()->appendServerMessage(i18n("Error"), i18n("Server does not support %1 as SASL authentication mechanism, skipping SASL authentication.", authCommand));
                else
                    requestCaps.append (QStringLiteral("sasl"));
            }
        }
        else if(m_capabilityNames.contains(nameValue.at(0)))
        {
            requestCaps.append (nameValue.at(0));
        }

        // HACK: twitch.tv's IRC server doesn't handle WHO so
        // let's disable all WHO requests for servers that has
        // twitch.tv capabilities
        if (nameValue.at(0).startsWith(QLatin1String("twitch.tv"))) {
            m_whoRequestsDisabled = true;
        }
    }

    if(!requestCaps.isEmpty())
    {
        QString capsString = requestCaps.join(QLatin1Char(' '));
        getStatusView()->appendServerMessage(i18n("Info"),i18n("Requesting capabilities: %1", capsString));
        queue(QStringLiteral("CAP REQ :") + capsString, HighPriority);
        m_capRequested++;
    } else {
        capEndNegotiation();
    }
}

void Server::capReply()
{
    m_capAnswered++;
}

void Server::capEndNegotiation()
{
    if(m_capRequested == m_capAnswered)
    {
        getStatusView()->appendServerMessage(i18n("Info"),i18n("Closing capabilities negotiation."));
        queue(QStringLiteral("CAP END"), HighPriority);
    }
}

void Server::capCheckIgnored()
{
    if (m_capRequested && !m_capAnswered)
        getStatusView()->appendServerMessage(i18n("Error"), i18n("Capabilities negotiation failed: Appears not supported by server."));
}

void Server::capAcknowledged(const QString& name, Server::CapModifiers modifiers)
{
    if (name == QLatin1String("sasl") && modifiers == Server::NoModifiers)
    {
        const QString &authCommand = (getIdentity()->getAuthType() == QLatin1String("saslexternal")) ?
            QStringLiteral("EXTERNAL") : QStringLiteral("PLAIN");

        getStatusView()->appendServerMessage(i18n("Info"), i18n("SASL capability acknowledged by server, attempting SASL %1 authentication...", authCommand));

        sendAuthenticate(authCommand);

        m_capEndDelayed = true;
    }

    m_capabilities |= m_capabilityNames.value(name);
}

void Server::capDenied(const QString& name)
{
    if (name == QLatin1String("sasl"))
        getStatusView()->appendServerMessage(i18n("Error"), i18n("SASL capability denied or not supported by server."));
}

void Server::capDel(const QString &unavailableCaps)
{
    const QStringList capsList = unavailableCaps.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    for (const QString &capString : std::as_const(capsList))
    {
        if (m_capabilityNames.contains(capString))
            m_capabilities &= ~(m_capabilityNames.value(capString));
    }
}

void Server::registerWithServices()
{
    if (!getIdentity())
        return;

    NickInfoPtr nickInfo = getNickInfo(getNickname());
    if (nickInfo && nickInfo->isIdentified())
        return;

    if (getIdentity()->getAuthType() == QLatin1String("nickserv"))
    {
        if (!getIdentity()->getNickservNickname().isEmpty()
            && !getIdentity()->getNickservCommand().isEmpty()
            && !getIdentity()->getAuthPassword().isEmpty())
        {
            queue(QStringLiteral("PRIVMSG ")+getIdentity()->getNickservNickname()+QStringLiteral(" :")+getIdentity()->getNickservCommand()+QLatin1Char(' ')+getIdentity()->getAuthPassword(), HighPriority);
        }
    }
    else if (getIdentity()->getAuthType() == QLatin1String("saslplain"))
    {
        QString authString = getIdentity()->getSaslAccount();
        authString.append(QChar(QChar::Null));
        authString.append(getIdentity()->getSaslAccount());
        authString.append(QChar(QChar::Null));
        authString.append(getIdentity()->getAuthPassword());

        sendAuthenticate(QLatin1String(authString.toLatin1().toBase64()));
    }
    else if (getIdentity()->getAuthType() == QLatin1String("saslexternal"))
    {
        QString authString = getIdentity()->getSaslAccount();

        // An account name is optional with SASL EXTERNAL.
        if (!authString.isEmpty()) {
            authString.append(QChar(QChar::Null));
            authString.append(getIdentity()->getSaslAccount());
        }

        sendAuthenticate(QLatin1String(authString.toLatin1().toBase64()));
    }
}

void Server::sendAuthenticate(const QString& message)
{
    m_lastAuthenticateCommand = message;

    if (message.isEmpty()) {
        queue(QStringLiteral("AUTHENTICATE +"), HighPriority);
    } else {
        queue(QStringLiteral("AUTHENTICATE ") + message, HighPriority);
    }
}

void Server::broken(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)

    qCDebug(KONVERSATION_LOG) << "Connection broken with state" << m_connectionState << "and error:" << m_socket->errorString();

    m_socket->blockSignals(true);

    resetQueues();

    m_notifyTimer.stop();
    m_pingSendTimer.stop();
    m_pingResponseTimer.stop();
    m_inputFilter.setLagMeasuring(false);
    m_currentLag = -1;

    purgeData();

    Q_EMIT resetLag(this);
    Q_EMIT nicksNowOnline(this, QStringList(), true);
    m_prevISONList.clear();

    updateAutoJoin();


    if (m_sslErrorLock)
    {
        // We got disconnected while dealing with an SSL error, e.g. due to the
        // user taking their time on dealing with the error dialog. Since auto-
        // reconnecting makes no sense in this situation, let's pass it off as a
        // deliberate disconnect. sslError() will either kick off a reconnect, or
        // reaffirm this.

        getStatusView()->appendServerMessage(i18n("SSL Connection Error"),
            i18n("Connection to server %1 (port %2) lost while waiting for user response to an SSL error. "
                 "Will automatically reconnect if error is ignored.",
                 getConnectionSettings().server().host(),
                 QString::number(getConnectionSettings().server().port())));

        updateConnectionState(SSDeliberatelyDisconnected);
    }
    else if (getConnectionState() == Konversation::SSDeliberatelyDisconnected)
    {
        if (m_reconnectImmediately)
        {
            m_reconnectImmediately = false;

            QMetaObject::invokeMethod(this, "connectToIRCServer", Qt::QueuedConnection);
        }
    }
    else
    {
        Application::instance()->notificationHandler()->connectionFailure(getStatusView(), getServerName());

        QString error = i18n("Connection to server %1 (port %2) lost: %3.",
            getConnectionSettings().server().host(),
            QString::number(getConnectionSettings().server().port()),
            m_socket->errorString());

        getStatusView()->appendServerMessage(i18n("Error"), error);

        updateConnectionState(Konversation::SSInvoluntarilyDisconnected);
    }

    // HACK Only show one nick change dialog at connection time.
    // This hack is a bit nasty as it assumes that the only QInputDialog
    // child of the statusview will be the nick change dialog.
    if (getStatusView())
    {
        auto* nickChangeDialog = getStatusView()->findChild<QInputDialog*>();

        if (nickChangeDialog) nickChangeDialog->reject();
    }

}

void Server::sslError(const QList<QSslError> &errors)
{
    // We have to explicitly grab the socket we got the error from,
    // lest we might end up calling ignoreSslErrors() on a different
    // socket later if m_socket has started pointing at something
    // else.
    QPointer<QSslSocket> socket = qobject_cast<QSslSocket *>(QObject::sender());

    m_sslErrorLock = true;
    KSslErrorUiData uiData(socket);
    bool ignoreSslErrors = KIO::SslUi::askIgnoreSslErrors(uiData, KIO::SslUi::RecallAndStoreRules);
    m_sslErrorLock = false;

    // The dialog-based user interaction above may take an undefined amount
    // of time, and anything could happen to the socket in that span of time.
    // If it was destroyed, let's not do anything and bail out.
    if (!socket)
    {
        qCDebug(KONVERSATION_LOG) << "Socket was destroyed while waiting for user interaction.";

        return;
    }

    // Ask the user if he wants to ignore the errors.
    if (ignoreSslErrors)
    {
        // Show a warning in the chat window that the SSL certificate failed the authenticity check.
        QString error = i18n("The SSL certificate for the server %1 (port %2) failed the authenticity check.",
            getConnectionSettings().server().host(),
            QString::number(getConnectionSettings().server().port()));

        getStatusView()->appendServerMessage(i18n("SSL Connection Warning"), error);

        // We may have gotten disconnected while waiting for user response and have
        // to reconnect.
        if (isConnecting())
        {
            // The user has chosen to ignore SSL errors.
            socket->ignoreSslErrors();
        }
        else
        {
            // QueuedConnection is vital here, otherwise we're deleting the socket
            // in a slot connected to one of its signals (connectToIRCServer deletes
            // any old socket) and crash.
            QMetaObject::invokeMethod(this, "connectToIRCServer", Qt::QueuedConnection);
        }
    }
    else
    {
        // Don't auto-reconnect if the user chose to ignore the SSL errors --
        // treat it as a deliberate disconnect.
        updateConnectionState(Konversation::SSDeliberatelyDisconnected);

        QString errorReason;

        for (const QSslError& error : errors) {
            errorReason += error.errorString() + QLatin1Char(' ');
        }

        QString error = i18n("Could not connect to %1 (port %2) using SSL encryption. Either the server does not support SSL (did you use the correct port?) or you rejected the certificate. %3",
            getConnectionSettings().server().host(),
            QString::number(getConnectionSettings().server().port()),
            errorReason);

        getStatusView()->appendServerMessage(i18n("SSL Connection Error"), error);

        Q_EMIT sslInitFailure();
    }
}

// Will be called from InputFilter as soon as the Welcome message was received
void Server::connectionEstablished(const QString& ownHost)
{
    // Some servers don't include the userhost in RPL_WELCOME, so we
    // need to use RPL_USERHOST to get ahold of our IP later on
    if (!ownHost.isEmpty())
        QHostInfo::lookupHost(ownHost, this, &Server::gotOwnResolvedHostByWelcome);

    updateConnectionState(Konversation::SSConnected);

    // Make a helper object to build ISON (notify) list.
    // TODO: Give the object a kick to get it started?
    delete m_serverISON;
    m_serverISON = new ServerISON(this);

    // get first notify very early
    startNotifyTimer(1000);

    // Register with services
    if (getIdentity() && getIdentity()->getAuthType() == QLatin1String("nickserv"))
        registerWithServices();

    // get own ip by userhost
    requestUserhost(getNickname());

    // Start the PINGPONG match
    m_pingSendTimer.start(1000 /*1 sec*/);

    // Recreate away state if we were set away prior to a reconnect.
    if (m_away)
    {
        // Correct server's beliefs about its away state.
        m_away = false;
        requestAway(m_awayReason);
    }
}

//FIXME operator[] inserts an empty T& so each destination might just as well have its own key storage
QByteArray Server::getKeyForRecipient(const QString& recipient) const
{
    return m_keyHash[recipient.toLower()];
}

void Server::setKeyForRecipient(const QString& recipient, const QByteArray& key)
{
    m_keyHash[recipient.toLower()] = key;
}

void Server::gotOwnResolvedHostByWelcome(const QHostInfo& res)
{
    if (res.error() == QHostInfo::NoError && !res.addresses().isEmpty())
        m_ownIpByWelcome = res.addresses().first().toString();
    else
        qCDebug(KONVERSATION_LOG) << "Got error: " << res.errorString();
}

bool Server::isSocketConnected() const
{
    if (!m_socket) return false;

    return (m_socket->state() == QAbstractSocket::ConnectedState);
}

void Server::updateConnectionState(Konversation::ConnectionState state)
{
    if (state != m_connectionState)
    {
        m_connectionState = state;

        if (m_connectionState == Konversation::SSConnected)
            Q_EMIT serverOnline(true);
        else if (m_connectionState != Konversation::SSConnecting)
            Q_EMIT serverOnline(false);

       Q_EMIT connectionStateChanged(this, state);
    }
}

void Server::reconnectServer(const QString& quitMessage)
{
    if (isConnecting() || isSocketConnected())
    {
        m_reconnectImmediately = true;

        quitServer(quitMessage);
    }
    else
        QMetaObject::invokeMethod(this, "connectToIRCServer", Qt::QueuedConnection);
}

void Server::disconnectServer(const QString& quitMessage)
{
    getConnectionSettings().setReconnectCount(0);

    if (isScheduledToConnect())
    {
        m_delayedConnectTimer->stop();
        getStatusView()->appendServerMessage(i18n("Info"), i18n("Delayed connect aborted."));
    }

    if (isSocketConnected()) quitServer(quitMessage);
}

void Server::quitServer(const QString& quitMessage)
{
    // Make clear this is deliberate even if the QUIT never actually goes through the queue
    // (i.e. this is not redundant with _send_internal()'s updateConnectionState() call for
    // a QUIT).
    updateConnectionState(Konversation::SSDeliberatelyDisconnected);

    if (!m_socket) return;

    QString toServer = QStringLiteral("QUIT :");

    if (quitMessage.isEmpty())
        toServer += getIdentity()->getQuitReason();
    else
        toServer += quitMessage;

    queue(toServer, HighPriority);

    flushQueues();
    m_socket->flush();

    // Close the socket to allow a dead connection to be reconnected before the socket timeout.
    m_socket->close();

    getStatusView()->appendServerMessage(i18n("Info"), i18n("Disconnected from %1 (port %2).",
        getConnectionSettings().server().host(),
        QString::number(getConnectionSettings().server().port())));
}

void Server::notifyAction(const QString& nick)
{
    QString out(Preferences::self()->notifyDoubleClickAction());

    OutputFilter::replaceAliases(out);

    // parse wildcards (toParse,nickname,channelName,nickList,parameter)
    out = parseWildcards(out, getNickname(), QString(), QString(), nick, QString());

    // Send all strings, one after another
    const QStringList outList = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& out : outList) {
        Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(), out, QString());
        queue(result.toServer);
    }                                             // endfor
}

void Server::notifyResponse(const QString& nicksOnline)
{
    bool nicksOnlineChanged = false;
    const QStringList actualList = nicksOnline.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QString lcActual = QLatin1Char(' ') + nicksOnline + QLatin1Char(' ');
    QString lcPrevISON = QLatin1Char(' ') + (m_prevISONList.join(QLatin1Char(' '))) + QLatin1Char(' ');

    //Are any nicks gone offline
    for (const QString& nick : std::as_const(m_prevISONList)) {
        if (!lcActual.contains(QLatin1Char(' ') + nick + QLatin1Char(' '), Qt::CaseInsensitive)) {
            setNickOffline(nick);
            nicksOnlineChanged = true;
        }
    }

    //Are any nicks gone online
    for (const QString& nick : actualList) {
        if (!lcPrevISON.contains(QLatin1Char(' ') + nick + QLatin1Char(' '), Qt::CaseInsensitive)) {
            setWatchedNickOnline(nick);
            nicksOnlineChanged = true;
        }
    }

    // Note: The list emitted in this signal *does* include nicks in joined channels.
    Q_EMIT nicksNowOnline(this, actualList, nicksOnlineChanged);

    m_prevISONList = actualList;

    // Next round
    startNotifyTimer();
}

void Server::notifyListStarted(int serverGroupId)
{
    if (getServerGroup())
        if (getServerGroup()->id() == serverGroupId)
            startNotifyTimer(1000);
}

void Server::startNotifyTimer(int msec)
{
    // make sure the timer gets started properly in case we have reconnected
    m_notifyTimer.stop();

    if (Preferences::self()->useNotify())
    {
        if (msec == 0)
            msec = Preferences::self()->notifyDelay() * 1000;

        m_notifyTimer.start(msec);
    }
}

void Server::notifyTimeout()
{
    // Notify delay time is over, send ISON request if desired
    if (Preferences::self()->useNotify())
    {
        // But only if there actually are nicks in the notify list
        QString list = getISONListString();

        if (!list.isEmpty())
            queue(QStringLiteral("ISON ") + list, LowPriority);
    }
}

void Server::autoCommandsAndChannels()
{
    if (getServerGroup() && !getServerGroup()->connectCommands().isEmpty())
    {
        QString connectCommands = getServerGroup()->connectCommands();

        if (!getNickname().isEmpty())
            connectCommands.replace(QStringLiteral("%nick"), getNickname());

        const QStringList connectCommandsList = connectCommands.split(QLatin1Char(';'), Qt::SkipEmptyParts);

        for (QString output : connectCommandsList) {
            output = output.simplified();
            OutputFilter::replaceAliases(output);
            Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),output,QString());
            queue(result.toServer);
        }
    }

    if (getAutoJoin())
    {
        for (const QString& command : std::as_const(m_autoJoinCommands)) {
            queue(command);
        }
    }

    if (!m_connectionSettings.oneShotChannelList().isEmpty())
    {
        const QStringList oneShotJoin = generateJoinCommand(m_connectionSettings.oneShotChannelList());
        for (const QString& join : oneShotJoin) {
            queue(join);
        }
        m_connectionSettings.clearOneShotChannelList();
    }
}

/** Create a set of indices into the nickname list of the current identity based on the current nickname.
 *
 * The index list is only used if the current nickname is not available. If the nickname is in the identity,
 * we do not want to retry it. If the nickname is not in the identity, it is considered to be at position -1.
 */
void Server::resetNickSelection()
{
    m_nickIndices.clear();

    // For equivalence testing in case the identity gets changed underneath us.
    m_referenceNicklist = getIdentity()->getNicknameList();

    for (int i = 0; i < m_referenceNicklist.length(); ++i) {
        // Pointless to include the nick we're already going to use.
        if (m_referenceNicklist.at(i) != getNickname()) {
            m_nickIndices.append(i);
        }
    }
}

QString Server::getNextNickname()
{
     //if the identity changed underneath us (likely impossible), start over
    if (m_referenceNicklist != getIdentity()->getNicknameList())
        resetNickSelection();

    QString newNick;

    if (!m_nickIndices.isEmpty())
    {
        newNick = getIdentity()->getNickname(m_nickIndices.takeFirst());
    }
    else
    {
        QString inputText = i18n("No nicknames from the \"%1\" identity were accepted by the connection \"%2\".\nPlease enter a new one or press Cancel to disconnect:", getIdentity()->getName(), getDisplayName());
        newNick = QInputDialog::getText(getStatusView(), i18n("Nickname error"), inputText);
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
        m_inputFilter.parseLine(front);
        m_processingIncoming = false;

        if (!m_inputBuffer.isEmpty()) m_incomingTimer.start(0);
    }
}

void Server::incoming()
{
    //if (getConnectionSettings().server().SSLEnabled())
    //    Q_EMIT sslConnected(this);


    //if (len <= 0 && getConnectionSettings().server().SSLEnabled())
    //    return;

    // split buffer to lines
    QList<QByteArray> bufferLines;
    while (m_socket->bytesAvailable())
    {
        m_lineBuffer += m_socket->readLine();

        if(m_lineBuffer.endsWith('\n') || m_lineBuffer.endsWith('\r'))
        {
            //remove \n blowfish doesn't like it
            int i = m_lineBuffer.size()-1;
            while (i >= 0 && (m_lineBuffer[i]=='\n' || m_lineBuffer[i]=='\r')) // since euIRC gets away with sending just \r, bet someone sends \n\r?
            {
                i--;
            }
            m_lineBuffer.truncate(i+1);

            if (!m_lineBuffer.isEmpty())
                bufferLines.append(m_lineBuffer);

            m_lineBuffer.clear();
        }
    }

    while(!bufferLines.isEmpty())
    {
        // Pre parsing is needed in case encryption/decryption is needed
        // BEGIN set channel encoding if specified
        QString senderNick;
        bool isServerMessage = false;
        QString channelKey;
        QTextCodec* codec = getIdentity()->getCodec();
        QByteArray first = bufferLines.first();

        bufferLines.removeFirst();

        QStringList lineSplit = codec->toUnicode(first).split(QLatin1Char(' '), Qt::SkipEmptyParts);

        if (lineSplit.count() >= 1)
        {
            if (lineSplit[0][0] == QLatin1Char(':'))          // does this message have a prefix?
            {
                if(!lineSplit[0].contains(QLatin1Char('!'))) // is this a server(global) message?
                    isServerMessage = true;
                else
                    senderNick = lineSplit[0].mid(1, lineSplit[0].indexOf(QLatin1Char('!'))-1);

                lineSplit.removeFirst();          // remove prefix
            }
        }

        if (lineSplit.isEmpty())
            continue;

        // BEGIN pre-parse to know where the message belongs to
        QString command = lineSplit[0].toLower();
        if( isServerMessage )
        {
            if( lineSplit.count() >= 3 )
            {
                if( command == QLatin1String("332") )            // RPL_TOPIC
                    channelKey = lineSplit[2];
                if( command == QLatin1String("372") )            // RPL_MOTD
                    channelKey = QStringLiteral(":server");
            }
        }
        else                                      // NOT a global message
        {
            if( lineSplit.count() >= 2 )
            {
                // query
                if( ( command == QLatin1String("privmsg") ||
                    command == QLatin1String("notice")  ) &&
                    lineSplit[1] == getNickname() )
                {
                    channelKey = senderNick;
                }
                // channel message
                else if( command == QLatin1String("privmsg") ||
                    command == QLatin1String("notice")  ||
                    command == QLatin1String("join")    ||
                    command == QLatin1String("kick")    ||
                    command == QLatin1String("part")    ||
                    command == QLatin1String("topic")   )
                {
                    channelKey = lineSplit[1];
                }
            }
        }
        // END pre-parse to know where the message belongs to
        // Decrypt if necessary

        //send to raw log before decryption
        if (m_rawLog)
            m_rawLog->appendRaw(RawLog::Inbound, first);

        #if HAVE_QCA2
        QByteArray cKey = getKeyForRecipient(channelKey);
        if(!cKey.isEmpty())
        {
            if(command == QLatin1String("privmsg"))
            {
                //only send encrypted text to decrypter
                int index = first.indexOf(":",first.indexOf(":")+1);
                if(this->identifyMsgEnabled()) // Workaround braindead Freenode prefixing messages with +
                    ++index;
                QByteArray backup = first.mid(0,index+1);

                if(getChannelByName(channelKey) && getChannelByName(channelKey)->getCipher()->setKey(cKey))
                    first = getChannelByName(channelKey)->getCipher()->decrypt(first.mid(index+1));
                else if(getQueryByName(channelKey) && getQueryByName(channelKey)->getCipher()->setKey(cKey))
                    first = getQueryByName(channelKey)->getCipher()->decrypt(first.mid(index+1));

                first.prepend(backup);
            }
            else if(command == QLatin1String("332") || command == QLatin1String("topic"))
            {
                //only send encrypted text to decrypter
                int index = first.indexOf(":",first.indexOf(":")+1);
                QByteArray backup = first.mid(0,index+1);

                if(getChannelByName(channelKey) && getChannelByName(channelKey)->getCipher()->setKey(cKey))
                    first = getChannelByName(channelKey)->getCipher()->decryptTopic(first.mid(index+1));
                else if(getQueryByName(channelKey) && getQueryByName(channelKey)->getCipher()->setKey(cKey))
                    first = getQueryByName(channelKey)->getCipher()->decryptTopic(first.mid(index+1));

                first.prepend(backup);
            }
        }
        #endif
        bool isUtf8 = Konversation::isUtf8(first);

        QString encoded;

        if (isUtf8)
            encoded = QString::fromUtf8(first);
        else
        {
            // check setting
            QString channelEncoding;
            if( !channelKey.isEmpty() )
            {
                if(getServerGroup())
                    channelEncoding = Preferences::channelEncoding(getServerGroup()->id(), channelKey);
                else
                    channelEncoding = Preferences::channelEncoding(getDisplayName(), channelKey);
            }
            // END set channel encoding if specified

            if( !channelEncoding.isEmpty() )
                codec = Konversation::IRCCharsets::self()->codecForName(channelEncoding);

            // if channel encoding is utf-8 and the string is definitely not utf-8
            // then try latin-1
            if (codec->mibEnum() == 106)
                codec = QTextCodec::codecForMib( 4 /* iso-8859-1 */ );

            encoded = codec->toUnicode(first);
        }

        // Qt uses 0xFDD0 and 0xFDD1 to mark the beginning and end of text frames. Remove
        // these here to avoid fatal errors encountered in QText* and the event loop pro-
        // cessing.
        sterilizeUnicode(encoded);

        if (!encoded.isEmpty())
            m_inputBuffer << encoded;

        //FIXME: This has nothing to do with bytes, and it's not raw received bytes either. Bogus number.
        //m_bytesReceived+=m_inputBuffer.back().length();
    }

    if( !m_incomingTimer.isActive() && !m_processingIncoming )
        m_incomingTimer.start(0);
}

/** Calculate how long this message premable will be.

    This is necessary because the irc server will clip messages so that the
    client receives a maximum of 512 bytes at once.
*/
int Server::getPreLength(const QString& command, const QString& dest) const
{
    NickInfoPtr info = getNickInfo(getNickname());
    int hostMaskLength = 0;

    if(info)
        hostMaskLength = info->getHostmask().length();

    //:Sho_!i=ehs1@konversation/developer/hein PRIVMSG #konversation :and then back to it

    //<colon>$nickname<!>$hostmask<space>$command<space>$destination<space><colon>$message<cr><lf>
    int x= 512 - 8 - (m_nickname.length() + hostMaskLength + command.length() + dest.length());

    return x;
}

//Commands greater than 1 have localizeable text
static QStringList outcmds = QStringList {
    QStringLiteral("WHO"),     // 0
    QStringLiteral("QUIT"),    // 1
    QStringLiteral("PRIVMSG"), // 2
    QStringLiteral("NOTICE"),  // 3
    QStringLiteral("KICK"),    // 4
    QStringLiteral("PART"),    // 5
    QStringLiteral("TOPIC"),   // 6
};

int Server::_send_internal(QString outputLine)
{
    QStringList outputLineSplit = outputLine.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    int outboundCommand = -1;
    if (!outputLineSplit.isEmpty()) {
        //Lets cache the uppercase command so we don't miss or reiterate too much
        outboundCommand = outcmds.indexOf(outputLineSplit[0].toUpper());
    }

    if (outputLine.at(outputLine.length()-1) == QLatin1Char('\n'))
    {
        qCDebug(KONVERSATION_LOG) << "found \\n on " << outboundCommand;
        outputLine.resize(outputLine.length()-1);
    }

    // remember the first arg of /WHO to identify responses
    if (outboundCommand == 0 && outputLineSplit.count() >= 2) //"WHO"
        m_inputFilter.addWhoRequest(outputLineSplit[1]);
    else if (outboundCommand == 1) //"QUIT"
        updateConnectionState(Konversation::SSDeliberatelyDisconnected);

    // set channel encoding if specified
    QString channelCodecName;

    //[ PRIVMSG | NOTICE | KICK | PART | TOPIC ] target :message
    if (outputLineSplit.count() > 2 && outboundCommand > 1)
    {
        if(getServerGroup()) // if we're connecting via a servergroup
            channelCodecName=Preferences::channelEncoding(getServerGroup()->id(), outputLineSplit[1]);
        else //if we're connecting to a server manually
            channelCodecName=Preferences::channelEncoding(getDisplayName(), outputLineSplit[1]);
    }
    QTextCodec* codec = nullptr;
    if (channelCodecName.isEmpty())
        codec = getIdentity()->getCodec();
    else
        codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);

    // Some codecs don't work with a negative value. This is a bug in Qt 3.
    // ex.: JIS7, eucJP, SJIS
    //int outlen=-1;

    //leaving this done twice for now, I'm uncertain of the implications of not encoding other commands
    QByteArray encoded = outputLine.toUtf8();
    if(codec)
        encoded = codec->fromUnicode(outputLine);

    #if HAVE_QCA2
    QByteArray cipherKey;
    if (outputLineSplit.count() > 2 && outboundCommand > 1)
        cipherKey = getKeyForRecipient(outputLineSplit.at(1));
    if (!cipherKey.isEmpty())
    {
        int colon = outputLine.indexOf(QLatin1Char(':'));
        if (colon > -1)
        {
            colon++;

            QString pay(outputLine.mid(colon));
            //only encode the actual user text, IRCD *should* desire only ASCII 31 < x < 127 for protocol elements
            QByteArray payload = pay.toUtf8();

            QByteArray dest;
            if (codec)
            {
                payload=codec->fromUnicode(pay);
                //apparently channel name isn't a protocol element...
                dest = codec->fromUnicode(outputLineSplit.at(1));
            }
            else
            {
                dest = outputLineSplit.at(1).toLatin1();
            }

            if (outboundCommand == 2 || outboundCommand == 6) // outboundCommand == 3
            {
                bool doit = true;
                if (outboundCommand == 2)
                {
                    //if its a privmsg and a ctcp but not an action, don't encrypt
                    //not interpreting `payload` in case encoding bollixed it
                    if (outputLineSplit.at(2).startsWith(QLatin1String(":\x01"))
                        && outputLineSplit.at(2) != QLatin1String(":\x01""ACTION"))
                        doit = false;
                }
                if (doit)
                {
                    QString target = outputLineSplit.at(1);

                    if(getChannelByName(target) && getChannelByName(target)->getCipher()->setKey(cipherKey))
                        getChannelByName(target)->getCipher()->encrypt(payload);
                    else if(getQueryByName(target) && getQueryByName(target)->getCipher()->setKey(cipherKey))
                        getQueryByName(target)->getCipher()->encrypt(payload);

                    encoded = outputLineSplit.at(0).toLatin1();
                    qCDebug(KONVERSATION_LOG) << payload << "\n" << payload.data();
                    //two lines because the compiler insists on using the wrong operator+
                    encoded += ' ' + dest + " :" + payload;
                }
            }
        }
    }
    #endif

    if (m_rawLog)
        m_rawLog->appendRaw(RawLog::Outbound, encoded);

    encoded += '\n';
    qint64 sout = m_socket->write(encoded);

    return sout;
}

void Server::toServer(const QString& s, IRCQueue* q)
{

    int sizesent = _send_internal(s);
    Q_EMIT sentStat(s.length(), sizesent, q); //tell the queues what we sent
    collectStats(s.length(), sizesent);
}

void Server::collectStats(int bytes, int encodedBytes)
{
    m_bytesSent += bytes;
    m_encodedBytesSent += encodedBytes;
    m_linesSent++;
}

bool Server::validQueue(QueuePriority priority)
{
   if (priority >=0 && priority <= Application::instance()->countOfQueues())
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

    for (const QString& line : buffer) {
        if (!line.isEmpty())
            out.enqueue(line);
    }
    return true;
}

void Server::resetQueues()
{
    m_incomingTimer.stop();
    m_inputBuffer.clear();
    for (int i=0; i <= Application::instance()->countOfQueues(); i++)
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
        for (int i=1; i <= Application::instance()->countOfQueues(); i++) //slow queue can rot
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

void Server::dbusRaw(const QString& command)
{
    if(command.startsWith(Preferences::self()->commandChar()))
    {
        queue(command.section(Preferences::self()->commandChar(), 1));
    }
    else
        queue(command);
}

void Server::dbusSay(const QString& target,const QString& command)
{
    if(isAChannel(target))
    {
        Channel* channel=getChannelByName(target);
        if(channel) channel->sendText(command);
    }
    else
    {
        Query* query = getQueryByName(target);
        if(query==nullptr)
        {
            NickInfoPtr nickinfo = obtainNickInfo(target);
            query=addQuery(nickinfo, true);
        }
        if(query)
        {
            if(!command.isEmpty())
                query->sendText(command);
            else
            {
                query->adjustFocus();
                getViewContainer()->getWindow()->activateAndRaiseWindow();
            }
        }
    }
}

void Server::dbusInfo(const QString& string)
{
    appendMessageToFrontmost(i18n("D-Bus"),string);
}

void Server::ctcpReply(const QString &receiver,const QString &text)
{
    queue(QStringLiteral("NOTICE ")+receiver+QStringLiteral(" :")+QLatin1Char('\x01')+text+QLatin1Char('\x01'));
}

// Given a nickname, returns NickInfo object.   0 if not found.
NickInfoPtr Server::getNickInfo(const QString& nickname) const
{
    QString lcNickname(nickname.toLower());
    if (m_allNicks.contains(lcNickname))
    {
        NickInfoPtr nickinfo = m_allNicks[lcNickname];
        Q_ASSERT(nickinfo);
        return nickinfo;
    }
    else
        return NickInfoPtr(); //! TODO FIXME null null null
}

// Given a nickname, returns an existing NickInfo object, or creates a new NickInfo object.
// Returns pointer to the found or created NickInfo object.
NickInfoPtr Server::obtainNickInfo(const QString& nickname)
{
    NickInfoPtr nickInfo = getNickInfo(nickname);
    if (!nickInfo)
    {
        nickInfo = new NickInfo(nickname, this);
        m_allNicks.insert(QString(nickname.toLower()), nickInfo);
    }
    return nickInfo;
}

const NickInfoMap* Server::getAllNicks() const { return &m_allNicks; }

// Returns the list of members for a channel in the joinedChannels list.
// 0 if channel is not in the joinedChannels list.
// Using code must not alter the list.
const ChannelNickMap *Server::getJoinedChannelMembers(const QString& channelName) const
{
    QString lcChannelName = channelName.toLower();
    if (m_joinedChannels.contains(lcChannelName))
        return m_joinedChannels[lcChannelName];
    else
        return nullptr;
}

// Returns the list of members for a channel in the unjoinedChannels list.
// 0 if channel is not in the unjoinedChannels list.
// Using code must not alter the list.
const ChannelNickMap *Server::getUnjoinedChannelMembers(const QString& channelName) const
{
    QString lcChannelName = channelName.toLower();
    if (m_unjoinedChannels.contains(lcChannelName))
        return m_unjoinedChannels[lcChannelName];
    else
        return nullptr;
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
ChannelNickPtr Server::getChannelNick(const QString& channelName, const QString& nickname) const
{
    QString lcNickname = nickname.toLower();
    const ChannelNickMap *channelNickMap = getChannelMembers(channelName);
    if (channelNickMap)
    {
        if (channelNickMap->contains(lcNickname))
            return (*channelNickMap)[lcNickname];
        else
            return ChannelNickPtr(); //! TODO FIXME null null null
    }
    else
    {
        return ChannelNickPtr(); //! TODO FIXME null null null
    }
}

// Updates a nickname in a channel.  If not on the joined or unjoined lists, and nick
// is in the watch list, adds the channel and nick to the unjoinedChannels list.
// If mode != 99, sets the mode for the nick in the channel.
// Returns the NickInfo object if nick is on any lists, otherwise 0.
ChannelNickPtr Server::setChannelNick(const QString& channelName, const QString& nickname, unsigned int mode)
{
    QString lcNickname = nickname.toLower();
    // If already on a list, update mode.
    ChannelNickPtr channelNick = getChannelNick(channelName, lcNickname);
    if (!channelNick)
    {
        // If the nick is on the watch list, add channel and nick to unjoinedChannels list.
        if (getWatchList().contains(lcNickname, Qt::CaseInsensitive))
        {
            channelNick = addNickToUnjoinedChannelsList(channelName, nickname);
            channelNick->setMode(mode);
        }
        else return ChannelNickPtr(); //! TODO FIXME null null null
    }

    if (mode != 99) channelNick->setMode(mode);
    return channelNick;
}

// Returns a list of all the joined channels that a nick is in.
QStringList Server::getNickJoinedChannels(const QString& nickname) const
{
    QString lcNickname = nickname.toLower();
    QStringList channellist;
    ChannelMembershipMap::ConstIterator channel;
    for( channel = m_joinedChannels.constBegin(); channel != m_joinedChannels.constEnd(); ++channel )
    {
        if (channel.value()->contains(lcNickname)) channellist.append(channel.key());
    }
    return channellist;
}

// Returns a list of all the channels (joined or unjoined) that a nick is in.
QStringList Server::getNickChannels(const QString& nickname) const
{
    QString lcNickname = nickname.toLower();
    QStringList channellist;
    ChannelMembershipMap::ConstIterator channel;
    for( channel = m_joinedChannels.constBegin(); channel != m_joinedChannels.constEnd(); ++channel )
    {
        if (channel.value()->contains(lcNickname)) channellist.append(channel.key());
    }
    for( channel = m_unjoinedChannels.constBegin(); channel != m_unjoinedChannels.constEnd(); ++channel )
    {
        if (channel.value()->contains(lcNickname)) channellist.append(channel.key());
    }
    return channellist;
}

QStringList Server::getSharedChannels(const QString& nickname) const
{
    QString lcNickname = nickname.toLower();
    QStringList channellist;
    ChannelMembershipMap::ConstIterator channel;
    for( channel = m_joinedChannels.constBegin(); channel != m_joinedChannels.constEnd(); ++channel )
    {
        if (channel.value()->contains(lcNickname)) channellist.append(channel.key());
    }
    return channellist;
}

bool Server::isNickOnline(const QString &nickname) const
{
    NickInfoPtr nickInfo = getNickInfo(nickname);
    return (nickInfo != nullptr);
}

QString Server::getOwnIpByNetworkInterface() const
{
    return m_socket->localAddress().toString();
}

QString Server::getOwnIpByServerMessage() const
{
    if(!m_ownIpByWelcome.isEmpty())
        return m_ownIpByWelcome;
    else if(!m_ownIpByUserhost.isEmpty())
        return m_ownIpByUserhost;
    else
        return QString();
}

Query* Server::addQuery(const NickInfoPtr & nickInfo, bool weinitiated)
{
    QString nickname = nickInfo->getNickname();
    // Only create new query object if there isn't already one with the same name
    Query* query = getQueryByName(nickname);

    if (!query)
    {
        QString lcNickname = nickname.toLower();
        query = getViewContainer()->addQuery(this, nickInfo, weinitiated);

        query->indicateAway(m_away);

        connect(query, &Query::sendFile, this, QOverload<>::of(&Server::requestDccSend));
        connect(this, &Server::serverOnline, query, &Query::serverOnline);

        // Append query to internal list
        m_queryList.append(query);

        m_queryNicks.insert(lcNickname, nickInfo);

        if (!weinitiated)
            Application::instance()->notificationHandler()->query(query, nickname);
    }
    else if (weinitiated)
    {
        Q_EMIT showView(query);
    }

    // try to get hostmask if there's none yet
    if (query->getNickInfo()->getHostmask().isEmpty()) requestUserhost(nickname);

    Q_ASSERT(query);

    return query;
}

void Server::closeQuery(const QString &name)
{
    Query* query = getQueryByName(name);
    removeQuery(query);

    // Update NickInfo.  If no longer on any lists, delete it altogether, but
    // only if not on the watch list.  ISON replies will determine whether the NickInfo
    // is deleted altogether in that case.
    QString lcNickname = name.toLower();
    m_queryNicks.remove(lcNickname);
    if (!isWatchedNick(name)) deleteNickIfUnlisted(name);
}

void Server::closeChannel(const QString& name)
{
    qCDebug(KONVERSATION_LOG) << "Server::closeChannel(" << name << ")";
    Channel* channelToClose = getChannelByName(name);

    if (channelToClose && channelToClose->isJoined()) {
        Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),
            Preferences::self()->commandChar() + QStringLiteral("PART"), name);
        queue(result.toServer);
    }
}

void Server::requestChannelList()
{
    m_inputFilter.setAutomaticRequest(QStringLiteral("LIST"), QString(), true);
    queue(QStringLiteral("LIST"));
}

void Server::requestWhois(const QString& nickname)
{
    m_inputFilter.setAutomaticRequest(QStringLiteral("WHOIS"), nickname, true);
    queue(QStringLiteral("WHOIS ")+nickname, LowPriority);
}

void Server::requestWho(const QString& channel)
{
    if(m_whoRequestsDisabled)
        return;

    m_inputFilter.setAutomaticRequest(QStringLiteral("WHO"), channel, true);
    QString command(QStringLiteral("WHO ") + channel);

    if (capabilities() & WHOX && capabilities() & ExtendedJoin)
    {
        // Request the account as well as the usual info.
        // See http://faerion.sourceforge.net/doc/irc/whox.var
        // for more info.
        command += QStringLiteral(" nuhsa%cuhsnfdra");
    }

    queue(command, LowPriority);
}

void Server::requestUserhost(const QString& nicks)
{
    if(m_whoRequestsDisabled)
        return;

    const QStringList nicksList = nicks.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString& nick : nicksList)
        m_inputFilter.setAutomaticRequest(QStringLiteral("USERHOST"), nick, true);
    queue(QStringLiteral("USERHOST ")+nicks, LowPriority);
}

void Server::requestTopic(const QString& channel)
{
    m_inputFilter.setAutomaticRequest(QStringLiteral("TOPIC"), channel, true);
    queue(QStringLiteral("TOPIC ")+channel, LowPriority);
}

void Server::resolveUserhost(const QString& nickname)
{
    m_inputFilter.setAutomaticRequest(QStringLiteral("WHOIS"), nickname, true);
    m_inputFilter.setAutomaticRequest(QStringLiteral("DNS"), nickname, true);
    queue(QStringLiteral("WHOIS ")+nickname, LowPriority); //FIXME when is this really used?
}

void Server::requestBan(const QStringList& users,const QString& channel,const QString& a_option)
{
    QString option=a_option.toLower();

    Channel* targetChannel=getChannelByName(channel);

    for (QString mask : users) {
        // first, set the ban mask to the specified nick
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
                    mask=targetNick->getChannelNick()->getNickname()+QLatin1Char('!')+hostmask;

                    // adapt ban mask to the option given
                    if(option==QStringLiteral("host"))
                        mask=QLatin1String("*!*@*.")+hostmask.section(QLatin1Char('.'),1);
                    else if(option==QStringLiteral("domain"))
                        mask=QLatin1String("*!*@")+hostmask.section(QLatin1Char('@'),1);
                    else if(option==QStringLiteral("userhost"))
                        mask=QLatin1String("*!")+hostmask.section(QLatin1Char('@'),0,0)+QLatin1String("@*.")+hostmask.section(QLatin1Char('.'),1);
                    else if(option==QStringLiteral("userdomain"))
                        mask=QLatin1String("*!")+hostmask.section(QLatin1Char('@'),0,0)+QLatin1Char('@')+hostmask.section(QLatin1Char('@'),1);
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

void Server::sendURIs(const QList<QUrl>& uris, const QString& nick)
{
    for (const QUrl &uri : uris)
         addDccSend(nick, uri);
}

void Server::requestDccSend(const QString &a_recipient)
{
    QString recipient(a_recipient);
    // if we don't have a recipient yet, let the user select one
    if (recipient.isEmpty())
    {
        recipient = recipientNick();
    }

    // do we have a recipient *now*?
    if(!recipient.isEmpty())
    {
        QPointer<DccFileDialog> dlg = new DccFileDialog (getViewContainer()->getWindow());
        //DccFileDialog fileDialog(KUrl(), QString(), getViewContainer()->getWindow());
        const QList<QUrl> fileURLs = dlg->getOpenUrls(
            QUrl(),
            i18n("Select File(s) to Send to %1", recipient)
        );
        for (const QUrl& fileUrl : fileURLs) {
            addDccSend(recipient, fileUrl, dlg->passiveSend());
        }
        delete dlg;
    }
}

void Server::slotNewDccTransferItemQueued(DCC::Transfer* transfer)
{
    if (transfer->getConnectionId() == connectionId() )
    {
        qCDebug(KONVERSATION_LOG) << "connecting slots for " << transfer->getFileName() << " [" << transfer->getType() << "]";
        if ( transfer->getType() == DCC::Transfer::Receive )
        {
            connect( transfer, &DCC::Transfer::done, this, &Server::dccGetDone );
            connect( transfer, &DCC::Transfer::statusChanged, this, &Server::dccStatusChanged );
        }
        else
        {
            connect( transfer, &DCC::Transfer::done, this, &Server::dccSendDone );
            connect( transfer, &DCC::Transfer::statusChanged, this, &Server::dccStatusChanged );
        }
    }
}

void Server::addDccSend(const QString &recipient, const QUrl &fileURL, bool passive, const QString &altFileName, quint64 fileSize)
{
    if (!fileURL.isValid())
    {
        return;
    }

    // We already checked that the file exists in output filter / requestDccSend() resp.
    DCC::TransferSend* newDcc = Application::instance()->getDccTransferManager()->newUpload();

    newDcc->setConnectionId(connectionId());

    newDcc->setPartnerNick(recipient);
    newDcc->setFileURL(fileURL);
    newDcc->setReverse(passive);
    if (!altFileName.isEmpty())
        newDcc->setFileName(altFileName);
    if (fileSize != 0)
        newDcc->setFileSize(fileSize);

    Q_EMIT addDccPanel();

    if (newDcc->queue())
        newDcc->start();
}

QString Server::recoverDccFileName(const QStringList & dccArguments, int offset) const
{
    QString fileName;
    if(dccArguments.count() > offset + 1)
    {
        qCDebug(KONVERSATION_LOG) << "recover filename";
        const int argumentOffsetSize = dccArguments.size() - offset;
        for (int i = 0; i < argumentOffsetSize; ++i)
        {
            fileName += dccArguments.at(i);
            //if not last element, append a space
            if (i < (argumentOffsetSize - 1))
            {
                fileName += QLatin1Char(' ');
            }
        }
    }
    else
    {
        fileName = dccArguments.at(0);
    }

    return cleanDccFileName(fileName);
}

QString Server::cleanDccFileName(const QString& filename) const
{
    QString cleanFileName = filename;

    //we want a clean filename to get rid of the mass """filename"""
    //NOTE: if a filename starts really with a ", it is escaped -> \" (2 chars)
    //      but most clients don't support that and just replace it with a _
    while (cleanFileName.startsWith(QLatin1Char('\"')) && cleanFileName.endsWith(QLatin1Char('\"')))
    {
        cleanFileName = cleanFileName.mid(1, cleanFileName.length() - 2);
    }

    return cleanFileName;
}

quint16 Server::stringToPort(const QString &port, bool *ok)
{
    bool toUintOk = false;
    uint uPort32 = port.toUInt(&toUintOk);
    // ports over 65535 are invalid, someone sends us bad data
    if (!toUintOk || uPort32 > USHRT_MAX)
    {
        if (ok)
        {
            *ok = false;
        }
    }
    else
    {
        if (ok)
        {
            *ok = true;
        }
    }
    return (quint16)uPort32;
}

QString Server::recipientNick() const
{
    QStringList nickList;

    // fill nickList with all nicks we know about
    for (Channel* lookChannel : m_channelList) {
        const auto lookChannelNickList = lookChannel->getNickList();
        for (Nick* lookNick : lookChannelNickList) {
            if (!nickList.contains(lookNick->getChannelNick()->getNickname()))
                nickList.append(lookNick->getChannelNick()->getNickname());
        }
    }

    // add Queries as well, but don't insert duplicates
    for (Query* lookQuery : m_queryList) {
        if(!nickList.contains(lookQuery->getName())) nickList.append(lookQuery->getName());
    }
    QStringListModel model;
    model.setStringList(nickList);
    return DCC::RecipientDialog::getNickname(getViewContainer()->getWindow(), &model);
}

void Server::addDccGet(const QString &sourceNick, const QStringList &dccArguments)
{
    //filename ip port filesize [token]
    QString ip;
    quint16 port;
    QString fileName;
    quint64 fileSize;
    QString token;
    const int argumentSize = dccArguments.count();
    bool ok = true;

    if (dccArguments.at(argumentSize - 3) == QLatin1Char('0')) //port==0, for passive send, ip can't be 0
    {
        //filename ip port(0) filesize token
        fileName = recoverDccFileName(dccArguments, 4); //ip port filesize token
        ip = DCC::DccCommon::numericalIpToTextIp( dccArguments.at(argumentSize - 4) ); //-1 index, -1 token, -1 port, -1 filesize
        port = 0;
        fileSize = dccArguments.at(argumentSize - 2).toULongLong(); //-1 index, -1 token
        token = dccArguments.at(argumentSize - 1); //-1 index
    }
    else
    {
        //filename ip port filesize
        ip = DCC::DccCommon::numericalIpToTextIp( dccArguments.at(argumentSize - 3) ); //-1 index, -1 filesize
        fileName = recoverDccFileName(dccArguments, 3); //ip port filesize
        fileSize = dccArguments.at(argumentSize - 1).toULongLong(); //-1 index
        port = stringToPort(dccArguments.at(argumentSize - 2), &ok); //-1 index, -1 filesize
    }

    if (!ok)
    {
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1=nickname","Received invalid DCC SEND request from %1.",
                                       sourceNick));
        return;
    }

    DCC::TransferRecv* newDcc = Application::instance()->getDccTransferManager()->newDownload();

    newDcc->setConnectionId(connectionId());
    newDcc->setPartnerNick(sourceNick);

    newDcc->setPartnerIp(ip);
    newDcc->setPartnerPort(port);
    newDcc->setFileName(fileName);
    newDcc->setFileSize(fileSize);
    // Reverse DCC
    if (!token.isEmpty())
    {
        newDcc->setReverse(true, token);
    }

    qCDebug(KONVERSATION_LOG) << "ip: " << ip;
    qCDebug(KONVERSATION_LOG) << "port: " << port;
    qCDebug(KONVERSATION_LOG) << "filename: " << fileName;
    qCDebug(KONVERSATION_LOG) << "filesize: " << fileSize;
    qCDebug(KONVERSATION_LOG) << "token: " << token;

    //emit after data was set
    Q_EMIT addDccPanel();

    if ( newDcc->queue() )
    {
        appendMessageToFrontmost( i18n( "DCC" ),
                                  i18n( "%1 offers to send you \"%2\" (%3)...",
                                        newDcc->getPartnerNick(),
                                        fileName,
                                        ( newDcc->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( newDcc->getFileSize() ) ) );

        if (Preferences::self()->dccAutoGet())
            newDcc->start();
    }
}

void Server::addDccChat(const QString& sourceNick, const QStringList& dccArguments)
{
    //chat ip port [token]
    QString ip;
    quint16 port = 0;
    QString token;
    bool reverse = false;
    const int argumentSize = dccArguments.count();
    bool ok = true;
    QString extension;

    extension = dccArguments.at(0);
    ip = DCC::DccCommon::numericalIpToTextIp(dccArguments.at(1));

    if (argumentSize == 3)
    {
        //extension ip port
        port = stringToPort(dccArguments.at(2), &ok);
    }
    else if (argumentSize == 4)
    {
        //extension ip port(0) token
        token = dccArguments.at(3);
        reverse = true;
    }

    if (!ok)
    {
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1=nickname","Received invalid DCC CHAT request from %1.",
                                       sourceNick));
        return;
    }

    DCC::Chat* newChat = Application::instance()->getDccTransferManager()->newChat();

    newChat->setConnectionId(connectionId());
    newChat->setPartnerNick(sourceNick);
    newChat->setOwnNick(getNickname());

    qCDebug(KONVERSATION_LOG) << "ip: " << ip;
    qCDebug(KONVERSATION_LOG) << "port: " << port;
    qCDebug(KONVERSATION_LOG) << "token: " << token;
    qCDebug(KONVERSATION_LOG) << "extension: " << extension;

    newChat->setPartnerIp(ip);
    newChat->setPartnerPort(port);
    newChat->setReverse(reverse, token);
    newChat->setSelfOpened(false);
    newChat->setExtension(extension);

    Q_EMIT addDccChat(newChat);
    newChat->start();
}

void Server::openDccChat(const QString& nickname)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    QString recipient(nickname);
    // if we don't have a recipient yet, let the user select one
    if (recipient.isEmpty())
    {
        recipient = recipientNick();
    }

    // do we have a recipient *now*?
    if (!recipient.isEmpty())
    {
        DCC::Chat* newChat = Application::instance()->getDccTransferManager()->newChat();
        newChat->setConnectionId(connectionId());
        newChat->setPartnerNick(recipient);
        newChat->setOwnNick(getNickname());
        newChat->setSelfOpened(true);
        Q_EMIT addDccChat(newChat);
        newChat->start();
    }
}

void Server::openDccWBoard(const QString& nickname)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    QString recipient(nickname);
    // if we don't have a recipient yet, let the user select one
    if (recipient.isEmpty())
    {
        recipient = recipientNick();
    }

    // do we have a recipient *now*?
    if (!recipient.isEmpty())
    {
        DCC::Chat* newChat = Application::instance()->getDccTransferManager()->newChat();
        newChat->setConnectionId(connectionId());
        newChat->setPartnerNick(recipient);
        newChat->setOwnNick(getNickname());
        // Set extension before emiting addDccChat
        newChat->setExtension(DCC::Chat::Whiteboard);
        newChat->setSelfOpened(true);
        Q_EMIT addDccChat(newChat);
        newChat->start();
    }
}

void Server::requestDccChat(const QString& partnerNick, const QString& extension, const QString& numericalOwnIp, quint16 ownPort)
{
    Konversation::OutputFilterResult result = getOutputFilter()->requestDccChat(partnerNick, extension, numericalOwnIp,ownPort);
    queue(result.toServer);
}

void Server::acceptDccGet(const QString& nick, const QString& file)
{
    Application::instance()->getDccTransferManager()->acceptDccGet(m_connectionId, nick, file);
}

void Server::dccSendRequest(const QString &partner, const QString &fileName, const QString &address, quint16 port, quint64 size)
{
    Konversation::OutputFilterResult result = getOutputFilter()->sendRequest(partner,fileName,address,port,size);
    queue(result.toServer);

    appendMessageToFrontmost( i18n( "DCC" ),
                              i18n( "Asking %1 to accept upload of \"%2\" (%3)...",
                                    partner,
                                    cleanDccFileName(fileName),
                                    ( size == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( size ) ) );
}

void Server::dccPassiveSendRequest(const QString& recipient,const QString& fileName,const QString& address,quint64 size,const QString& token)
{
    Konversation::OutputFilterResult result = getOutputFilter()->passiveSendRequest(recipient,fileName,address,size,token);
    queue(result.toServer);

    appendMessageToFrontmost( i18n( "DCC" ),
                              i18n( "Asking %1 to accept passive upload of \"%2\" (%3)...",
                                    recipient,
                                    cleanDccFileName(fileName),
                                    ( size == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( size ) ) );
}

void Server::dccPassiveChatRequest(const QString& recipient, const QString& extension, const QString& address, const QString& token)
{
    Konversation::OutputFilterResult result = getOutputFilter()->passiveChatRequest(recipient, extension, address, token);
    queue(result.toServer);

    appendMessageToFrontmost(i18n("DCC"),
                             i18nc("%1=name, %2=dcc extension, chat or wboard for example","Asking %1 to accept %2...", recipient, extension));
}

void Server::dccPassiveResumeGetRequest(const QString& sender,const QString& fileName,quint16 port,KIO::filesize_t startAt,const QString &token)
{
    Konversation::OutputFilterResult result = getOutputFilter()->resumePassiveRequest(sender,fileName,port,startAt,token);;
    queue(result.toServer);
}

void Server::dccResumeGetRequest(const QString &sender, const QString &fileName, quint16 port, KIO::filesize_t startAt)
{
    Konversation::OutputFilterResult result = getOutputFilter()->resumeRequest(sender,fileName,port,startAt);;
    queue(result.toServer);
}

void Server::dccReverseSendAck(const QString& partnerNick,const QString& fileName,const QString& ownAddress,quint16 ownPort,quint64 size,const QString& reverseToken)
{
    Konversation::OutputFilterResult result = getOutputFilter()->acceptPassiveSendRequest(partnerNick,fileName,ownAddress,ownPort,size,reverseToken);
    queue(result.toServer);
}

void Server::dccReverseChatAck(const QString& partnerNick, const QString& extension, const QString& ownAddress, quint16 ownPort, const QString& reverseToken)
{
    Konversation::OutputFilterResult result = getOutputFilter()->acceptPassiveChatRequest(partnerNick, extension, ownAddress, ownPort, reverseToken);
    queue(result.toServer);
}

void Server::dccRejectSend(const QString& partnerNick, const QString& fileName)
{
    Konversation::OutputFilterResult result = getOutputFilter()->rejectDccSend(partnerNick,fileName);
    queue(result.toServer);
}

void Server::dccRejectChat(const QString& partnerNick, const QString& extension)
{
    Konversation::OutputFilterResult result = getOutputFilter()->rejectDccChat(partnerNick, extension);
    queue(result.toServer);
}

void Server::startReverseDccChat(const QString &sourceNick, const QStringList &dccArguments)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    DCC::TransferManager* dtm = Application::instance()->getDccTransferManager();

    bool ok = true;
    QString partnerIP = DCC::DccCommon::numericalIpToTextIp(dccArguments.at(1));
    quint16 port = stringToPort(dccArguments.at(2), &ok);
    QString token = dccArguments.at(3);

    qCDebug(KONVERSATION_LOG) << "ip: " << partnerIP;
    qCDebug(KONVERSATION_LOG) << "port: " << port;
    qCDebug(KONVERSATION_LOG) << "token: " << token;

    if (!ok || dtm->startReverseChat(connectionId(), sourceNick,
                                    partnerIP, port, token) == nullptr)
    {
        // DTM could not find a matched item
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1 = nickname",
                                       "Received invalid passive DCC chat acceptance message from %1.",
                                       sourceNick));
    }
}

void Server::startReverseDccSendTransfer(const QString& sourceNick,const QStringList& dccArguments)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    DCC::TransferManager* dtm = Application::instance()->getDccTransferManager();

    bool ok = true;
    const int argumentSize = dccArguments.size();
    QString partnerIP = DCC::DccCommon::numericalIpToTextIp( dccArguments.at(argumentSize - 4) ); //dccArguments[1] ) );
    quint16 port = stringToPort(dccArguments.at(argumentSize - 3), &ok);
    QString token = dccArguments.at(argumentSize - 1);
    quint64 fileSize = dccArguments.at(argumentSize - 2).toULongLong();
    QString fileName = recoverDccFileName(dccArguments, 4); //ip port filesize token

    qCDebug(KONVERSATION_LOG) << "ip: " << partnerIP;
    qCDebug(KONVERSATION_LOG) << "port: " << port;
    qCDebug(KONVERSATION_LOG) << "filename: " << fileName;
    qCDebug(KONVERSATION_LOG) << "filesize: " << fileSize;
    qCDebug(KONVERSATION_LOG) << "token: " << token;

    if (!ok ||
        dtm->startReverseSending(connectionId(), sourceNick,
                                 fileName,  // filename
                                 partnerIP,  // partner IP
                                 port,  // partner port
                                 fileSize,  // filesize
                                 token  // Reverse DCC token
         ) == nullptr)
    {
        // DTM could not find a matched item
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1 = file name, %2 = nickname",
                                       "Received invalid passive DCC send acceptance message for \"%1\" from %2.",
                                       fileName,
                                       sourceNick));
    }
}

void Server::resumeDccGetTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
    DCC::TransferManager* dtm = Application::instance()->getDccTransferManager();

    //filename port position [token]
    QString fileName;
    quint64 position;
    quint16 ownPort;
    bool ok = true;
    const int argumentSize = dccArguments.count();

    if (dccArguments.at(argumentSize - 3) == QLatin1Char('0')) //-1 index, -1 token, -1 pos
    {
        fileName = recoverDccFileName(dccArguments, 3); //port position token
        ownPort = 0;
        position = dccArguments.at(argumentSize - 2).toULongLong(); //-1 index, -1 token
    }
    else
    {
        fileName = recoverDccFileName(dccArguments, 2); //port position
        ownPort = stringToPort(dccArguments.at(argumentSize - 2), &ok); //-1 index, -1 pos
        position = dccArguments.at(argumentSize - 1).toULongLong(); //-1 index
    }
    //do we need the token here?

    DCC::TransferRecv* dccTransfer = nullptr;
    if (ok)
    {
        dccTransfer = dtm->resumeDownload(connectionId(), sourceNick, fileName, ownPort, position);
    }

    if (dccTransfer)
    {
        appendMessageToFrontmost(i18n("DCC"),
                                 i18nc("%1 = file name, %2 = nickname of sender, %3 = percentage of file size, %4 = file size",
                                       "Resuming download of \"%1\" from %2 starting at %3% of %4...",
                                       fileName,
                                       sourceNick,
                                       QString::number( dccTransfer->getProgress()),
                                       (dccTransfer->getFileSize() == 0) ? i18n("unknown size") : KIO::convertSize(dccTransfer->getFileSize())));
    }
    else
    {
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1 = file name, %2 = nickname",
                                       "Received invalid resume acceptance message for \"%1\" from %2.",
                                       fileName,
                                       sourceNick));
    }
}

void Server::resumeDccSendTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
    DCC::TransferManager* dtm = Application::instance()->getDccTransferManager();

    bool passiv = false;
    QString fileName;
    quint64 position;
    QString token;
    quint16 ownPort;
    bool ok = true;
    const int argumentSize = dccArguments.count();

    //filename port filepos [token]
    if (dccArguments.at( argumentSize - 3) == QLatin1Char('0'))
    {
        //filename port filepos token
        passiv = true;
        ownPort = 0;
        token = dccArguments.at( argumentSize - 1); // -1 index
        position = dccArguments.at( argumentSize - 2).toULongLong(); // -1 index, -1 token
        fileName = recoverDccFileName(dccArguments, 3); //port filepos token
    }
    else
    {
        //filename port filepos
        ownPort = stringToPort(dccArguments.at(argumentSize - 2), &ok); //-1 index, -1 filesize
        position = dccArguments.at( argumentSize - 1).toULongLong(); // -1 index
        fileName = recoverDccFileName(dccArguments, 2); //port filepos
    }

    DCC::TransferSend* dccTransfer = nullptr;
    if (ok)
    {
        dccTransfer = dtm->resumeUpload(connectionId(), sourceNick, fileName, ownPort, position);
    }

    if (dccTransfer)
    {
        appendMessageToFrontmost(i18n("DCC"),
                                 i18nc("%1 = file name, %2 = nickname of recipient, %3 = percentage of file size, %4 = file size",
                                       "Resuming upload of \"%1\" to %2 starting at %3% of %4...",
                                       fileName,
                                       sourceNick,
                                       QString::number(dccTransfer->getProgress()),
                                       (dccTransfer->getFileSize() == 0) ? i18n("unknown size") : KIO::convertSize(dccTransfer->getFileSize())));

        // fileName can't have " here
        if (fileName.contains(QLatin1Char(' ')))
            fileName = QLatin1Char('\"')+fileName+QLatin1Char('\"');

        // FIXME: this operation should be done by TransferManager
        Konversation::OutputFilterResult result;
        if (passiv)
            result = getOutputFilter()->acceptPassiveResumeRequest( sourceNick, fileName, ownPort, position, token );
        else
            result = getOutputFilter()->acceptResumeRequest( sourceNick, fileName, ownPort, position );
        queue( result.toServer );
    }
    else
    {
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1 = file name, %2 = nickname",
                                       "Received invalid resume request for \"%1\" from %2.",
                                       fileName,
                                       sourceNick));
    }
}

void Server::rejectDccSendTransfer(const QString &sourceNick, const QStringList &dccArguments)
{
    DCC::TransferManager* dtm = Application::instance()->getDccTransferManager();

    //filename
    QString fileName = recoverDccFileName(dccArguments, 0);

    DCC::TransferSend* dccTransfer = dtm->rejectSend(connectionId(), sourceNick, fileName);

    if (!dccTransfer)
    {
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1 = file name, %2 = nickname",
                                       "Received invalid reject request for \"%1\" from %2.",
                                       fileName,
                                       sourceNick));
    }
}

void Server::rejectDccChat(const QString& sourceNick)
{
    DCC::TransferManager* dtm = Application::instance()->getDccTransferManager();

    DCC::Chat* dccChat = dtm->rejectChat(connectionId(), sourceNick);

    if (!dccChat)
    {
        appendMessageToFrontmost(i18n("Error"),
                                 i18nc("%1 = nickname",
                                       "Received invalid reject request from %1.",
                                       sourceNick));
    }
}

void Server::dccGetDone(DCC::Transfer* item)
{
    if (!item)
        return;

    if(item->getStatus() == DCC::Transfer::Done)
    {
        appendMessageToFrontmost(i18n("DCC"), i18nc("%1 = file name, %2 = nickname of sender",
            "Download of \"%1\" from %2 finished.", item->getFileName(), item->getPartnerNick()));
    }
    else if(item->getStatus() == DCC::Transfer::Failed)
    {
        appendMessageToFrontmost(i18n("DCC"), i18nc("%1 = file name, %2 = nickname of sender",
            "Download of \"%1\" from %2 failed. Reason: %3.", item->getFileName(),
            item->getPartnerNick(), item->getStatusDetail()));
    }
}

void Server::dccSendDone(DCC::Transfer* item)
{
    if (!item)
        return;

    if(item->getStatus() == DCC::Transfer::Done)
        appendMessageToFrontmost(i18n("DCC"), i18nc("%1 = file name, %2 = nickname of recipient",
            "Upload of \"%1\" to %2 finished.", item->getFileName(), item->getPartnerNick()));
    else if(item->getStatus() == DCC::Transfer::Failed)
        appendMessageToFrontmost(i18n("DCC"), i18nc("%1 = file name, %2 = nickname of recipient",
            "Upload of \"%1\" to %2 failed. Reason: %3.", item->getFileName(), item->getPartnerNick(),
            item->getStatusDetail()));
}

void Server::dccStatusChanged(DCC::Transfer *item, int newStatus, int oldStatus)
{
    if(!item)
        return;

    if ( item->getType() == DCC::Transfer::Send )
    {
        // when resuming, a message about the receiver's acceptance has been shown already, so suppress this message
        if ( newStatus == DCC::Transfer::Transferring && oldStatus == DCC::Transfer::WaitingRemote && !item->isResumed() )
            appendMessageToFrontmost( i18n( "DCC" ), i18nc( "%1 = file name, %2 nickname of recipient",
                "Sending \"%1\" to %2...", item->getFileName(), item->getPartnerNick() ) );
    }
    else  // type == Receive
    {
        if ( newStatus == DCC::Transfer::Transferring && !item->isResumed() )
        {
            appendMessageToFrontmost( i18n( "DCC" ),
                                        i18nc( "%1 = file name, %2 = file size, %3 = nickname of sender", "Downloading \"%1\" (%2) from %3...",
                                              item->getFileName(),
                                            ( item->getFileSize() == 0 ) ? i18n( "unknown size" ) : KIO::convertSize( item->getFileSize() ),
                                            item->getPartnerNick() ) );
        }
    }
}

void Server::removeQuery(Query* query)
{
    m_queryList.removeOne(query);
    query->deleteLater();
}

void Server::sendJoinCommand(const QString& name, const QString& password)
{
    Konversation::OutputFilterResult result = getOutputFilter()->parse(getNickname(),
        Preferences::self()->commandChar() + QStringLiteral("JOIN ") + name + QLatin1Char(' ') + password, QString());
    queue(result.toServer);
}

Channel* Server::joinChannel(const QString& name, const QString& hostmask, const QHash<QString, QString> &messageTags)
{
    // (re-)join channel, open a new panel if needed
    Channel* channel = getChannelByName(name);

    if (!channel)
    {
        channel=getViewContainer()->addChannel(this,name);
        Q_ASSERT(channel);
        channel->setNickname(getNickname());
        channel->indicateAway(m_away);

        if (getServerGroup())
        {
            Konversation::ChannelSettings channelSettings = getServerGroup()->channelByNameFromHistory(name);
            channel->setNotificationsEnabled(channelSettings.enableNotifications());
            getServerGroup()->appendChannelHistory(channelSettings);
        }

        m_channelList.append(channel);
        m_loweredChannelNameHash.insert(channel->getName().toLower(), channel);

        connect(channel, &Channel::sendFile, this, QOverload<>::of(&Server::requestDccSend));
        connect(this, &Server::nicknameChanged, channel, &Channel::setNickname);
    }

    // Move channel from unjoined (if present) to joined list and add our own nickname to the joined list.
    ChannelNickPtr channelNick = addNickToJoinedChannelsList(name, getNickname());

    if ((channelNick->getHostmask() != hostmask ) && !hostmask.isEmpty())
    {
        NickInfoPtr nickInfo = channelNick->getNickInfo();
        nickInfo->setHostmask(hostmask);
    }

    channel->joinNickname(channelNick, messageTags);

    return channel;
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

    m_channelList.removeOne(channel);
    m_loweredChannelNameHash.remove(channel->getName().toLower());

    if (!isConnected())
        updateAutoJoin();
}

void Server::updateChannelMode(const QString &updater, const QString &channelName, char mode, bool plus, const QString &parameter, const QHash<QString, QString> &messageTags)
{

    Channel* channel=getChannelByName(channelName);

    if(channel)                                   //Let the channel be verbose to the screen about the change, and update channelNick
        channel->updateMode(updater, mode, plus, parameter, messageTags);
    // TODO: What is mode character for owner?
    // Answer from JOHNFLUX - I think that admin is the same as owner.  Channel.h has owner as "a"
    // "q" is the likely answer.. UnrealIRCd and euIRCd use it.
    // TODO these need to become dynamic
    QString userModes=QStringLiteral("vhoqa");                    // voice halfop op owner admin
    int modePos = userModes.indexOf(QLatin1Char(mode));
    if (modePos > 0)
    {
        ChannelNickPtr updateeNick = getChannelNick(channelName, parameter);
        if(!updateeNick)
        {
/*
            if(parameter.isEmpty())
            {
                qCDebug(KONVERSATION_LOG) << "in updateChannelMode, a nick with no-name has had their mode '" << mode << "' changed to (" <<plus << ") in channel '" << channelName << "' by " << updater << ".  How this happened, I have no idea.  Please report this message to irc #konversation if you want to be helpful." << endl << "Ignoring the error and continuing.";
                                                  //this will get their attention.
                qCDebug(KONVERSATION_LOG) << kBacktrace();
            }
            else
            {
                qCDebug(KONVERSATION_LOG) << "in updateChannelMode, could not find updatee nick " << parameter << " for channel " << channelName;
                qCDebug(KONVERSATION_LOG) << "This could indicate an obscure race condition that is safely being handled (like the mode of someone changed and they quit almost simulatanously, or it could indicate an internal error.";
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
            addBan(channelName, QStringLiteral("%1 %2 %3").arg(parameter, updater).arg(QDateTime::currentDateTime().toSecsSinceEpoch()));
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

Channel* Server::getChannelByName(const QString& name) const
{
    if (name.isEmpty()) {
        return nullptr;
    }

    // Convert wanted channel name to lowercase
    QString wanted = name.toLower();

    QRegularExpressionMatch p = m_targetMatcher.match(wanted);
    int index = p.capturedStart(2);

    if (index >= 0)
    {
        wanted = wanted.mid(index);

        if (m_loweredChannelNameHash.contains(wanted))
            return m_loweredChannelNameHash.value(wanted);
    }

    return nullptr;
}

Query* Server::getQueryByName(const QString& name) const
{
    // Convert wanted query name to lowercase
    QString wanted = name.toLower();

    // Traverse through list to find the query with "name"
    for (Query* lookQuery : std::as_const(m_queryList)) {
        if(lookQuery->getName().toLower()==wanted) return lookQuery;
    }
    // No query by that name found? Must be a new query request. Return 0
    return nullptr;
}

ChatWindow* Server::getChannelOrQueryByName(const QString& name) const
{
    ChatWindow* window = getChannelByName(name);

    if (!window)
        window = getQueryByName(name);

    return window;
}

void Server::queueNicks(const QString& channelName, const QStringList& nicknameList)
{
    Channel* channel = getChannelByName(channelName);
    if (channel) channel->queueNicks(nicknameList);
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
    QString lcNickname(nickname.toLower());
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
    QString lcChannelName = channelName.toLower();
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
        channelNick = new ChannelNick(nickInfo, lcChannelName);
        Q_ASSERT(channelNick);
        channel->insert(lcNickname, channelNick);
        doChannelMembersChangedSignal = true;
    }
    channelNick = (*channel)[lcNickname];
    Q_ASSERT(channelNick);                        //Since we just added it if it didn't exist, it should be guaranteed to exist now
    if (doWatchedNickChangedSignal) Q_EMIT watchedNickChanged(this, nickname, true);
    if (doChannelJoinedSignal) Q_EMIT channelJoinedOrUnjoined(this, channelName, true);
    if (doChannelMembersChangedSignal) Q_EMIT channelMembersChanged(this, channelName, true, false, nickname);
    return channelNick;
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
    QString lcNickname(nickname.toLower());
    // Create NickInfo if not already created.
    NickInfoPtr nickInfo = getNickInfo(nickname);
    if (!nickInfo)
    {
        nickInfo = new NickInfo(nickname, this);
        m_allNicks.insert(lcNickname, nickInfo);
        doWatchedNickChangedSignal = isWatchedNick(nickname);
    }
    // Move the channel from joined list (if present) to unjoined list.
    QString lcChannelName = channelName.toLower();
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
        channelNick = new ChannelNick(nickInfo, lcChannelName);
        channel->insert(lcNickname, channelNick);
        doChannelMembersChangedSignal = true;
    }
    channelNick = (*channel)[lcNickname];
    // Set the mode for the nick in this channel.
    if (doWatchedNickChangedSignal) Q_EMIT watchedNickChanged(this, nickname, true);
    if (doChannelUnjoinedSignal) Q_EMIT channelJoinedOrUnjoined(this, channelName, false);
    if (doChannelMembersChangedSignal) Q_EMIT channelMembersChanged(this, channelName, false, false, nickname);
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
        QString lcNickname(nickname.toLower());
        nickInfo = new NickInfo(nickname, this);
        m_allNicks.insert(lcNickname, nickInfo);
    }

    Q_EMIT watchedNickChanged(this, nickname, true);

    appendMessageToFrontmost(i18nc("Message type", "Notify"), i18n("%1 is online (%2).", nickname, getServerName()), QHash<QString, QString>(), getStatusView());

    Application::instance()->notificationHandler()->nickOnline(getStatusView(), nickname);

    nickInfo->setPrintedOnline(true);
    return nickInfo;
}

void Server::setWatchedNickOffline(const QString& nickname, const NickInfoPtr &nickInfo)
{
    Q_UNUSED(nickInfo)

    Q_EMIT watchedNickChanged(this, nickname, false);

    appendMessageToFrontmost(i18nc("Message type", "Notify"), i18n("%1 went offline (%2).", nickname, getServerName()), QHash<QString, QString>(), getStatusView());

    Application::instance()->notificationHandler()->nickOffline(getStatusView(), nickname);

}

bool Server::setNickOffline(const QString& nickname)
{
    QString lcNickname(nickname.toLower());
    NickInfoPtr nickInfo = getNickInfo(lcNickname);

    bool wasOnline = nickInfo ? nickInfo->getPrintedOnline() : false;

    if (wasOnline)
    {
        // Delete from query list, if present.
        if (m_queryNicks.contains(lcNickname)) m_queryNicks.remove(lcNickname);
        // Delete the nickname from all channels (joined or unjoined).
        const QStringList nickChannels = getNickChannels(lcNickname);

        for (const QString& channel : nickChannels) {
            removeChannelNick(channel, lcNickname);
        }

        // Delete NickInfo.
        if (m_allNicks.contains(lcNickname)) m_allNicks.remove(lcNickname);
        // If the nick was in the watch list, emit various signals and messages.
        if (isWatchedNick(nickname)) setWatchedNickOffline(nickname, nickInfo);

        nickInfo->setPrintedOnline(false);
    }

    return (nickInfo != nullptr);
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
    QString lcNickname(nickname.toLower());
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
    QString lcChannelName = channelName.toLower();
    QString lcNickname = nickname.toLower();
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
        else
        {
            qCDebug(KONVERSATION_LOG) << "Error: Tried to remove nickname=" << nickname << " from joined channel=" << channelName;
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
            else
            {
                qCDebug(KONVERSATION_LOG) << "Error: Tried to remove nickname=" << nickname << " from unjoined channel=" << channelName;
            }
        }
    }
    if (doSignal) Q_EMIT channelMembersChanged(this, channelName, joined, true, nickname);
}

QStringList Server::getWatchList() const
{
    // no nickinfo ISON for the time being
    if (getServerGroup())
        return Preferences::notifyListByGroupId(getServerGroup()->id());
    else
        return QStringList();

    if (m_serverISON)
        return m_serverISON->getWatchList();
    else
        return QStringList();
}

QStringList Server::getISONList() const
{
    // no nickinfo ISON for the time being
    if (getServerGroup())
        return Preferences::notifyListByGroupId(getServerGroup()->id());
    else
        return QStringList();

    if (m_serverISON)
        return m_serverISON->getISONList();
    else
        return QStringList();
}

QString Server::getISONListString() const { return getISONList().join(QLatin1Char(' ')); }

/**
 * Return true if the given nickname is on the watch list.
 */
bool Server::isWatchedNick(const QString& nickname) const
{
    // no nickinfo ISON for the time being
    if (getServerGroup())
        return Preferences::isNotify(getServerGroup()->id(), nickname);
    else
        return false;

    // ###### ERROR: not reached
    return getWatchList().contains(nickname, Qt::CaseInsensitive);
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
    QString lcChannelName = channelName.toLower();
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
            if (!watchListLower.contains(lcNickname))
            {
                // Remove the unwatched nickname from the unjoined channel.
                channel->erase(member);
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
    if (doSignal) Q_EMIT channelJoinedOrUnjoined(this, channelName, false);
}

// Renames a nickname in all NickInfo lists.
// Returns pointer to the NickInfo object or 0 if nick not found.
void Server::renameNickInfo(NickInfoPtr nickInfo, const QString& newname)
{
    if (nickInfo)
    {
        // Get existing lowercase nickname and rename nickname in the NickInfo object.
        QString lcNickname(nickInfo->loweredNickname());
        nickInfo->setNickname(newname);
        QString lcNewname(newname.toLower());
        // Rename the key in m_allNicks list.
        m_allNicks.remove(lcNickname);
        m_allNicks.insert(lcNewname, nickInfo);
        // Rename key in the joined and unjoined lists.
        const QStringList nickChannels = getNickChannels(lcNickname);

        for (const QString& channelName : nickChannels) {
            const ChannelNickMap *channel = getChannelMembers(channelName);
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
        qCDebug(KONVERSATION_LOG) << "was called for newname='" << newname << "' but nickInfo is null";
    }
}

Channel* Server::nickJoinsChannel(const QString &channelName, const QString &nickname, const QString &hostmask, const QString &account,
                                  const QString &realName, const QHash<QString, QString> &messageTags)
{
    Channel* outChannel = getChannelByName(channelName);
    if(outChannel)
    {
        // Update NickInfo.
        ChannelNickPtr channelNick = addNickToJoinedChannelsList(channelName, nickname);
        NickInfoPtr nickInfo = channelNick->getNickInfo();
        if ((nickInfo->getHostmask() != hostmask) && !hostmask.isEmpty())
        {
            nickInfo->setHostmask(hostmask);
        }
        if (!account.isEmpty())
        {
            nickInfo->setAccount(account);
        }
        if (!realName.isEmpty())
        {
            nickInfo->setRealName(realName);
        }
        outChannel->joinNickname(channelNick, messageTags);
    }

    return outChannel;
}

void Server::addHostmaskToNick(const QString& sourceNick, const QString& sourceHostmask)
{
    // Update NickInfo.
    NickInfoPtr nickInfo = getNickInfo(sourceNick);
    if (nickInfo)
    {
        if ((nickInfo->getHostmask() != sourceHostmask) && !sourceHostmask.isEmpty())
        {
            nickInfo->setHostmask(sourceHostmask);
        }
    }
}

Channel* Server::removeNickFromChannel(const QString &channelName, const QString &nickname, const QString &reason, const QHash<QString, QString> &messageTags, bool quit)
{
    Channel* outChannel = getChannelByName(channelName);
    if(outChannel)
    {
        outChannel->flushNickQueue();
        ChannelNickPtr channelNick = getChannelNick(channelName, nickname);
        if(channelNick)
        {
            outChannel->removeNick(channelNick,reason,quit, messageTags);
        }
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

void Server::nickWasKickedFromChannel(const QString &channelName, const QString &nickname, const QString &kicker, const QString &reason, const QHash<QString, QString> &messageTags)
{
    Channel* outChannel = getChannelByName(channelName);
    if(outChannel)
    {
        outChannel->flushNickQueue();
        ChannelNickPtr channelNick = getChannelNick(channelName, nickname);

        if(channelNick)
        {
          outChannel->kickNick(channelNick, kicker, reason, messageTags);
          // Tell Nickinfo
          removeChannelNick(channelName,nickname);
        }
    }
}

void Server::removeNickFromServer(const QString &nickname,const QString &reason, const QHash<QString, QString> &messageTags)
{
    for (Channel* channel : std::as_const(m_channelList)) {
        channel->flushNickQueue();
        // Check if nick is in this channel or not.
        if(channel->getNickByName(nickname))
            removeNickFromChannel(channel->getName(), nickname, reason, messageTags, true);
    }

    Query* query = getQueryByName(nickname);
    if (query) query->quitNick(reason, messageTags);

    // Delete the nick from all channels and then delete the nickinfo,
    // emitting signal if on the watch list.
    setNickOffline(nickname);
}

void Server::renameNick(const QString &nickname, const QString &newNick, const QHash<QString, QString> &messageTags)
{
    if(nickname.isEmpty() || newNick.isEmpty())
    {
        qCDebug(KONVERSATION_LOG) << "called with empty strings!  Trying to rename '" << nickname << "' to '" << newNick << "'";
        return;
    }

    // If this was our own nickchange, tell our server object about it
    if (nickname == getNickname())
        setNickname(newNick);

    //Actually do the rename.
    NickInfoPtr nickInfo = getNickInfo(nickname);

    if(!nickInfo)
    {
        qCDebug(KONVERSATION_LOG) << "called for nickname '" << nickname << "' to '" << newNick << "' but getNickInfo('" << nickname << "') returned no results.";
    }
    else
    {
        renameNickInfo(nickInfo, newNick);
        //The rest of the code below allows the channels to echo to the user to tell them that the nick has changed.

        // Rename the nick in every channel they are in
        for (Channel* channel : std::as_const(m_channelList)) {
            channel->flushNickQueue();

            // All we do is notify that the nick has been renamed.. we haven't actually renamed it yet
            if (channel->getNickByName(nickname)) channel->nickRenamed(nickname, *nickInfo, messageTags);
        }

        //Watched nicknames stuff
        if (isWatchedNick(nickname)) setWatchedNickOffline(nickname, NickInfoPtr());
    }

    // We had an encrypt conversation with the user that changed his nick, lets copy the key to the new nick and remove the old nick
    #if HAVE_QCA2
    QByteArray userKey = getKeyForRecipient(nickname);

    if (!userKey.isEmpty())
    {
        setKeyForRecipient(newNick, userKey);
        m_keyHash.remove(nickname.toLower());
    }
    #endif

}

void Server::userhost(const QString& nick,const QString& hostmask,bool away,bool /* ircOp */)
{
    addHostmaskToNick(nick, hostmask);
    // remember my IP for DCC things
                                                  // myself
    if (m_ownIpByUserhost.isEmpty() && nick == getNickname())
    {
        QString myhost = hostmask.section(QLatin1Char('@'), 1);
        // Use async lookup else you will be blocking GUI badly
        QHostInfo::lookupHost(myhost, this, &Server::gotOwnResolvedHostByUserhost);
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

void Server::gotOwnResolvedHostByUserhost(const QHostInfo& res)
{
    if ( res.error() == QHostInfo::NoError && !res.addresses().isEmpty() )
        m_ownIpByUserhost = res.addresses().first().toString();
    else
        qCDebug(KONVERSATION_LOG) << "Got error: " << res.errorString();
}

void Server::appendServerMessageToChannel(const QString& channel,const QString& type,const QString& message, const QHash<QString, QString> &messageTags)
{
    Channel* outChannel = getChannelByName(channel);
    if (outChannel) outChannel->appendServerMessage(type, message, messageTags);
}

void Server::appendCommandMessageToChannel(const QString& channel, const QString& command, const QString& message, const QHash<QString, QString> &messageTags, bool highlight, bool parseURL)
{
    Channel* outChannel = getChannelByName(channel);
    if (outChannel)
    {
        outChannel->appendCommandMessage(command, message, messageTags, parseURL, !highlight);
    }
    else
    {
        appendStatusMessage(command, QStringLiteral("%1 %2").arg(channel, message), messageTags);
    }
}

void Server::appendStatusMessage(const QString& type, const QString& message, const QHash<QString, QString> &messageTags)
{
    getStatusView()->appendServerMessage(type, message, messageTags);
}

void Server::appendMessageToFrontmost(const QString& type, const QString& message, const QHash<QString, QString> &messageTags, bool parseURL)
{
    getViewContainer()->appendToFrontmost(type, message, getStatusView(), messageTags, parseURL);
}

void Server::setNickname(const QString &newNickname)
{
    m_nickname = newNickname;
    m_loweredNickname = newNickname.toLower();
    if (!m_nickListModel->stringList().contains(newNickname)) {
        m_nickListModel->insertRows(m_nickListModel->rowCount(), 1);
        m_nickListModel->setData(m_nickListModel->index(m_nickListModel->rowCount() -1 , 0), newNickname, Qt::DisplayRole);
    }
    Q_EMIT nicknameChanged(newNickname);
}

void Server::setChannelTopic(const QString &channel, const QString &newTopic, const QHash<QString, QString> &messageTags)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
    {
        // encoding stuff is done in send()
        outChannel->setTopic(newTopic, messageTags);
    }
}

                                                  // Overloaded
void Server::setChannelTopic(const QString& nickname, const QString &channel, const QString &newTopic, const QHash<QString, QString> &messageTags)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
    {
        // encoding stuff is done in send()
        outChannel->setTopic(nickname, newTopic, messageTags);
    }
}

void Server::setTopicAuthor(const QString& channel, const QString& author, const QDateTime &time)
{
    Channel* outChannel = getChannelByName(channel);
    if(outChannel)
        outChannel->setTopicAuthor(author, time);
}

void Server::endOfWho(const QString& target)
{
    Channel* channel = getChannelByName(target);
    if(channel)
        channel->scheduleAutoWho();
}

void Server::endOfNames(const QString& target)
{
    Channel* channel = getChannelByName(target);
    if(channel)
        channel->endOfNames();
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

QString Server::parseWildcards(const QString& toParse, ChatWindow* context, const QStringList &nicks)
{
    QString inputLineText;

    if (context && context->getInputBar())
        inputLineText = context->getInputBar()->toPlainText();

    if (!context)
        return parseWildcards(toParse, getNickname(), QString(), QString(), QString(), QString());
    else if (context->getType() == ChatWindow::Channel)
    {
        auto* channel = static_cast<Channel*>(context);

        return parseWildcards(toParse, getNickname(), context->getName(), channel->getPassword(),
            !nicks.isEmpty() ? nicks : channel->getSelectedNickList(), inputLineText);
    }
    else if (context->getType() == ChatWindow::Query)
        return parseWildcards(toParse, getNickname(), context->getName(), QString(), context->getName(), inputLineText);

    return parseWildcards(toParse, getNickname(), context->getName(), QString(), QString(), inputLineText);
}

QString Server::parseWildcards(const QString& toParse,
const QString& sender,
const QString& channelName,
const QString& channelKey,
const QString& nick,
const QString& inputLineText)
{
    return parseWildcards(toParse, sender, channelName, channelKey, nick.split(QLatin1Char(' '), Qt::SkipEmptyParts), inputLineText);
}

QString Server::parseWildcards(const QString& toParse,
const QString& sender,
const QString& channelName,
const QString& channelKey,
const QStringList& nickList,
const QString& inputLineText
)
{
    // store the parsed version
    QString out;

    // default separator
    QString separator(QStringLiteral(" "));

    int index = 0, found = 0;
    QChar toExpand;

    while ((found = toParse.indexOf(QLatin1Char('%'), index)) != -1)
    {
                                                  // append part before the %
        out.append(QStringView(toParse).mid(index,found-index));
        index = found + 1;                        // skip the part before, including %
        if (index >= (int)toParse.length())
            break;                                // % was the last char (not valid)
        toExpand = toParse.at(index++);
        if (toExpand == QLatin1Char('s'))
        {
            found = toParse.indexOf(QLatin1Char('%'), index);
            if (found == -1)                      // no other % (not valid)
                break;
            separator = toParse.mid(index,found-index);
            index = found + 1;                    // skip separator, including %
        }
        else if (toExpand == QLatin1Char('u'))
        {
            out.append(nickList.join(separator));
        }
        else if (toExpand == QLatin1Char('c'))
        {
            if(!channelName.isEmpty())
                out.append(channelName);
        }
        else if (toExpand == QLatin1Char('o'))
        {
            out.append(sender);
        }
        else if (toExpand == QLatin1Char('k'))
        {
            if(!channelKey.isEmpty())
                out.append(channelKey);
        }
        else if (toExpand == QLatin1Char('K'))
        {
            if(getConnectionSettings().server().password().isEmpty())
                out.append(getConnectionSettings().server().password());
        }
        else if (toExpand == QLatin1Char('n'))
        {
            out.append(QLatin1Char('\n'));
        }
        else if (toExpand == QLatin1Char('p'))
        {
            out.append(QLatin1Char('%'));
        }
        else if (toExpand == QLatin1Char('i'))
        {
            out.append(inputLineText);
        }
    }

                                                  // append last part
    out.append(QStringView(toParse).mid(index,toParse.length()-index));
    return out;
}

void Server::sendToAllChannels(const QString &text)
{
    // Send a message to all channels we are in
    for (Channel* channel : std::as_const(m_channelList)) {
        channel->sendText(text);
    }
}

void Server::invitation(const QString& nick,const QString& channel)
{
    if(!m_inviteDialog)
    {
        QDialogButtonBox::StandardButton buttonCode = QDialogButtonBox::Cancel;

        if(!InviteDialog::shouldBeShown(buttonCode))
        {
            if (buttonCode == QDialogButtonBox::Ok)
                sendJoinCommand(channel);

            return;
        }

        m_inviteDialog = new InviteDialog (getViewContainer()->getWindow());
        connect(m_inviteDialog, &InviteDialog::joinChannelsRequested,
                this, [this](const QString& channels) { sendJoinCommand(channels); });
    }

    m_inviteDialog->show();
    m_inviteDialog->raise();

    m_inviteDialog->addInvite(nick, channel);
}

void Server::scriptNotFound(const QString& name)
{
    appendMessageToFrontmost(i18n("D-Bus"),i18n("Error: Could not find script \"%1\".", name));
}

void Server::scriptExecutionError(const QString& name)
{
    appendMessageToFrontmost(i18n("D-Bus"),i18n("Error: Could not execute script \"%1\". Check file permissions.", name));
}

bool Server::isAChannel(const QString &channel) const
{
    if (channel.isEmpty()) return false;

    QRegularExpressionMatch x = m_targetMatcher.match(channel);
    int index = x.capturedStart(2);

    return (index >= 0 && getChannelTypes().contains(channel.at(index)));
}

void Server::addRawLog(bool show)
{
    if (!m_rawLog) m_rawLog = getViewContainer()->addRawLog(this);

    connect(this, &Server::serverOnline, m_rawLog, &RawLog::serverOnline);

    // bring raw log to front since the main window does not do this for us
    if (show) Q_EMIT showView(m_rawLog);
}

void Server::closeRawLog()
{
    delete m_rawLog;
}

void Server::requestOpenChannelListPanel(const QString& filter)
{
    getViewContainer()->openChannelList(this, filter, true);
}

ChannelListPanel* Server::addChannelListPanel()
{
    if(!m_channelListPanel)
    {
        m_channelListPanel = getViewContainer()->addChannelListPanel(this);

        connect(&m_inputFilter, &InputFilter::endOfChannelList, m_channelListPanel.data(), &ChannelListPanel::endOfChannelList);
        connect(m_channelListPanel.data(), &ChannelListPanel::refreshChannelList, this, &Server::requestChannelList);
        connect(m_channelListPanel, &ChannelListPanel::joinChannel, this, [this](const QString& name) { sendJoinCommand(name); });
        connect(this, &Server::serverOnline, m_channelListPanel, &ChannelListPanel::serverOnline);
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
    delete m_channelListPanel;
}

void Server::updateAutoJoin(Konversation::ChannelList channels)
{
    Konversation::ChannelList tmpList;

    if (!channels.isEmpty())
    {
        tmpList.reserve(channels.size());
        for (const ChannelSettings& cs : std::as_const(channels)) {
            tmpList << cs;
        }
    }
    else if (m_channelList.isEmpty() && getServerGroup())
        tmpList = getServerGroup()->channelList();
    else
    {
        tmpList.reserve(m_channelList.size());
        for (Channel* channel : std::as_const(m_channelList)) {
            tmpList << channel->channelSettings();
        }
    }

    if (!tmpList.isEmpty())
    {
        setAutoJoinCommands(generateJoinCommand(tmpList));
        setAutoJoin(!m_autoJoinCommands.isEmpty());
    }
    else
    {
        m_autoJoinCommands.clear();
        setAutoJoin(false);
    }
}


QStringList Server::generateJoinCommand(const Konversation::ChannelList &tmpList)
{
    QStringList channels;
    QStringList passwords;
    QStringList joinCommands;
    uint length = 0;

    for (const auto& channel : tmpList) {
        const QString channelName = channel.name();

        // Only add the channel to the JOIN command if it has a valid channel name.
        if (isAChannel(channelName)) {
            const QString password = channel.password().isEmpty() ? QStringLiteral(".") : channel.password();

            uint currentLength = getIdentity()->getCodec()->fromUnicode(channelName).length();
            currentLength += getIdentity()->getCodec()->fromUnicode(password).length();

            //channels.count() and passwords.count() account for the commas
            if (length + currentLength + 6 + channels.count() + passwords.count() >= 512) // 6: "JOIN " plus separating space between chans and pws.
            {
                while (!passwords.isEmpty() && passwords.last() == QLatin1Char('.')) passwords.pop_back();

                joinCommands << QStringLiteral("JOIN ") + channels.join(QLatin1Char(',')) + QLatin1Char(' ') + passwords.join(QLatin1Char(','));

                channels.clear();
                passwords.clear();

                length = 0;
            }

            length += currentLength;

            channels << channelName;
            passwords << password;
        }
    }

    while (!passwords.isEmpty() && passwords.last() == QLatin1Char('.')) passwords.pop_back();

    // Even if the given tmpList contained entries they might have been filtered
    // out by the isAChannel() check.
    if (!channels.isEmpty())
    {
        joinCommands << QStringLiteral("JOIN ") + channels.join(QLatin1Char(',')) + QLatin1Char(' ') + passwords.join(QLatin1Char(','));
    }

    return joinCommands;
}

ViewContainer* Server::getViewContainer() const
{
    Application* konvApp = Application::instance();
    return konvApp->getMainWindow()->getViewContainer();
}


bool Server::getUseSSL() const
{
        if ( m_socket )
            return ( m_socket->mode() != QSslSocket::UnencryptedMode );
        else
            return false;
}


QString Server::getSSLInfo() const
{
//     SSLSocket* sslsocket = dynamic_cast<SSLSocket*>(m_socket);

//     if(sslsocket)
//         return sslsocket->details();

    return QString();
}


void Server::sendMultiServerCommand(const QString& command, const QString& parameter)
{
    Q_EMIT multiServerCommand(command, parameter);
}

void Server::executeMultiServerCommand(const QString& command, const QString& parameter)
{
    if (command == QLatin1String("msg"))
        sendToAllChannelsAndQueries(parameter);
    else
        sendToAllChannelsAndQueries(Preferences::self()->commandChar() + command + QLatin1Char(' ') + parameter);
}

void Server::sendToAllChannelsAndQueries(const QString& text)
{
    // Send a message to all channels we are in
    for (Channel* channel : std::as_const(m_channelList)) {
        channel->sendText(text);
    }

    // Send a message to all queries we are in
    for (Query* query : std::as_const(m_queryList)) {
        query->sendText(text);
    }
}

void Server::requestAway(const QString& reason)
{
    QString awayReason = reason;

    IdentityPtr identity = getIdentity();

    if (awayReason.isEmpty() && identity)
        awayReason = identity->getAwayMessage();

    // Fallback in case the identity has no away message set.
    if (awayReason.isEmpty())
        awayReason = i18n("Gone away for now");

    setAwayReason(awayReason);

    queue(QStringLiteral("AWAY :") + awayReason);
}

void Server::requestUnaway()
{
    queue(QStringLiteral("AWAY"));
}

void Server::setAway(bool away, const QHash<QString, QString> &messageTags)
{
    IdentityPtr identity = getIdentity();

    if (away)
    {
        if (!m_away) startAwayTimer();

        m_away = true;

        Q_EMIT awayState(true);

        if (identity && !identity->getAwayNickname().isEmpty() && identity->getAwayNickname() != getNickname())
        {
            m_nonAwayNick = getNickname();
            queue(QStringLiteral("NICK ") + getIdentity()->getAwayNickname());
        }

        if (!m_awayReason.isEmpty())
            appendMessageToFrontmost(i18n("Away"), i18n("You are now marked as being away (reason: %1).",m_awayReason), messageTags);
        else
           appendMessageToFrontmost(i18n("Away"), i18n("You are now marked as being away."), messageTags);

        if (identity && identity->getRunAwayCommands())
        {
            QString message = identity->getAwayCommand();
            const QRegularExpression re(QStringLiteral("%s"), QRegularExpression::CaseInsensitiveOption);
            sendToAllChannels(message.replace(re, m_awayReason));
        }

        if (identity && identity->getInsertRememberLineOnAway())
            Q_EMIT awayInsertRememberLine(this);
    }
    else
    {
        m_awayReason.clear();

        Q_EMIT awayState(false);

        if (!identity->getAwayNickname().isEmpty() && !m_nonAwayNick.isEmpty())
        {
            queue(QStringLiteral("NICK ") + m_nonAwayNick);
            m_nonAwayNick.clear();
        }

        if (m_away)
        {
            appendMessageToFrontmost(i18n("Away"), i18n("You are no longer marked as being away."), messageTags);

            if (identity && identity->getRunAwayCommands())
            {
                QString message = identity->getReturnCommand();
                const QRegularExpression re(QStringLiteral("%t"), QRegularExpression::CaseInsensitiveOption);
                sendToAllChannels(message.replace(re, awayTime()));
            }
        }
        else
            appendMessageToFrontmost(i18n("Away"), i18n("You are not marked as being away."), messageTags);

        m_away = false;
    }
}

QString Server::awayTime() const
{
    QString retVal;

    if (m_away)
    {
        int diff = QDateTime::currentDateTime().toSecsSinceEpoch() - m_awayTime;
        int num = diff / 3600;

        if (num < 10)
            retVal = QLatin1Char('0') + QString::number(num) + QLatin1Char(':');
        else
            retVal = QString::number(num) + QLatin1Char(':');

        num = (diff % 3600) / 60;

        if (num < 10) retVal += QLatin1Char('0');

        retVal += QString::number(num) + QLatin1Char(':');

        num = (diff % 3600) % 60;

        if (num < 10) retVal += QLatin1Char('0');

        retVal += QString::number(num);
    }
    else
        retVal = QStringLiteral("00:00:00");

    return retVal;
}

void Server::startAwayTimer()
{
    m_awayTime = QDateTime::currentDateTime().toSecsSinceEpoch();
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
    ql << QStringLiteral("PING LAG") + QTime::currentTime().toString(QStringLiteral("hhmmss"));
    if(!m_whoRequestsDisabled)
    {
        getInputFilter()->setAutomaticRequest(QStringLiteral("WHO"), getNickname(), true);
        ql << QStringLiteral("WHO ") + getNickname();
    }
    queueList(ql, HighPriority);

    m_lagTime.start();
    m_inputFilter.setLagMeasuring(true);
    m_pingResponseTimer.start(1000 /*1 sec*/);
}

void Server::pongReceived()
{
    // ignore unrequested PONGs
    if (m_pingSendTimer.isActive())
        return;

    m_currentLag = m_lagTime.elapsed();
    m_inputFilter.setLagMeasuring(false);
    m_pingResponseTimer.stop();

    Q_EMIT serverLag(this, m_currentLag);

    // Send another PING in 60 seconds
    m_pingSendTimer.start(60000 /*60 sec*/);
}

void Server::updateLongPongLag()
{
    if (isSocketConnected())
    {
        m_currentLag = m_lagTime.elapsed();
        Q_EMIT tooLongLag(this, m_currentLag);
        // qCDebug(KONVERSATION_LOG) << "Current lag: " << currentLag;

        if (m_currentLag > (Preferences::self()->maximumLagTime() * 1000))
            m_socket->close();
    }
}

void Server::updateEncoding()
{
    if(getViewContainer() && getViewContainer()->getFrontView())
        getViewContainer()->updateViewEncoding(getViewContainer()->getFrontView());
}

#if HAVE_QCA2
void Server::initKeyExchange(const QString &receiver)
{
    Query* query;
    if (getQueryByName(receiver))
    {
        query = getQueryByName(receiver);
    }
    else
    {
        NickInfoPtr nickinfo = obtainNickInfo(receiver);
        query = addQuery(nickinfo, true);
    }

    Konversation::Cipher* cipher = query->getCipher();

    QByteArray pubKey = cipher->initKeyExchange();
    if(pubKey.isEmpty())
    {
        appendMessageToFrontmost(i18n("Error"), i18n("Failed to initiate key exchange with %1.", receiver));
    }
    else
    {
        queue(QLatin1String("NOTICE ") + receiver + QLatin1String(" :DH1080_INIT ") + QLatin1String(pubKey));
    }
}

void Server::parseInitKeyX(const QString &sender, const QString &remoteKey)
{
    if (!Konversation::Cipher::isFeatureAvailable(Konversation::Cipher::DH))
    {
        appendMessageToFrontmost(i18n("Error"), i18n("Unable to perform key exchange with %1.", sender)
            + QLatin1Char(' ') + Konversation::Cipher::runtimeError());

        return;
    }

    //TODO ask the user to accept without blocking
    Query* query;
    if (getQueryByName(sender))
    {
        query = getQueryByName(sender);
    }
    else
    {
        NickInfoPtr nickinfo = obtainNickInfo(sender);
        query = addQuery(nickinfo, false);
    }

    Konversation::Cipher* cipher = query->getCipher();

    QByteArray pubKey = cipher->parseInitKeyX(remoteKey.toLocal8Bit());

    if (pubKey.isEmpty())
    {
        appendMessageToFrontmost(i18n("Error"), i18n("Failed to parse the DH1080_INIT of %1. Key exchange failed.", sender));
    }
    else
    {
        setKeyForRecipient(sender, cipher->key());
        query->setEncryptedOutput(true);
        appendMessageToFrontmost(i18n("Notice"), i18n("Your key is set and your messages will now be encrypted, sending DH1080_FINISH to %1.", sender));
        queue(QLatin1String("NOTICE ") + sender + QLatin1String(" :DH1080_FINISH ") + QLatin1String(pubKey));
    }
}

void Server::parseFinishKeyX(const QString &sender, const QString &remoteKey)
{
    Query* query;
    if (getQueryByName(sender))
    {
        query = getQueryByName(sender);
    }
    else
        return;

    if (!Konversation::Cipher::isFeatureAvailable(Konversation::Cipher::DH))
    {
        appendMessageToFrontmost(i18n("Error"), i18n("Unable to complete key exchange with %1.", sender)
            + QLatin1Char(' ') + Konversation::Cipher::runtimeError());

        return;
    }

    Konversation::Cipher* cipher = query->getCipher();

    if (cipher->parseFinishKeyX(remoteKey.toLocal8Bit()))
    {
        setKeyForRecipient(sender,cipher->key());
        query->setEncryptedOutput(true);
        appendMessageToFrontmost(i18n("Notice"), i18n("Successfully parsed DH1080_FINISH sent by %1. Your key is set and your messages will now be encrypted.", sender));
    }
    else
    {
        appendMessageToFrontmost(i18n("Error"), i18n("Failed to parse DH1080_FINISH sent by %1. Key exchange failed.", sender));
    }
}
#endif

QAbstractItemModel* Server::nickListModel() const
{
    return m_nickListModel;
}

void Server::startNickInfoChangedTimer()
{
    if(!m_nickInfoChangedTimer->isActive())
        m_nickInfoChangedTimer->start();
}

void Server::sendNickInfoChangedSignals()
{
    Q_EMIT nickInfoChanged();

    for (NickInfoPtr nickInfo : std::as_const(m_allNicks)) {
        if(nickInfo->isChanged())
        {
            Q_EMIT nickInfoChanged(this, nickInfo);
            nickInfo->setChanged(false);
        }
    }
}

void Server::startChannelNickChangedTimer(const QString& channel)
{
    if(!m_channelNickChangedTimer->isActive())
        m_channelNickChangedTimer->start();

    m_changedChannels.append(channel);
}

void Server::sendChannelNickChangedSignals()
{
    for (const QString& channel : std::as_const(m_changedChannels)) {
        if (m_joinedChannels.contains (channel))
        {
            Q_EMIT channelNickChanged(channel);

            for (ChannelNickPtr nick : std::as_const(*m_joinedChannels[channel])) {
                if(nick->isChanged())
                {
                    nick->setChanged(false);
                }
            }
        }
    }

    m_changedChannels.clear();
}

void Server::involuntaryQuit()
{
    if((m_connectionState == Konversation::SSConnected || m_connectionState == Konversation::SSConnecting) &&
       (m_socket->peerAddress() != QHostAddress(QHostAddress::LocalHost) && m_socket->peerAddress() != QHostAddress(QHostAddress::LocalHostIPv6)))
    {
        quitServer();
        updateConnectionState(Konversation::SSInvoluntarilyDisconnected);
    }
}

void Server::reconnectInvoluntary()
{
    if(m_connectionState == Konversation::SSInvoluntarilyDisconnected)
        reconnectServer();
}

void Server::initCapablityNames()
{
    m_capabilityNames = {
        { QStringLiteral("away-notify"),            AwayNotify },
        { QStringLiteral("extended-join"),          ExtendedJoin },
        { QStringLiteral("server-time"),            ServerTime },
        { QStringLiteral("znc.in/server-time-iso"), ServerTime },
        { QStringLiteral("userhost-in-names"),      UserHostInNames },
        { QStringLiteral("sasl"),                   SASL },
        { QStringLiteral("multi-prefix"),           MultiPrefix },
        { QStringLiteral("account-notify"),         AccountNotify },
        { QStringLiteral("znc.in/self-message"),    SelfMessage },
        { QStringLiteral("chghost"),                ChgHost },
        { QStringLiteral("cap-notify"),             CapNofify },
    };
}

#include "moc_server.cpp"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
