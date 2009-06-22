/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The konversation DCOP interface class
  begin:     Mar 7 2003
  copyright: (C) 2003 by Alex Zepeda
  email:     zipzippy@sonic.net
*/

#include "dbus.h"
#include "application.h"
#include "connectionmanager.h"
#include "awaymanager.h"
#include "channel.h"
#include "identity.h"
#include "server.h"

#include <QDBusConnection>

using namespace Konversation;


DBus::DBus(QObject *parent) : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.connect("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", "ActiveChanged", this, SLOT(changeAwayStatus(bool)));
}

void DBus::raw(const QString& server,const QString& command)
{
    kDebug();
    // send raw IRC protocol data
    emit dbusRaw(server,command);
}

QStringList DBus::listServers()
{
    Application* konvApp = static_cast<Application*>(kapp);

    QStringList hosts;
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();

    foreach (Server* server, serverList)
        if (server) hosts << server->getServerName();

    return hosts;
}

QStringList DBus::listConnectedServers()
{
    Application* konvApp = static_cast<Application*>(kapp);

    QStringList connectedHosts;
    const QList<Server*> serverList = konvApp->getConnectionManager()->getServerList();

    foreach (Server* server, serverList)
        if (server && server->isConnected()) connectedHosts << server->getServerName();

    return connectedHosts;
}

void DBus::setAway(const QString& awaymessage)
{
    static_cast<Application*>(kapp)->getAwayManager()->requestAllAway(awaymessage);
}

void DBus::setBack()
{
    static_cast<Application*>(kapp)->getAwayManager()->requestAllUnaway();
}

void DBus::sayToAll(const QString &message)
{
    emit dbusMultiServerRaw("msg " + message);
}

void DBus::actionToAll(const QString &message)
{
    emit dbusMultiServerRaw("me " + message);
}

void DBus::say(const QString& _server,const QString& _target,const QString& _command)
{
    //Sadly, copy on write doesn't exist with QString::replace
    QString server(_server), target(_target), command(_command);

    // TODO: this just masks a greater problem - Server::addQuery will return a query for '' --argonel
    // TODO: other DCOP calls need argument checking too --argonel
    if (server.isEmpty() || target.isEmpty() || command.isEmpty())
        kDebug() <<  "DBus::say() requires 3 arguments.";
    else
    {
        command.replace('\n',"\\n");
        command.replace('\r',"\\r");
        target.remove('\n');
        target.remove('\r');
        server.remove('\n');
        server.remove('\r');
        // Act as if the user typed it
        emit dbusSay(server,target,command);
    }
}

void DBus::info(const QString& string)
{
    kDebug();
    emit dbusInfo(string);
}

void DBus::debug(const QString& string)
{
    kDebug();
    emit dbusInfo(QString("Debug: %1").arg(string));
}

void DBus::error(const QString& string)
{
    kDebug();
    emit dbusInfo(QString("Error: %1").arg(string));
}

void DBus::insertMarkerLine()
{
    emit dbusInsertMarkerLine();
}

void DBus::connectToServer(const QString& address, int port, const QString& channel, const QString& password)
{
    emit connectTo(Konversation::SilentlyReuseConnection, address, QString::number(port), password, "", channel);
}

QString DBus::getNickname(const QString& serverName)
{
    Server* server = Application::instance()->getConnectionManager()->getServerByName(serverName);

    if (!server)
    {
        error( i18n( "getNickname: Server %1 is not found.",  serverName ) );
        return QString();
    }

    return server->getNickname();
}

QString DBus::getAnyNickname()
{
    Application* konvApp = static_cast<Application*>(kapp);

    Server* server = konvApp->getConnectionManager()->getAnyServer();

    if (server) return server->getNickname();

    return QString();
}

QString DBus::getChannelEncoding(const QString& server, const QString& channel)
{
    return Preferences::channelEncoding(server,channel);
}

void DBus::changeAwayStatus(bool away)
{
    Application* konvApp = static_cast<Application*>(kapp);
    
    if (away)
    {
        konvApp->getAwayManager()->setManagedIdentitiesAway();
    }
    else
    {
        konvApp->getAwayManager()->screensaverDisabled();
        konvApp->getAwayManager()->setManagedIdentitiesUnaway();
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
    IdentityList ids = Preferences::identityList();
    for(IdentityList::ConstIterator it = ids.constBegin(); it != ids.constEnd(); ++it)
    {
        identities.append((*it)->getName());
    }
    return identities;
}

void IdentDBus::setrealName(const QString &id_name, const QString& name)
{
    IdentityList ids = Preferences::identityList();

    for(IdentityList::iterator it = ids.begin(); it != ids.end(); ++it)
    {
        if ((*it)->getName() == id_name)
        {
            (*it)->setRealName(name);
            return;
        }
    }

}

QString IdentDBus::getrealName(const QString &id_name)
{
    IdentityList ids = Preferences::identityList();

    for(IdentityList::ConstIterator it = ids.constBegin(); it != ids.constEnd(); ++it)
    {
        if ((*it)->getName() == id_name)
        {
            return (*it)->getRealName();
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
    return Preferences::identityByName(identity)->getIdent();
}

void IdentDBus::setNickname(const QString &identity, int index,const QString& nick)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setNickname(index, nick);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getNickname(const QString &identity, int index)
{
    return Preferences::identityByName(identity)->getNickname(index);
}

void IdentDBus::setBot(const QString &identity, const QString& bot)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setBot(bot);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getBot(const QString &identity)
{
    return Preferences::identityByName(identity)->getBot();
}

void IdentDBus::setPassword(const QString &identity, const QString& password)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setPassword(password);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getPassword(const QString &identity)
{
    return Preferences::identityByName(identity)->getPassword();
}

void IdentDBus::setNicknameList(const QString &identity, const QStringList& newList)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setNicknameList(newList);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QStringList IdentDBus::getNicknameList(const QString &identity)
{
    return Preferences::identityByName(identity)->getNicknameList();
}

void IdentDBus::setQuitReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setQuitReason(reason);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getQuitReason(const QString &identity)
{
    return Preferences::identityByName(identity)->getQuitReason();
}


void IdentDBus::setPartReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setPartReason(reason);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getPartReason(const QString &identity)
{
    return Preferences::identityByName(identity)->getPartReason();
}

void IdentDBus::setKickReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setKickReason(reason);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getKickReason(const QString &identity)
{
    return Preferences::identityByName(identity)->getKickReason();
}

void IdentDBus::setShowAwayMessage(const QString &identity, bool state)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setShowAwayMessage(state);
    static_cast<Application *>(kapp)->saveOptions(true);
}

bool IdentDBus::getShowAwayMessage(const QString &identity)
{
    return Preferences::identityByName(identity)->getShowAwayMessage();
}

void IdentDBus::setAwayMessage(const QString &identity, const QString& message)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setAwayMessage(message);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getAwayMessage(const QString &identity)
{
    const QString f = Preferences::identityByName(identity)->getAwayMessage();
    return f;
}

void IdentDBus::setReturnMessage(const QString &identity, const QString& message)
{
    const Identity *i = Preferences::identityByName(identity).data();
    const_cast<Identity *>(i)->setReturnMessage(message);
    static_cast<Application *>(kapp)->saveOptions(true);
}

QString IdentDBus::getReturnMessage(const QString &identity)
{
    return Preferences::identityByName(identity)->getReturnMessage();
}

#include "dbus.moc"
