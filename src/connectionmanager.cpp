/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2008 Eike Hein <hein@kde.org>
*/

#include "connectionmanager.h"
#include "connectionsettings.h"
#include "serversettings.h"
#include "servergroupsettings.h"
#include "config/preferences.h"
#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "statuspanel.h"

#include <qregexp.h>

#include <klocale.h>
#include <kmessagebox.h>


ConnectionManager::ConnectionManager(QObject* parent) : QObject(parent)
{
    connect(this, SIGNAL(requestReconnect(Server*)), this, SLOT(handleReconnect(Server*)));
}

ConnectionManager::~ConnectionManager()
{
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, const QString& target,
    const QString& port, const QString& password, const QString& nick, const QString& channel,
    bool useSSL)
{
    ConnectionSettings settings;

    if (target.startsWith(QLatin1String("irc://")))
        decodeIrcUrl(target, settings);
    else
    {
        decodeAddress(target, settings);

        Konversation::ServerSettings server = settings.server();

        if (!port.isEmpty()) server.setPort(port.toInt());

        if (!password.isEmpty()) server.setPassword(password);

        if (useSSL) server.setSSLEnabled(true);

        settings.setServer(server);

        if (!nick.isEmpty()) settings.setInitialNick(nick);

        if (!channel.isEmpty())
        {
            Konversation::ChannelSettings channelSettings(channel);

            settings.setInitialChannel(channelSettings);
        }
    }

    connectTo(flag, settings);
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, int serverGroupId)
{
    ConnectionSettings settings;

    Konversation::ServerGroupSettingsPtr serverGroup;

    serverGroup = Preferences::serverGroupById(serverGroupId);

    if (serverGroup)
    {
        settings.setServerGroup(serverGroup);

        if (serverGroup->serverList().size() > 0)
            settings.setServer(serverGroup->serverList()[0]);
    }

    connectTo(flag, settings);
}

void ConnectionManager::connectTo(Konversation::ConnectionFlag flag, ConnectionSettings& settings)
{
    if (!settings.isValid()) return;

    emit closeServerList();

    if (flag != Konversation::CreateNewConnection
        && reuseExistingConnection(settings, (flag == Konversation::PromptToReuseConnection)))
    {
        return;
    }

    IdentityPtr identity = settings.identity();

    if (!identity || !validateIdentity(identity)) return;

    KonversationApplication* konvApp = static_cast<KonversationApplication *>(kapp);
    KonversationMainWindow* mainWindow = konvApp->getMainWindow();

    Server* server = new Server(this, settings);

    enlistConnection(server->connectionId(), server);

    connect(server, SIGNAL(destroyed(int)), this, SLOT(delistConnection(int)));

    connect(server, SIGNAL(connectionStateChanged(Server*, Konversation::ConnectionState)),
            this, SLOT(handleConnectionStateChange(Server*, Konversation::ConnectionState)));

    connect(server, SIGNAL(awayState(bool)), this, SIGNAL(connectionChangedAwayState(bool)));

    connect(server, SIGNAL(nicksNowOnline(Server*, const QStringList&, bool)),
        mainWindow, SLOT(setOnlineList(Server*, const QStringList&,bool)));
    connect(server, SIGNAL(awayInsertRememberLine(Server*)),
        mainWindow, SIGNAL(triggerRememberLines(Server*)));

    connect(mainWindow, SIGNAL(startNotifyTimer(int)), server, SLOT(startNotifyTimer(int)));

    connect(server, SIGNAL(multiServerCommand(const QString&, const QString&)),
        konvApp, SLOT(sendMultiServerCommand(const QString&, const QString&)));
}

void ConnectionManager::enlistConnection(int connectionId, Server* server)
{
    m_connectionList.insert(connectionId, server);
}

void ConnectionManager::delistConnection(int connectionId)
{
    m_connectionList.remove(connectionId);
}

void ConnectionManager::handleConnectionStateChange(Server* server, Konversation::ConnectionState state)
{
    emit connectionChangedState(server, state);

    int identityId = server->getIdentity()->id();

    if (state == Konversation::SSConnected)
    {
        if (!m_activeIdentities.contains(identityId))
        {
            m_activeIdentities.insert(identityId);

            emit identityOnline(identityId);
        }
    }
    else if (state != Konversation::SSConnecting)
    {
        if (m_activeIdentities.contains(identityId))
        {
            m_activeIdentities.remove(identityId);

            emit identityOffline(identityId);
        }
    }

    if (state == Konversation::SSInvoluntarilyDisconnected)
    {
        // The asynchronous invocation of handleReconnect() makes sure that
        // connectionChangedState() is emitted and delivered before it runs
        // (and causes the next connection state change to occur).
        emit requestReconnect(server);
    }
}

