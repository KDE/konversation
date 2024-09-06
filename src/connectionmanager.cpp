/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
*/

#include "connectionmanager.h"

#include "connectionsettings.h"
#include "serversettings.h"
#include "servergroupsettings.h"
#include "config/preferences.h"
#include "application.h"
#include "mainwindow.h"
#include "statuspanel.h"
#include "konversation_log.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QRegularExpression>


ConnectionManager::ConnectionManager(QObject* parent)
    : QObject(parent), m_overrideAutoReconnect (false)
{
// Reenable the check again when it works reliably for all backends
//    if (Solid::Networking::status() != Solid::Networking::Connected)
//        m_overrideAutoReconnect = true;

    connect(this, &ConnectionManager::requestReconnect, this, &ConnectionManager::handleReconnect);
}

ConnectionManager::~ConnectionManager()
{
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, const QString& target,
    const QString& port, const QString& password, const QString& nick, const QString& channel,
    bool useSSL)
{
    connectTo(flag, ConnectionSettings(target, port, password, nick, channel, useSSL));
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, int serverGroupId)
{
    ConnectionSettings settings;

    Konversation::ServerGroupSettingsPtr serverGroup;

    serverGroup = Preferences::serverGroupById(serverGroupId);

    if (serverGroup)
    {
        settings.setServerGroup(serverGroup);

        if (!serverGroup->serverList().isEmpty())
            settings.setServer(serverGroup->serverList()[0]);
    }

    connectTo(flag, settings);
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, const QList<QUrl>& list)
{
    QMap<QString,Konversation::ChannelList> serverChannels;
    QMap<QString,ConnectionSettings> serverConnections;

    for (const QUrl& url : list) {
        ConnectionSettings settings(url.url());

        qCDebug(KONVERSATION_LOG) << settings.name() << " - "
                 << settings.server().host() << settings.server().port()
                 << settings.server().password() << " - "
                 << (settings.serverGroup()?settings.serverGroup()->name():QString());

        QString sname = (settings.serverGroup() ? settings.serverGroup()->name()
            : (settings.server().host() + QLatin1Char(':') + QString::number(settings.server().port())));

        if (!serverChannels.contains(sname))
            serverConnections[sname] = settings;

        serverChannels[sname] += settings.oneShotChannelList();
    }

    // Perform the connection.
    QMap<QString,Konversation::ChannelList>::ConstIterator s_i = serverChannels.constBegin();

    for (; s_i != serverChannels.constEnd(); ++s_i)
    {
        serverConnections[s_i.key()].setOneShotChannelList(s_i.value());
        connectTo(flag, serverConnections[s_i.key()]);
    }
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, ConnectionSettings settings)
{
    if (!settings.isValid()) return;

    Q_EMIT closeServerList();

    if (flag != Konversation::CreateNewConnection
        && reuseExistingConnection(settings, (flag == Konversation::PromptToReuseConnection)))
    {
        return;
    }

    IdentityPtr identity = settings.identity();

    if (!identity || !validateIdentity(identity)) return;

    Application* konvApp = Application::instance();
    MainWindow* mainWindow = konvApp->getMainWindow();

    auto* server = new Server(this, settings);

    enlistConnection(server->connectionId(), server);

    connect(server, &Server::destroyed, this, &ConnectionManager::delistConnection);

    connect(server, &Server::connectionStateChanged,
            this, &ConnectionManager::handleConnectionStateChange);

    connect(server, &Server::awayState, this, &ConnectionManager::connectionChangedAwayState);

    connect(server, &Server::nicksNowOnline,
        mainWindow, &MainWindow::setOnlineList);
    connect(server, &Server::awayInsertRememberLine,
        mainWindow, &MainWindow::triggerRememberLines);

    connect(server, &Server::multiServerCommand,
        konvApp, &Application::sendMultiServerCommand);
}

void ConnectionManager::enlistConnection(int connectionId, Server* server)
{
    m_connectionList.insert(connectionId, server);

    Q_EMIT connectionListChanged();
}

void ConnectionManager::delistConnection(int connectionId)
{
    m_connectionList.remove(connectionId);

    Q_EMIT connectionListChanged();
}

