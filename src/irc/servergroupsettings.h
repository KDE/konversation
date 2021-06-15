/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2007 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONSERVERGROUPSETTINGS_H
#define KONVERSATIONSERVERGROUPSETTINGS_H

#include "serversettings.h"
#include "identity.h"


#include <QExplicitlySharedDataPointer>


namespace Konversation
{

    class ChannelSettings
    {
        public:
            ChannelSettings();
            explicit ChannelSettings(const QString& name);
            ChannelSettings(const QString& name, const QString& password);
            ChannelSettings(const QString& name, const QString& password, bool enableNotifications);

            void setName(const QString& name) { m_name = name; }
            QString name() const { return m_name; }

            void setPassword(const QString& password) { m_password = password; }
            QString password() const { return m_password; }

            void setNotificationsEnabled(bool enable) { m_enableNotifications = enable; }
            bool enableNotifications() const { return m_enableNotifications; }

            bool operator==(const ChannelSettings& channel) const;

        private:
            QString m_name;
            QString m_password;

            bool m_enableNotifications;
    };

    class ServerGroupSettings;
    using ServerGroupSettingsPtr = QExplicitlySharedDataPointer<ServerGroupSettings>;
    using ServerGroupHash = QHash<int, ServerGroupSettingsPtr>;
    using ServerList = QList<ServerSettings>;
    using ChannelList = QList<ChannelSettings>;

    class ServerGroupSettings : public QSharedData
    {
        public:
            explicit ServerGroupSettings();
            explicit ServerGroupSettings(int id);
            explicit ServerGroupSettings(const ServerGroupSettings& settings);
            explicit ServerGroupSettings(const QString& name);

            ServerGroupSettings& operator=(const ServerGroupSettings& settings);

            void setName(const QString& name) { m_name = name; }
            QString name() const { return m_name; }

            void setServerList(const ServerList& list);
            void addServer(const ServerSettings& settings) { m_serverList.append(settings); }
            void removeServer(const ServerSettings& settings);
            ServerList serverList() const { return m_serverList; }
            ServerSettings serverByIndex(int index) const;


            void setIdentityId(int identityId) { m_identityId = identityId; }
            int identityId() const { return m_identityId; }
            IdentityPtr identity() const;

            void setChannelList(const ChannelList& list);
            void addChannel(const ChannelSettings& channel) { m_channelList.append(channel); }
            void addChannel(const ChannelSettings& channel, const ChannelSettings& before);
            void removeChannel(const ChannelSettings& channel);
            ChannelList channelList() const { return m_channelList; }
            ChannelSettings channelByIndex(int index) const;

            void setConnectCommands(const QString& commands) { m_connectCommands = commands; }
            QString connectCommands() const { return m_connectCommands; }

            void setAutoConnectEnabled(bool enabled) { m_autoConnect = enabled; }
            bool autoConnectEnabled() const { return m_autoConnect; }

            int id() const { return m_id; }

            void setSortIndex(int sortIndex) { m_sortIndex = sortIndex; }
            int sortIndex() const { return m_sortIndex; }

            void clearChannelHistory();
            void setChannelHistory(const ChannelList& list) { m_channelHistory = list; }
            void appendChannelHistory(const ChannelSettings& channel);
            void removeChannelFromHistory(const ChannelSettings& channel);
            ChannelList channelHistory() const { return m_channelHistory; }
            ChannelSettings channelByNameFromHistory(const QString& channelName);

            void setNotificationsEnabled(bool enable) { m_enableNotifications = enable; }
            bool enableNotifications() const { return m_enableNotifications; }

            void setExpanded(bool enable) { m_expanded = enable; }
            bool expanded() const { return m_expanded; }

        private:
            static int s_availableId;
            int m_sortIndex;
            QString m_name;
            ServerList m_serverList;
            int m_identityId;
            ChannelList m_channelList;
            ChannelList m_channelHistory;
            QString m_connectCommands;
            bool m_autoConnect;
            QString m_group;
            int m_id;
            bool m_enableNotifications;
            bool m_expanded;
    };

}
#endif