void ConnectionManager::handleReconnect(Server* server)
{
    if (!Preferences::self()->autoReconnect()) return;

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
                 "Trying to connect to %2 in 1 second.",
                 "Trying to connect to %2 in %1 seconds.",
                 Preferences::self()->reconnectDelay(),
                 settings.server().host()));
        }
        else
        {
            server->getStatusView()->appendServerMessage(i18n("Info"),
                i18np(
                 "Trying to reconnect to %2 in 1 second.",
                 "Trying to reconnect to %2 in %1 seconds.",
                 Preferences::self()->reconnectDelay(),
                 settings.server().host()));
        }

        server->getConnectionSettings().incrementReconnectCount();

        QTimer::singleShot(Preferences::self()->reconnectDelay() * 1000, server, SLOT(connectToIRCServer()));
    }
    else
        server->getStatusView()->appendServerMessage(i18n("Error"), i18n("Reconnection attempts exceeded."));
}

void ConnectionManager::quitServers()
{
    QMap<int, Server*>::ConstIterator it;

    for (it = m_connectionList.constBegin(); it != m_connectionList.constEnd(); ++it)
        it.value()->quitServer();
}

void ConnectionManager::decodeIrcUrl(const QString& url, ConnectionSettings& settings)
{
    if (!url.startsWith(QLatin1String("irc://"))) return;

    QString mangledUrl = url;

    mangledUrl.remove(QRegExp("^irc:/+"));

    if (mangledUrl.isEmpty()) return;

    // Parsing address and channel.
    QStringList mangledUrlSegments;

    mangledUrlSegments = mangledUrl.split('/', QString::KeepEmptyParts);

    // Check for ",isserver".
    if (mangledUrlSegments[0].contains(','))
    {
        QStringList addressSegments;
        bool checkIfServerGroup = true;

        addressSegments = mangledUrlSegments[0].split(',', QString::KeepEmptyParts);

        if (addressSegments.filter("isserver").size() > 0)
            checkIfServerGroup = false;

        decodeAddress(addressSegments[0], settings, checkIfServerGroup);
    }
    else
        decodeAddress(mangledUrlSegments[0], settings);

    QString channel;
    Konversation::ChannelSettings channelSettings;

    // Grabbing channel from in front of potential ?key=value parameters.
    if (mangledUrlSegments.size() > 1)
        channel = mangledUrlSegments[1].section('?', 0, 0);

    if (!channel.isEmpty())
    {
        // Add default prefix to channel if necessary.
        if (!channel.contains(QRegExp("^[#+&]{1}")))
            channel = '#' + channel;

        channelSettings.setName(channel);
    }

    // Parsing ?key=value parameters.
    QString parameterString;

    if (mangledUrlSegments.size() > 1)
        parameterString = mangledUrlSegments[1].section('?', 1);

    if (parameterString.isEmpty() && mangledUrlSegments.size() > 2)
        parameterString = mangledUrlSegments[2];

    if (!parameterString.isEmpty())
    {
        QRegExp parameterCatcher;

        parameterCatcher.setPattern("pass=([^&]+)");

        if (parameterCatcher.indexIn(parameterString) != -1)
        {
            Konversation::ServerSettings server = settings.server();

            server.setPassword(parameterCatcher.cap(1));

            settings.setServer(server);
        }

        parameterCatcher.setPattern("key=([^&]+)");

        if (parameterCatcher.indexIn(parameterString) != -1)
            channelSettings.setPassword(parameterCatcher.cap(1));
    }

    // Assigning channel.
    if (!channelSettings.name().isEmpty())
        settings.setInitialChannel(channelSettings);
}

void ConnectionManager::decodeAddress(const QString& address, ConnectionSettings& settings,
                                      bool checkIfServerGroup)
{
    QString host;
    QString port = "6667";

    // Full-length IPv6 address with port
    // Example: RFC 2732 notation:     [2001:0DB8:0000:0000:0000:0000:1428:57ab]:6666
    // Example: Non-RFC 2732 notation: 2001:0DB8:0000:0000:0000:0000:1428:57ab:6666
    if (address.contains(':')==8)
    {
        host = address.section(':',0,-2).remove('[').remove(']');
        port = address.section(':',-1);
    }
    // Full-length IPv6 address without port or not-full-length IPv6 address with port
    // Example: Without port, RFC 2732 notation:     [2001:0DB8:0000:0000:0000:0000:1428:57ab]
    // Example: Without port, Non-RFC 2732 notation: 2001:0DB8:0000:0000:0000:0000:1428:57ab
    // Example: With port, RFC 2732 notation:        [2001:0DB8::1428:57ab]:6666
    else if (address.count(QChar(':'))>=4)
    {
        // Last segment does not end with ], but the next to last does;
        // Assume not-full-length IPv6 address with port
        // Example: [2001:0DB8::1428:57ab]:6666
        if (address.section(':',0,-2).endsWith(']') && !address.section(':',-1).endsWith(']'))
        {
            host = address.section(':',0,-2).remove('[').remove(']');
            port = address.section(':',-1);
        }
        else
        {
            QString addressCopy = address;
            host = addressCopy.remove('[').remove(']');
        }
    }
    // IPv4 address or ordinary hostname with port
    // Example: IPv4 address with port: 123.123.123.123:6666
    // Example: Hostname with port:     irc.bla.org:6666
    else if (address.count(':')==1)
    {
        host = address.section(':',0,-2);
        port = address.section(':',-1);
    }
    else
        host = address;

    // Try to assign server group.
    if (checkIfServerGroup && Preferences::isServerGroup(host))
    {
        // If host is found to be the name of a server group.

        int serverGroupId = Preferences::serverGroupIdByName(host);

        Konversation::ServerGroupSettingsPtr serverGroup;

        serverGroup = Preferences::serverGroupById(serverGroupId);

        settings.setServerGroup(serverGroup);

        if (serverGroup->serverList().size() > 0)
            settings.setServer(serverGroup->serverList()[0]);
    }
    else
    {
        if (Preferences::serverGroupByServer(host))
        {
            // If the host is found to be part of a server group's server list.

            Konversation::ServerGroupSettingsPtr serverGroup;

            serverGroup = Preferences::serverGroupByServer(host);

            settings.setServerGroup(serverGroup);
        }

        Konversation::ServerSettings server;

        server.setHost(host);
        server.setPort(port.toInt());

        settings.setServer(server);
    }
}