void ConnectionManager::handleConnectionStateChange(Server* server, Konversation::ConnectionState state)
{
    Q_EMIT connectionChangedState(server, state);

    int identityId = server->getIdentity()->id();

    if (state == Konversation::SSConnected)
    {
        m_overrideAutoReconnect = false;

        if (!m_activeIdentities.contains(identityId))
        {
            m_activeIdentities.insert(identityId);

            Q_EMIT identityOnline(identityId);
        }
    }
    else if (state != Konversation::SSConnecting)
    {
        if (m_activeIdentities.contains(identityId))
        {
            m_activeIdentities.remove(identityId);

            Q_EMIT identityOffline(identityId);
        }
    }

    if (state == Konversation::SSInvoluntarilyDisconnected && !m_overrideAutoReconnect)
    {
        // The asynchronous invocation of handleReconnect() makes sure that
        // connectionChangedState() is emitted and delivered before it runs
        // (and causes the next connection state change to occur).
        Q_EMIT requestReconnect(server);
    }
    else if (state == Konversation::SSInvoluntarilyDisconnected && m_overrideAutoReconnect)
    {
        server->getStatusView()->appendServerMessage(i18n("Info"), i18n ("Network is down, will reconnect automatically when it is back up."));
    }
}

void ConnectionManager::handleReconnect(Server* server)
{
    if (!Preferences::self()->autoReconnect() || m_overrideAutoReconnect) return;

    ConnectionSettings settings = server->getConnectionSettings();

    uint reconnectCount = Preferences::self()->reconnectCount();

    // For server groups, one iteration over their server list shall count as one
    // connection attempt.
    if (settings.serverGroup())
        reconnectCount = reconnectCount * settings.serverGroup()->serverList().size();

    if (reconnectCount == 0 || settings.reconnectCount() < reconnectCount)
    {
        if (settings.serverGroup() && settings.serverGroup()->serverList().size() > 1)
        {
            Konversation::ServerList serverList = settings.serverGroup()->serverList();

            int index = serverList.indexOf(settings.server());
            int size = serverList.size();

            if (index == size - 1 || index == -1)
                settings.setServer(serverList[0]);
            else if (index < size - 1)
                settings.setServer(serverList[index+1]);

            server->setConnectionSettings(settings);

            server->getStatusView()->appendServerMessage(i18n("Info"),
                i18np(
                 "Trying to connect to %2 (port %3) in 1 second.",
                 "Trying to connect to %2 (port %3) in %1 seconds.",
                 Preferences::self()->reconnectDelay(),
                 settings.server().host(),
                 QString::number(settings.server().port())));
        }
        else
        {
            server->getStatusView()->appendServerMessage(i18n("Info"),
                i18np(
                 "Trying to reconnect to %2 (port %3) in 1 second.",
                 "Trying to reconnect to %2 (port %3) in %1 seconds.",
                 Preferences::self()->reconnectDelay(),
                 settings.server().host(),
                 QString::number(settings.server().port())));
        }

        server->getConnectionSettings().incrementReconnectCount();
        server->connectToIRCServerIn(Preferences::self()->reconnectDelay());
    }
    else
    {
        server->getConnectionSettings().setReconnectCount(0);
        server->getStatusView()->appendServerMessage(i18n("Error"), i18n("Reconnection attempts exceeded."));
    }
}

void ConnectionManager::quitServers()
{
    for (Server* server : std::as_const(m_connectionList)) {
        server->quitServer();
    }
}

void ConnectionManager::reconnectServers()
{
    for (Server* server : std::as_const(m_connectionList)) {
        server->reconnectServer();
    }
}

