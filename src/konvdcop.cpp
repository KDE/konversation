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

#include "konvdcop.h"
#include "konversationapplication.h"
#include "connectionmanager.h"
#include "awaymanager.h"
#include "channel.h"
#include "identity.h"
#include "server.h"

#include <kapplication.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <klocale.h>
//Added by qt3to4:
#include <Q3PtrList>


KonvDCOP::KonvDCOP() : DCOPObject("irc"), QObject(0, "irc")
{
    connectDCOPSignal("kdesktop", "KScreensaverIface", "KDE_start_screensaver()", "setScreenSaverStarted()", false);
    connectDCOPSignal("kdesktop", "KScreensaverIface", "KDE_stop_screensaver()", "setScreenSaverStopped()", false);
}

void KonvDCOP::raw(const QString& server,const QString& command)
{
    kDebug() << "KonvDCOP::raw()" << endl;
    // send raw IRC protocol data
    emit dcopRaw(server,command);
}

QStringList KonvDCOP::listServers()
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    QStringList hosts;
    Q3PtrList<Server> serverList = konvApp->getConnectionManager()->getServerList();
    Server* server;

    for (server = serverList.first(); server; server = serverList.next())
        if (server) hosts << server->getServerName();

    return hosts;
}

QStringList KonvDCOP::listConnectedServers()
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    QStringList connectedHosts;
    Q3PtrList<Server> serverList = konvApp->getConnectionManager()->getServerList();
    Server* server;

    for (server = serverList.first(); server; server = serverList.next())
        if (server && server->isConnected()) connectedHosts << server->getServerName();

    return connectedHosts;
}

void KonvDCOP::setAway(const QString& awaymessage)
{
    static_cast<KonversationApplication*>(kapp)->getAwayManager()->requestAllAway(awaymessage);
}

void KonvDCOP::setBack()
{
    static_cast<KonversationApplication*>(kapp)->getAwayManager()->requestAllUnaway();
}

void KonvDCOP::setScreenSaverStarted()
{
    static_cast<KonversationApplication*>(kapp)->getAwayManager()->setManagedIdentitiesAway();
}

void KonvDCOP::setScreenSaverStopped()
{
    static_cast<KonversationApplication*>(kapp)->getAwayManager()->setManagedIdentitiesUnaway();
}

void KonvDCOP::sayToAll(const QString &message)
{
    emit dcopMultiServerRaw("msg " + message);
}

void KonvDCOP::actionToAll(const QString &message)
{
    emit dcopMultiServerRaw("me " + message);
}

void KonvDCOP::say(const QString& _server,const QString& _target,const QString& _command)
{
    //Sadly, copy on write doesn't exist with QString::replace
    QString server(_server), target(_target), command(_command);

    // TODO: this just masks a greater problem - Server::addQuery will return a query for '' --argonel
    // TODO: other DCOP calls need argument checking too --argonel
    if (server.isEmpty() || target.isEmpty() || command.isEmpty())
        kDebug() <<  "KonvDCOP::say() requires 3 arguments." << endl;
    else
    {
        command.replace('\n',"\\n");
        command.replace('\r',"\\r");
        target.remove('\n');
        target.remove('\r');
        server.remove('\n');
        server.remove('\r');
        // Act as if the user typed it
        emit dcopSay(server,target,command);
    }
}

void KonvDCOP::info(const QString& string)
{
    kDebug() << "KonvDCOP::info()" << endl;
    emit dcopInfo(string);
}

void KonvDCOP::debug(const QString& string)
{
    kDebug() << "KonvDCOP::debug()" << endl;
    emit dcopInfo(QString("Debug: %1").arg(string));
}

void KonvDCOP::error(const QString& string)
{
    kDebug() << "KonvDCOP::error()" << endl;
    emit dcopInfo(QString("Error: %1").arg(string));
}

void KonvDCOP::insertMarkerLine()
{
    emit dcopInsertMarkerLine();
}

void KonvDCOP::connectToServer(const QString& address, int port, const QString& channel, const QString& password)
{
    emit connectTo(Konversation::SilentlyReuseConnection, address, QString::number(port), password, "", channel);
}