bool ConnectionManager::reuseExistingConnection(ConnectionSettings& settings, bool interactive)
{
    Server* dupe = 0;
    ConnectionDupe dupeType;
    bool doReuse = true;

    KonversationApplication* konvApp = static_cast<KonversationApplication *>(kapp);
    KonversationMainWindow* mainWindow = konvApp->getMainWindow();

    QMap<int, Server*>::ConstIterator it;

    for (it = m_connectionList.constBegin(); it != m_connectionList.constEnd(); ++it)
    {
        if (it.value()->getServerGroup() && settings.serverGroup()
            && it.value()->getServerGroup() == settings.serverGroup())
        {
            dupe = it.value();
            dupeType = SameServerGroup;

            break;
        }
    }

    if (!dupe)
    {
        for (it = m_connectionList.constBegin(); it != m_connectionList.constEnd(); ++it)
        {
            if (it.value()->getConnectionSettings().server() == settings.server())
            {
                dupe = it.value();
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
            QString("ReuseExistingConnection"));

        if (result == KMessageBox::Continue) doReuse = false;
    }

    if (dupe && doReuse)
    {
        if (interactive && dupeType == SameServerGroup
            && !(dupe->getConnectionSettings().server() == settings.server()))
        {
            int result = KMessageBox::warningContinueCancel(
                mainWindow,
                //my, isn't this fucking ugly
                i18n("You are presently connected to %1 via '%2' (port %3). Do you want to switch to '%4' (port %5) instead?",
                    dupe->getDisplayName(),
                    dupe->getServerName(),
                    dupe->getPort(),
                    settings.server().host(),
                    settings.server().port()),
                i18n("Already connected to %1", dupe->getDisplayName()),
                KGuiItem(i18n("Switch Server")),
                KStandardGuiItem::cancel(),
                "ReconnectWithDifferentServer");

            if (result == KMessageBox::Continue)
            {
                dupe->disconnect();

                dupe->setConnectionSettings(settings);
            }
        }

        if (!dupe->isConnected())
        {
            if (!settings.initialChannel().name().isEmpty())
                dupe->updateAutoJoin(settings.initialChannel());

            if (!dupe->isConnecting())
                dupe->reconnect();
        }
        else
        {
            if (!settings.initialChannel().name().isEmpty())
            {
                dupe->sendJoinCommand(settings.initialChannel().name(),
                                      settings.initialChannel().password());
            }
        }
    }

    return (dupe && doReuse);
}

bool ConnectionManager::validateIdentity(IdentityPtr identity, bool interactive)
{
    KonversationApplication* konvApp = static_cast<KonversationApplication *>(kapp);
    KonversationMainWindow* mainWindow = konvApp->getMainWindow();

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
                    KGuiItem(i18n("Edit Identity...")),
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

    QMap<int, Server*>::ConstIterator it;

    for (it = m_connectionList.constBegin(); it != m_connectionList.constEnd(); ++it)
        serverList.append(it.value());

    return serverList;
}

Server* ConnectionManager::getServerByConnectionId(int connectionId)
{
    if (m_connectionList.contains(connectionId))
        return m_connectionList[connectionId];
    else
        return 0;
}

Server* ConnectionManager::getServerByName(const QString& name)
{
    QMap<int, Server*>::ConstIterator it;

    for (it = m_connectionList.constBegin(); it != m_connectionList.constEnd(); ++it)
    {
        if (it.value()->getDisplayName() == name || it.value()->getServerName() == name)
            return it.value();
    }

    return 0;
}

Server* ConnectionManager::getAnyServer()
{
    if ( m_connectionList.count() > 0)
        return m_connectionList[0];

    return 0;
}

#include "connectionmanager.moc"
