/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
*/

#ifndef CONNECTIONSETTINGS_H
#define CONNECTIONSETTINGS_H

#include "servergroupsettings.h"
#include "serversettings.h"

#include <QString>


class ConnectionSettings
{
    public:
        explicit ConnectionSettings();
        explicit ConnectionSettings(const QString& target,
                                    const QString& port = QString(),
                                    const QString& password = QString(),
                                    const QString& nick = QString(),
                                    const QString& channel = QString(),
                                    bool useSSL = false);

        bool isValid() const;

        QString name() const;

        Konversation::ServerSettings server() const { return m_server; }
        void setServer(const Konversation::ServerSettings &server) { m_server = server; }

        Konversation::ServerGroupSettingsPtr serverGroup() const { return m_serverGroup; }
        void setServerGroup(const Konversation::ServerGroupSettingsPtr &serverGroup) { m_serverGroup = serverGroup; }

        IdentityPtr identity() const;

        QString initialNick() const;
        void setInitialNick(const QString& nick) { m_initialNick = nick; }

        void setOneShotChannelList(const Konversation::ChannelList& list);
        Konversation::ChannelList oneShotChannelList() const { return m_oneShotChannelList; }
        void clearOneShotChannelList() { m_oneShotChannelList.clear(); }

        uint reconnectCount() const { return m_reconnectCount; }
        void incrementReconnectCount() { m_reconnectCount++; }
        void setReconnectCount(uint count) { m_reconnectCount = count; }


    private:
        void decodeIrcUrl(const QString& url);

        void decodeAddress(const QString& address, bool checkIfServerGroup = true);

        Konversation::ServerSettings m_server;
        Konversation::ServerGroupSettingsPtr m_serverGroup;

        QString m_initialNick;

        Konversation::ChannelList m_oneShotChannelList;

        uint m_reconnectCount;
};


#endif
