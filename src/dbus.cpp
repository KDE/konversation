/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Alex Zepeda <zipzippy@sonic.net>
*/

#include "dbus.h"
#include "application.h"
#include "connectionmanager.h"
#include "awaymanager.h"
#include "channel.h"
#include "identity.h"
#include "server.h"
#include "konversation_log.h"

#include <KLocalizedString>

#include <QDBusConnection>

using namespace Konversation;


DBus::DBus(QObject *parent) : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.connect(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("ActiveChanged"), this, SLOT(changeAwayStatus(bool)));
}

void DBus::raw(const QString& server,const QString& command)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    // send raw IRC protocol data
    Q_EMIT dbusRaw(sterilizeUnicode(server), sterilizeUnicode(command));
}

QStringList DBus::listConnections()
{
    Application* konvApp = Application::instance();

    QStringList connections;
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();

    connections.reserve(serverList.size());
    for (Server* server : serverList)
        connections << QString::number(server->connectionId());

    return connections;
}

QStringList DBus::listServers()
{
    Application* konvApp = Application::instance();

    QStringList hosts;
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();

    for (Server* server : serverList)
        if (server) hosts << server->getServerName();

    return hosts;
}

QStringList DBus::listConnectedServers()
{
    Application* konvApp = Application::instance();

    QStringList connectedHosts;
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();

    for (Server* server : serverList)
        if (server && server->isConnected()) connectedHosts << server->getServerName();

    return connectedHosts;
}

QStringList DBus::listJoinedChannels(const QString& serverName)
{
    QStringList joinedChannels;

    ConnectionManager* connectionManager = Application::instance()->getConnectionManager();

    Server* server = connectionManager->getServerByName(serverName, ConnectionManager::MatchByIdThenName);

    if (server)
    {
        const QList<Channel*>& channelList = server->getChannelList();

        joinedChannels.reserve(channelList.size());

        for (Channel* channel : channelList) {
            if (channel->isJoined())
                joinedChannels.append(channel->getName());
        }
    }

    return joinedChannels;
}

void DBus::setAway(const QString& awaymessage)
{
    Application::instance()->getAwayManager()->requestAllAway(sterilizeUnicode(awaymessage));
}

void DBus::setBack()
{
    Application::instance()->getAwayManager()->requestAllUnaway();
}

void DBus::sayToAll(const QString &message)
{
    Q_EMIT dbusMultiServerRaw(QStringLiteral("msg ") + sterilizeUnicode(message));
}

void DBus::actionToAll(const QString &message)
{
    Q_EMIT dbusMultiServerRaw(QStringLiteral("me ") + sterilizeUnicode(message));
}

void DBus::say(const QString& _server,const QString& _target,const QString& _command)
{
    //Sadly, copy on write doesn't exist with QString::replace
    QString server(sterilizeUnicode(_server)), target(sterilizeUnicode(_target)), command(sterilizeUnicode(_command));

    // TODO: this just masks a greater problem - Server::addQuery will return a query for '' --argonel
    // TODO: other DCOP calls need argument checking too --argonel
    if (server.isEmpty() || target.isEmpty() || command.isEmpty())
        qCDebug(KONVERSATION_LOG) <<  "DBus::say() requires 3 arguments.";
    else
    {
        command.replace(QLatin1Char('\n'),QStringLiteral("\\n"));
        command.replace(QLatin1Char('\r'),QStringLiteral("\\r"));
        target.remove(QLatin1Char('\n'));
        target.remove(QLatin1Char('\r'));
        server.remove(QLatin1Char('\n'));
        server.remove(QLatin1Char('\r'));
        // Act as if the user typed it
        Q_EMIT dbusSay(server,target,command);
    }
}

void DBus::info(const QString& string)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    Q_EMIT dbusInfo(sterilizeUnicode(string));
}

void DBus::debug(const QString& string)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    Q_EMIT dbusInfo(i18n("Debug: %1", sterilizeUnicode(string)));
}

void DBus::error(const QString& string)
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
    Q_EMIT dbusInfo(i18n("Error: %1", sterilizeUnicode(string)));
}

void DBus::insertMarkerLine()
{
    Q_EMIT dbusInsertMarkerLine();
}

void DBus::connectToServer(const QString& address, int port, const QString& channel, const QString& password)
{
    Q_EMIT connectTo(Konversation::SilentlyReuseConnection, sterilizeUnicode(address), QString::number(port), sterilizeUnicode(password), QString(), sterilizeUnicode(channel));
}

QString DBus::getNickname(const QString& server_Name)
{
    QString serverName(sterilizeUnicode(server_Name));
    Server* server = Application::instance()->getConnectionManager()->getServerByName(serverName);

    if (!server)
    {
        error( i18n( "getNickname: Server %1 is not found.",  serverName ) );
        return QString();
    }

    return server->getNickname();
}

