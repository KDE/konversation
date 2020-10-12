/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Alex Zepeda <zipzippy@sonic.net>
*/

#ifndef KONVERSATION_DBUS_H
#define KONVERSATION_DBUS_H

#include "common.h"

#include <QObject>

namespace Konversation
{

/**
 * The konversation D-Bus interface class
 */
class DBus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.konversation")

    public:
        explicit DBus(QObject *parent = nullptr);

        QString getNickname (const QString &server);
        QString getChannelEncoding(const QString& server, const QString& channel);

    Q_SIGNALS:
        void dbusSay(const QString& server,const QString& target,const QString& command);
        void dbusInfo(const QString& string);
        void dbusInsertMarkerLine();
        void dbusRaw(const QString& server, const QString& command);
        void dbusMultiServerRaw(const QString& command);

        void connectTo(Konversation::ConnectionFlag flag,
                       const QString& hostName,
                       const QString& port = QString(),
                       const QString& password = QString(),
                       const QString& nick = QString(),
                       const QString& channel = QString(),
                       bool useSSL = false);

    public Q_SLOTS:
        void setAway(const QString &awaymessage);
        void setBack();
        void sayToAll(const QString &message);
        void actionToAll(const QString &message);
        void raw(const QString& server,const QString& command);
        void say(const QString& server,const QString& target,const QString& command);
        void info(const QString& string);
        void debug(const QString& string);
        void error(const QString& string);
        void insertMarkerLine();
        void connectToServer(const QString& address, int port, const QString& channel, const QString& password);
        QStringList listConnections();
        QStringList listServers();
        QStringList listConnectedServers();
        QStringList listJoinedChannels(const QString& server);

    private Q_SLOTS:
        void changeAwayStatus(bool away);
};

class IdentDBus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.konversation")

        public:
        explicit IdentDBus(QObject *parent = nullptr);

    public Q_SLOTS:
        void setrealName(const QString &identity, const QString& name);
        QString getrealName(const QString &identity);
        void setIdent(const QString &identity, const QString& ident);
        QString getIdent(const QString &identity);

        void setNickname(const QString &identity, int index,const QString& nick);
        QString getNickname(const QString &identity, int index);

        void setBot(const QString &identity, const QString& bot);
        QString getBot(const QString &identity);
        void setPassword(const QString &identity, const QString& password);
        QString getPassword(const QString &identity);

        void setNicknameList(const QString &identity, const QStringList& newList);
        QStringList getNicknameList(const QString &identity);

        void setQuitReason(const QString &identity, const QString& reason);
        QString getQuitReason(const QString &identity);
        void setPartReason(const QString &identity, const QString& reason);
        QString getPartReason(const QString &identity);
        void setKickReason(const QString &identity, const QString& reason);
        QString getKickReason(const QString &identity);

        void setRunAwayCommands(const QString &identity, bool run);
        bool getRunAwayCommands(const QString &identity);
        void setAwayCommand(const QString &identity, const QString& command);
        QString getAwayCommand(const QString &identity);
        void setReturnCommand(const QString &identity, const QString& command);
        QString getReturnCommand(const QString &identity);

        void setAwayNickname(const QString& identity, const QString& nickname);
        QString getAwayNickname(const QString& identity);

        void setAwayMessage(const QString& identity, const QString& message);
        QString getAwayMessage(const QString& identity);

        QStringList listIdentities();
};

}

#endif
