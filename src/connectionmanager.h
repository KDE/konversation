/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
*/

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "server.h"
#include "identity.h"

#include <QObject>
#include <QSet>
#include <QNetworkInformation>

class ConnectionSettings;

class ConnectionManager : public QObject
{
    Q_OBJECT
    friend class Application;

    public:
        explicit ConnectionManager(QObject* parent = nullptr);
        ~ConnectionManager() override;

        uint connectionCount() const { return m_connectionList.count(); }

        QList<Server*> getServerList() const;

        enum NameMatchFlags
        {
            MatchByName,
            MatchByIdThenName
        };

        Server* getServerByConnectionId(int connectionId) const;
        Server* getServerByName(const QString& name, NameMatchFlags flags = MatchByName) const;


    public Q_SLOTS:
        void connectTo(Konversation::ConnectionFlag flag,
                       const QString& target,
                       const QString& port = QString(),
                       const QString& password = QString(),
                       const QString& nick = QString(),
                       const QString& channel = QString(),
                       bool useSSL = false);

        void connectTo(Konversation::ConnectionFlag flag, int serverGroupId);
        void connectTo(Konversation::ConnectionFlag flag, const QList<QUrl>& list);
        void connectTo(Konversation::ConnectionFlag flag, ConnectionSettings settings);

        void quitServers();
        void reconnectServers();

        void involuntaryQuitServers();
        void reconnectInvoluntary();


    Q_SIGNALS:
        void connectionListChanged();

        void connectionChangedState(Server* server, Konversation::ConnectionState state);

        void connectionChangedAwayState(bool away);

        void requestReconnect(Server* server);

        void identityOnline(int identityId);
        void identityOffline(int identityId);

        void closeServerList();


    private Q_SLOTS:
        void delistConnection(int connectionId);

        void handleConnectionStateChange(Server* server, Konversation::ConnectionState state);

        void handleReconnect(Server* server);
        void onOnlineStateChanged(QNetworkInformation::Reachability reachability);

    private:
        void enlistConnection(int connectionId, Server* server);

        void decodeIrcUrl(const QString& url, ConnectionSettings& settings);

        void decodeAddress(const QString& address,
                           ConnectionSettings& settings,
                           bool checkIfServerGroup = true);

        bool reuseExistingConnection(ConnectionSettings& settings, bool interactive);
        bool validateIdentity(IdentityPtr identity, bool interactive = true);

    private:
        QMap<int, Server*> m_connectionList;
        QSet<uint> m_activeIdentities;
        bool m_overrideAutoReconnect;

        enum ConnectionDupe { SameServer, SameServerGroup };

        Q_DISABLE_COPY(ConnectionManager)
};

#endif