QString DBus::getChannelEncoding(const QString& server, const QString& channel)
{
    return Preferences::channelEncoding(sterilizeUnicode(server), sterilizeUnicode(channel));
}

void DBus::changeAwayStatus(bool away)
{
    Application* konvApp = Application::instance();

    if (away)
    {
        konvApp->getAwayManager()->setManagedIdentitiesAway();
    }
    else
    {
        konvApp->getAwayManager()->setManagedIdentitiesUnaway();

        // Simulate user activity so the whole idle time calculation
        // logic is being restarted. This is needed as a DBus call
        // could be made without any user activity involved. Simulating
        // user activity then correctly causes the idle-time calculation
        // to be restarted completely (which means the user will only
        // get marked as "auto-away" if the configured idle-timeout has
        // expired).
        konvApp->getAwayManager()->resetUserActivity();
    }
}


// Identity stuff
IdentDBus::IdentDBus(QObject *parent)
: QObject(parent)
{
}

QStringList IdentDBus::listIdentities()
{
    QStringList identities;

    const IdentityList ids = Preferences::identityList();
    identities.reserve(ids.size());

    for (const auto& id : ids) {
        identities.append(id->getName());
    }

    sterilizeUnicode(identities);
    return identities;
}

void IdentDBus::setrealName(const QString &_id_name, const QString& name)
{
    QString id_name(sterilizeUnicode(_id_name));

    const IdentityList ids = Preferences::identityList();

    for (const auto& id : ids) {
        if (id->getName() == id_name) {
            id->setRealName(sterilizeUnicode(name));
            return;
        }
    }

}

QString IdentDBus::getrealName(const QString &_id_name)
{
    QString id_name(sterilizeUnicode(_id_name));

    const IdentityList ids = Preferences::identityList();

    for (const auto& id : ids) {
        if (id->getName() == id_name) {
            return sterilizeUnicode(id->getRealName());
        }
    }

    return QString();
}

void IdentDBus::setIdent(const QString &/*identity*/, const QString& /*ident*/)
{
    //Preferences::identityByName(identity)->.setIdent(;
}

QString IdentDBus::getIdent(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getIdent());
}

void IdentDBus::setNickname(const QString &identity, int index,const QString& nick)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setNickname(index, sterilizeUnicode(nick));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getNickname(const QString &identity, int index)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getNickname(index));
}

void IdentDBus::setBot(const QString &identity, const QString& bot)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setNickservNickname(sterilizeUnicode(bot));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getBot(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getNickservNickname());
}

void IdentDBus::setPassword(const QString &identity, const QString& password)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setAuthPassword(sterilizeUnicode(password));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getPassword(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getAuthPassword());
}

void IdentDBus::setNicknameList(const QString &identity, const QStringList& newList)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setNicknameList(sterilizeUnicode(newList));
    Application::instance()->saveOptions(true);
}

QStringList IdentDBus::getNicknameList(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getNicknameList());
}

void IdentDBus::setQuitReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setQuitReason(sterilizeUnicode(reason));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getQuitReason(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getQuitReason());
}


void IdentDBus::setPartReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setPartReason(sterilizeUnicode(reason));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getPartReason(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getPartReason());
}

void IdentDBus::setKickReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setKickReason(sterilizeUnicode(reason));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getKickReason(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getKickReason());
}

void IdentDBus::setRunAwayCommands(const QString &identity, bool run)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setRunAwayCommands(run);
    Application::instance()->saveOptions(true);
}

bool IdentDBus::getRunAwayCommands(const QString &identity)
{
    return Preferences::identityByName(sterilizeUnicode(identity))->getRunAwayCommands();
}

void IdentDBus::setAwayCommand(const QString &identity, const QString& command)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setAwayCommand(sterilizeUnicode(command));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getAwayCommand(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(identity)->getAwayCommand());
}

void IdentDBus::setReturnCommand(const QString &identity, const QString& command)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setReturnCommand(sterilizeUnicode(command));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getReturnCommand(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getReturnCommand());
}

void IdentDBus::setAwayMessage(const QString &identity, const QString& message)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setAwayMessage(sterilizeUnicode(message));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getAwayMessage(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getAwayMessage());
}

void IdentDBus::setAwayNickname(const QString &identity, const QString& nickname)
{
    const Identity *i = Preferences::identityByName(sterilizeUnicode(identity)).data();
    const_cast<Identity *>(i)->setAwayNickname(sterilizeUnicode(nickname));
    Application::instance()->saveOptions(true);
}

QString IdentDBus::getAwayNickname(const QString &identity)
{
    return sterilizeUnicode(Preferences::identityByName(sterilizeUnicode(identity))->getAwayNickname());
}

#include "moc_dbus.cpp"