bool ConnectionManager::reuseExistingConnection(ConnectionSettings& settings, bool interactive)
{
    Server* dupe = nullptr;
    ConnectionDupe dupeType;
    bool doReuse = true;

    Application* konvApp = Application::instance();
    MainWindow* mainWindow = konvApp->getMainWindow();

    for (Server* server : std::as_const(m_connectionList)) {
        if (server->getServerGroup() && settings.serverGroup()
            && server->getServerGroup() == settings.serverGroup())
        {
            dupe = server;
            dupeType = SameServerGroup;

            break;
        }
    }

    if (!dupe)
    {
        for (Server* server : std::as_const(m_connectionList)) {
            if (server->getConnectionSettings().server() == settings.server()) {
                dupe = server;
                dupeType = SameServer;

                break;
            }
        }
    }

    if (dupe && interactive)
    {
        int result = KMessageBox::warningContinueCancel(
            mainWindow,
            i18n("You are already connected to %1. Do you want to open another connection?", dupe->getDisplayName()),
            i18n("Already connected to %1", dupe->getDisplayName()),
            KGuiItem(i18n("Create connection")),
            KStandardGuiItem::cancel(),
            QStringLiteral("ReuseExistingConnection"));

        if (result == KMessageBox::Continue) doReuse = false;
    }

    if (dupe && doReuse)
    {
        if (interactive && dupeType == SameServerGroup
            && !(dupe->getConnectionSettings().server() == settings.server()))
        {
            int result = KMessageBox::warningContinueCancel(
                mainWindow,
                i18n("You are presently connected to %1 via '%2' (port %3). Do you want to switch to '%4' (port %5) instead?",
                    dupe->getDisplayName(),
                    dupe->getServerName(),
                    QString::number(dupe->getPort()),
                    settings.server().host(),
                    QString::number(settings.server().port())),
                i18n("Already connected to %1", dupe->getDisplayName()),
                KGuiItem(i18n("Switch Server")),
                KStandardGuiItem::cancel(),
                QStringLiteral("ReconnectWithDifferentServer"));

            if (result == KMessageBox::Continue)
            {
                dupe->disconnectServer();

                dupe->setConnectionSettings(settings);
            }
        }

        if (!dupe->isConnected())
        {
            if (!settings.oneShotChannelList().isEmpty())
                dupe->updateAutoJoin(settings.oneShotChannelList());

            if (!dupe->isConnecting())
                dupe->reconnectServer();
        }
        else
        {
            const auto oneShotChannelList = settings.oneShotChannelList();
            if (!oneShotChannelList.isEmpty()) {
                for (const auto& channel : oneShotChannelList) {
                    dupe->sendJoinCommand(channel.name(), channel.password());
                }
                settings.clearOneShotChannelList();
            }
        }
    }

    return (dupe && doReuse);
}

bool ConnectionManager::validateIdentity(IdentityPtr identity, bool interactive)
{
    Application* konvApp = Application::instance();
    MainWindow* mainWindow = konvApp->getMainWindow();

    QString errors;

    if (identity->getIdent().isEmpty())
        errors+=i18n("Please fill in your <b>Ident</b>.<br/>");

    if (identity->getRealName().isEmpty())
        errors+=i18n("Please fill in your <b>Real name</b>.<br/>");

    if (identity->getNickname(0).isEmpty())
        errors+=i18n("Please provide at least one <b>Nickname</b>.<br/>");

    if (!errors.isEmpty())
    {
        if (interactive)
        {
            int result = KMessageBox::warningContinueCancel(
                    mainWindow,
                    i18n("<qt>Your identity \"%1\" is not set up correctly:<br/>%2</qt>", identity->getName(), errors),
                    i18n("Identity Settings"),
                    KGuiItem(i18n("Edit Identity..."), QStringLiteral("document-edit")),
                    KStandardGuiItem::cancel());

            if (result == KMessageBox::Continue)
            {
                identity = mainWindow->editIdentity(identity);

                if (identity && validateIdentity(identity, false))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        return false;
    }

    return true;
}

QList<Server*> ConnectionManager::getServerList() const
{
    QList<Server*> serverList;

    serverList.reserve(m_connectionList.size());
    for (Server* server : std::as_const(m_connectionList)) {
        serverList.append(server);
    }

    return serverList;
}

Server* ConnectionManager::getServerByConnectionId(int connectionId) const
{
    if (m_connectionList.contains(connectionId))
        return m_connectionList[connectionId];
    else
        return nullptr;
}

Server* ConnectionManager::getServerByName(const QString& name, NameMatchFlags flags) const
{
    if (flags == MatchByIdThenName)
    {
        bool conversion = false;
        const int connectionId = name.toInt(&conversion);

        if (conversion)
        {
            Server* const server = this->getServerByConnectionId(connectionId);
            if (server)
                return server;
        }
    }

    for (Server* server : std::as_const(m_connectionList)) {
        if (server->getDisplayName() == name || server->getServerName() == name) {
            return server;
        }
    }

    return nullptr;
}

void ConnectionManager::involuntaryQuitServers()
{
    m_overrideAutoReconnect = true;

    for (Server* server : std::as_const(m_connectionList)) {
        server->involuntaryQuit();
    }
}

void ConnectionManager::reconnectInvoluntary()
{
    m_overrideAutoReconnect = false;

    for (Server* server : std::as_const(m_connectionList)) {
        server->reconnectInvoluntary();
    }
}

void ConnectionManager::onOnlineStateChanged(QNetworkInformation::Reachability reachability)
{
    if (reachability == QNetworkInformation::Reachability::Online) {
        reconnectInvoluntary();
    } else {

        involuntaryQuitServers();
    }
}

#include "moc_connectionmanager.cpp"
