/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
*/

#include "connectionsettings.h"
#include "config/preferences.h"


ConnectionSettings::ConnectionSettings()
{
    m_reconnectCount = 0;
}

bool ConnectionSettings::isValid() const
{
    if (m_server.host().isEmpty()) return false;

    return true;
}

QString ConnectionSettings::name() const
{
    if (m_serverGroup)
        return m_serverGroup->name();
    else
        return m_server.host();
}

IdentityPtr ConnectionSettings::identity() const
{
    if (m_serverGroup)
        return m_serverGroup->identity();

    return Preferences::identityById(0);
}

QString ConnectionSettings::initialNick() const
{
    if (!m_initialNick.isEmpty())
        return m_initialNick;

    return identity()->getNickname(0);
}

void ConnectionSettings::setOneShotChannelList(const Konversation::ChannelList& list)
{
    m_oneShotChannelList.clear();
    m_oneShotChannelList = list;
}

