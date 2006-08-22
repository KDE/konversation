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

#include <kapplication.h>
#include <kdebug.h>
#include <dcopclient.h>

#include <qstring.h>
#include <klocale.h>

#include "konversationapplication.h"
#include "channel.h"
#include "konvdcop.h"
#include "identity.h"
#include "server.h"

KonvDCOP::KonvDCOP()
: DCOPObject("Konversation"),
QObject(0,"Konversation")
{
    // reset hook counter

    KConfig *config = KGlobal::config();
    config->setGroup("AutoAway");                 //TODO - add this to preferences somewhere

    if (config->readBoolEntry("UseAutoAway", true))
    {
        //disable for now until auto-available is working, and we prevent the user from changing nick _and_ setting away message
        //    connectDCOPSignal("kdesktop", "KScreensaverIface",
        //                      "KDE_start_screensaver()", "setAutoAway()", false);
    }
    else
    {
        disconnectDCOPSignal("kdesktop", "KScreensaverIface",
            "KDE_start_screensaver()", "setAutoAway()");
    }
}

void KonvDCOP::raw(const QString& server,const QString& command)
{
    kdDebug() << "KonvDCOP::raw()" << endl;
    // send raw IRC protocol data
    emit dcopRaw(server,command);
}

QStringList KonvDCOP::listServers()
{
    QStringList serverlist;
    QPtrList<Server>servers = static_cast<KonversationApplication *>(kapp)->getServerList();
    Server *server;
    for ( server = servers.first(); server; server = servers.next() )
        serverlist.append(server->getServerName());
    return serverlist;
}

QStringList KonvDCOP::listConnectedServers()
{
    QStringList serverlist;
    QPtrList<Server>servers = static_cast<KonversationApplication *>(kapp)->getServerList();
    Server *server;
    for ( server = servers.first(); server; server = servers.next() )
        if(server->connected())
            serverlist.append(server->getServerName());
    return serverlist;
}

void KonvDCOP::setAway(const QString &awaymessage)
{
    if(awaymessage.isEmpty())
                                                  //away messages can't be empty.
        emit dcopMultiServerRaw("away " + i18n("Gone away for now."));
    else
        emit dcopMultiServerRaw("away " + awaymessage);
}

void KonvDCOP::setAutoAway()
{
    kdDebug() << "set auto away" << endl;
    emit dcopSetAutoAway();
}

void KonvDCOP::setBack()
{
    emit dcopMultiServerRaw("away");
}

void KonvDCOP::sayToAll(const QString &message)
{
    emit dcopMultiServerRaw("msg " + message);
}

void KonvDCOP::actionToAll(const QString &message)
{
    emit dcopMultiServerRaw("me " + message);
}

void KonvDCOP::say(const QString& server,const QString& target,const QString& command)
{
    kdDebug() << "KonvDCOP::say()" << endl;
    // Act as if the user typed it
    emit dcopSay(server,target,command);
}

void KonvDCOP::info(const QString& string)
{
    kdDebug() << "KonvDCOP::info()" << endl;
    emit dcopInfo(string);
}

void KonvDCOP::debug(const QString& string)
{
    kdDebug() << "KonvDCOP::debug()" << endl;
    emit dcopInfo(QString("Debug: %1").arg(string));
}

void KonvDCOP::error(const QString& string)
{
    kdDebug() << "KonvDCOP::error()" << endl;
    emit dcopInfo(QString("Error: %1").arg(string));
}

void KonvDCOP::insertRememberLine()
{
    emit dcopInsertRememberLine();
}

void KonvDCOP::connectToServer(const QString& url, int port, const QString& channel, const QString& password)
{
    emit dcopConnectToServer(url, port, channel, password);
}

QString KonvDCOP::getNickname (const QString &serverName)
{
    Server* server = static_cast<KonversationApplication *>(kapp)->getServerByName(serverName);
    if ( !server )
    {
        error( i18n( "getNickname: Server %1 is not found." ).arg( serverName ) );
        return QString();
    }
    return server->getNickname();
}

QString KonvDCOP::getAnyNickname ()
{
    const QPtrList<Server>serverlist = static_cast<KonversationApplication *>(kapp)->getServerList();
    Server *server = serverlist.getFirst();
    return server->getNickname();
}

QString KonvDCOP::getChannelEncoding(const QString& server, const QString& channel)
{
    return Preferences::channelEncoding(server,channel);
}

// Identity stuff
KonvIdentDCOP::KonvIdentDCOP()
: DCOPObject("KonvDCOPIdentity"),
QObject(0, "KonvDCOPIdentity")
{
}

QStringList KonvIdentDCOP::listIdentities()
{
    QStringList identities;
    QValueList<IdentityPtr> ids = Preferences::identityList();
    for(QValueList<IdentityPtr>::iterator it = ids.begin(); it != ids.end(); ++it)
    {
        identities.append((*it)->getName());
    }
    return identities;
}

void KonvIdentDCOP::setrealName(const QString &id_name, const QString& name)
{
    QValueList<IdentityPtr> ids = Preferences::identityList();

    for(QValueList<IdentityPtr>::iterator it = ids.begin(); it != ids.end(); ++it)
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
    QValueList<IdentityPtr> ids = Preferences::identityList();

    for(QValueList<IdentityPtr>::iterator it = ids.begin(); it != ids.end(); ++it)
    {
        if ((*it)->getName() == id_name)
        {
            return (*it)->getRealName();
        }
    }

    return QString::null;
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