QString KonvDCOP::getNickname(const QString& serverName)
{
    Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByName(serverName);

    if (!server)
    {
        error( i18n( "getNickname: Server %1 is not found." ).arg( serverName ) );
        return QString();
    }

    return server->getNickname();
}

QString KonvDCOP::getAnyNickname()
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    Server* server = konvApp->getConnectionManager()->getAnyServer();

    if (server) return server->getNickname();

    return QString();
}

QString KonvDCOP::getChannelEncoding(const QString& server, const QString& channel)
{
    return Preferences::channelEncoding(server,channel);
}

// Identity stuff
KonvIdentDCOP::KonvIdentDCOP()
: DCOPObject("identity"),
QObject(0, "identity")
{
}

QStringList KonvIdentDCOP::listIdentities()
{
    QStringList identities;
    IdentityList ids = Preferences::identityList();
    for(IdentityList::ConstIterator it = ids.begin(); it != ids.end(); ++it)
    {
        identities.append((*it)->getName());
    }
    return identities;
}

void KonvIdentDCOP::setrealName(const QString &id_name, const QString& name)
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

QString KonvIdentDCOP::getrealName(const QString &id_name)
{
    IdentityList ids = Preferences::identityList();

    for(IdentityList::ConstIterator it = ids.begin(); it != ids.end(); ++it)
    {
        if ((*it)->getName() == id_name)
        {
            return (*it)->getRealName();
        }
    }

    return QString();
}

void KonvIdentDCOP::setIdent(const QString &/*identity*/, const QString& /*ident*/)
{
    //Preferences::identityByName(identity)->.setIdent(;
}

QString KonvIdentDCOP::getIdent(const QString &identity)
{
    return Preferences::identityByName(identity)->getIdent();
}

void KonvIdentDCOP::setNickname(const QString &identity, int index,const QString& nick)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setNickname(index, nick);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getNickname(const QString &identity, int index)
{
    return Preferences::identityByName(identity)->getNickname(index);
}

void KonvIdentDCOP::setBot(const QString &identity, const QString& bot)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setBot(bot);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getBot(const QString &identity)
{
    return Preferences::identityByName(identity)->getBot();
}

void KonvIdentDCOP::setPassword(const QString &identity, const QString& password)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setPassword(password);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getPassword(const QString &identity)
{
    return Preferences::identityByName(identity)->getPassword();
}

void KonvIdentDCOP::setNicknameList(const QString &identity, const QStringList& newList)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setNicknameList(newList);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QStringList KonvIdentDCOP::getNicknameList(const QString &identity)
{
    return Preferences::identityByName(identity)->getNicknameList();
}

void KonvIdentDCOP::setQuitReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setQuitReason(reason);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getQuitReason(const QString &identity)
{
    return Preferences::identityByName(identity)->getQuitReason();
}


void KonvIdentDCOP::setPartReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setPartReason(reason);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getPartReason(const QString &identity)
{
    return Preferences::identityByName(identity)->getPartReason();
}

void KonvIdentDCOP::setKickReason(const QString &identity, const QString& reason)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setKickReason(reason);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getKickReason(const QString &identity)
{
    return Preferences::identityByName(identity)->getKickReason();
}

void KonvIdentDCOP::setShowAwayMessage(const QString &identity, bool state)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setShowAwayMessage(state);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

bool KonvIdentDCOP::getShowAwayMessage(const QString &identity)
{
    return Preferences::identityByName(identity)->getShowAwayMessage();
}

void KonvIdentDCOP::setAwayMessage(const QString &identity, const QString& message)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setAwayMessage(message);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getAwayMessage(const QString &identity)
{
    const QString f = Preferences::identityByName(identity)->getAwayMessage();
    return f;
}

void KonvIdentDCOP::setReturnMessage(const QString &identity, const QString& message)
{
    const Identity *i = Preferences::identityByName(identity);
    const_cast<Identity *>(i)->setReturnMessage(message);
    static_cast<KonversationApplication *>(kapp)->saveOptions(true);
}

QString KonvIdentDCOP::getReturnMessage(const QString &identity)
{
    return Preferences::identityByName(identity)->getReturnMessage();
}

#include "konvdcop.moc"
