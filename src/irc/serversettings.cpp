/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2008 Eike Hein <hein@kde.org>
*/

#include "serversettings.h"


namespace Konversation
{

    ServerSettings::ServerSettings()
    {
        setPort(6667);
        setSSLEnabled(false);
    }

    ServerSettings::ServerSettings(const ServerSettings& settings)
    {
        setHost(settings.host());
        setPort(settings.port());
        setPassword(settings.password());
        setSSLEnabled(settings.SSLEnabled());
    }

    ServerSettings::ServerSettings(const QString& host)
    {
        setHost(host);
        setPort(6667);
        setSSLEnabled(false);
    }

    ServerSettings::~ServerSettings()
    {
    }

    bool ServerSettings::operator==(const ServerSettings& settings) const
    {
        if (m_host.toLower() == settings.host().toLower()
            && m_port == settings.port()
            && m_password == settings.password()
            && m_SSLEnabled == settings.SSLEnabled())
        {
            return true;
        }
        else
            return false;
    }

    void ServerSettings::setHost(const QString& host)
    {
        m_host = host.trimmed();
    }

    void ServerSettings::setPassword(const QString& password)
    {
        m_password = password.trimmed();
    }
}
