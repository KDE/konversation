/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2008 Eike Hein <hein@kde.org>
*/

#include "connectionsettings.h"
#include "config/preferences.h"


ConnectionSettings::ConnectionSettings()
{
    m_reconnectCount = 0;
}

ConnectionSettings::~ConnectionSettings()
{
}

bool ConnectionSettings::isValid()
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

