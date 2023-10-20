/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2007 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#include "servergroupsettings.h"
#include "application.h"


namespace Konversation
{

    int ServerGroupSettings::s_availableId = 0;

    ServerGroupSettings::ServerGroupSettings()
        : QSharedData()
    {
        m_id = s_availableId;
        s_availableId++;
        m_sortIndex = m_id;
        m_autoConnect = false;
        m_identityId = 0;
        m_enableNotifications = true;
        m_expanded = false;
    }

    ServerGroupSettings::ServerGroupSettings(int id)
        : QSharedData()
    {
        if(id < 0)
        {
            m_id = s_availableId;
            s_availableId++;
        }
        else
        {
            m_id = id;
        }

        m_sortIndex = m_id;
        m_autoConnect = false;
        m_identityId = 0;
        m_enableNotifications = true;
        m_expanded = false;
    }

    ServerGroupSettings::ServerGroupSettings(const ServerGroupSettings& settings)
        : QSharedData()
    {
        (*this) = settings;
    }


    ServerGroupSettings& Konversation::ServerGroupSettings::operator=(const ServerGroupSettings& settings)
    {
        setName(settings.name());
        setServerList(settings.serverList());
        setIdentityId(settings.identityId());
        setChannelList(settings.channelList());
        setConnectCommands(settings.connectCommands());
        setAutoConnectEnabled(settings.autoConnectEnabled());
        setNotificationsEnabled(settings.enableNotifications());
        setExpanded(settings.expanded());
        m_id = settings.id();
        m_sortIndex = settings.sortIndex();
        return *this;
    }

    ServerGroupSettings::ServerGroupSettings(const QString& name)
        : QSharedData()
    {
        setName(name);
        m_id = s_availableId;
        s_availableId++;
        m_sortIndex = m_id;
        m_autoConnect = false;
        m_identityId = 0;
        m_enableNotifications = true;
        m_expanded = false;
    }

    void ServerGroupSettings::setServerList(const ServerList& list)
    {
        m_serverList.clear();
        m_serverList = list;
    }

    void ServerGroupSettings::removeServer(const ServerSettings& settings)
    {
        m_serverList.removeOne(settings);
    }

    ServerSettings ServerGroupSettings::serverByIndex(int index) const
    {
        ServerList servers = serverList();

        if(index < servers.count())
        {
            return servers[index];
        }

        return ServerSettings();
    }

    void ServerGroupSettings::setChannelList(const ChannelList& list)
    {
        m_channelList.clear();
        m_channelList = list;
    }

    ChannelSettings ServerGroupSettings::channelByIndex(int index) const
    {
        if(index < m_channelList.count())
        {
            return m_channelList[index];
        }

        return ChannelSettings();
    }

    void ServerGroupSettings::addChannel(const ChannelSettings& channel, const ChannelSettings& before)
    {
        if (before.name().isEmpty())
            m_channelList.append(channel);
        else
            m_channelList.insert(m_channelList.indexOf(before), channel);
    }

    void ServerGroupSettings::removeChannel(const ChannelSettings& channel)
    {
        m_channelList.removeAll(channel);
    }

    void ServerGroupSettings::removeChannelFromHistory(const ChannelSettings& channel)
    {
        m_channelHistory.removeAll(channel);
    }

    IdentityPtr ServerGroupSettings::identity() const
    {
        return Preferences::identityById(m_identityId);
    }

    void ServerGroupSettings::clearChannelHistory()
    {
      m_channelHistory.clear();
    }

    void ServerGroupSettings::appendChannelHistory(const ChannelSettings& channel)
    {
        for (auto& historyChannel : m_channelHistory) {
            if (channel.name() == historyChannel.name()) {
                historyChannel.setPassword(channel.password());
                historyChannel.setNotificationsEnabled(channel.enableNotifications());
                return;
            }
        }

        m_channelHistory.append(channel);
    }

    ChannelSettings ServerGroupSettings::channelByNameFromHistory(const QString& channelName)
    {
        for (const auto& channel : std::as_const(m_channelHistory)) {
            if (channelName == channel.name()) {
                return channel;
            }
        }

        return ChannelSettings(channelName);
    }

    //
    // ChannelSettings
    //

    ChannelSettings::ChannelSettings()
    {
        setNotificationsEnabled(true);
    }

    ChannelSettings::ChannelSettings(const QString& name)
    {
        setName(name);
        setNotificationsEnabled(true);
    }

    ChannelSettings::ChannelSettings(const QString& name, const QString& password)
    {
        setName(name);
        setPassword(password);
        setNotificationsEnabled(true);
    }

    ChannelSettings::ChannelSettings(const QString& name, const QString& password, bool enableNotifications)
    {
        setName(name);
        setPassword(password);
        setNotificationsEnabled(enableNotifications);
    }

    bool ChannelSettings::operator== (const ChannelSettings& channel) const
    {
        if (m_name.toLower() == channel.name().toLower())
            return true;
        else
            return false;
    }

}
