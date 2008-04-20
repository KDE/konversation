/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2007-2007 Eike Hein <hein@kde.org>
*/

#ifndef KONV_IFACE_H
#define KONV_IFACE_H

#include "ignore.h"

#include <qobject.h>
#include <qstringlist.h>

#include <dcopobject.h>


class KonvIface : virtual public DCOPObject
{
    K_DCOP

        k_dcop:
    virtual void setAway(const QString &awaymessage) = 0;
    virtual void setBack() = 0;
    virtual void sayToAll(const QString &message) = 0;
    virtual void actionToAll(const QString &message) = 0;

    virtual void raw(const QString& server,const QString& command) = 0;
    virtual void say(const QString& server,const QString& target,const QString& command) = 0;
    virtual void info(const QString& string) = 0;
    virtual void debug(const QString& string) = 0;
    virtual void error(const QString& string) = 0;
    virtual void insertMarkerLine() = 0;
    virtual void connectToServer(const QString& address, int port, const QString& channel, const QString& password) = 0;
    virtual QString getNickname (const QString &serverid) = 0;
    virtual QString getAnyNickname () = 0;
    virtual QStringList listServers() = 0;
    virtual QStringList listConnectedServers() = 0;
    virtual QString getChannelEncoding(const QString& server, const QString& channel) = 0;
};

class KonvIdentityIface : virtual public DCOPObject
{
    K_DCOP
        k_dcop:

    virtual void setrealName(const QString &identity, const QString& name) = 0;
    virtual QString getrealName(const QString &identity) = 0;
    virtual void setIdent(const QString &identity, const QString& ident) = 0;
    virtual QString getIdent(const QString &identity) = 0;

    virtual void setNickname(const QString &identity, int index,const QString& nick) = 0;
    virtual QString getNickname(const QString &identity, int index) = 0;

    virtual void setBot(const QString &identity, const QString& bot) = 0;
    virtual QString getBot(const QString &identity) = 0;
    virtual void setPassword(const QString &identity, const QString& password) = 0;
    virtual QString getPassword(const QString &identity) = 0;

    virtual void setNicknameList(const QString &identity, const QStringList& newList) = 0;
    virtual QStringList getNicknameList(const QString &identity) = 0;

    virtual void setQuitReason(const QString &identity, const QString& reason) = 0;
    virtual QString getQuitReason(const QString &identity) = 0;
    virtual void setPartReason(const QString &identity, const QString& reason) = 0;
    virtual QString getPartReason(const QString &identity) = 0;
    virtual void setKickReason(const QString &identity, const QString& reason) = 0;
    virtual QString getKickReason(const QString &identity) = 0;

    virtual void setShowAwayMessage(const QString &identity, bool state) = 0;
    virtual bool getShowAwayMessage(const QString &identity) = 0;

    virtual void setAwayMessage(const QString &identity, const QString& message) = 0;
    virtual QString getAwayMessage(const QString &identity) = 0;
    virtual void setReturnMessage(const QString &identity, const QString& message) = 0;
    virtual QString getReturnMessage(const QString &identity) = 0;

    virtual QStringList listIdentities() = 0;
};
#endif
