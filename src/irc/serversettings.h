/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONSERVERSETTINGS_H
#define KONVERSATIONSERVERSETTINGS_H

#include <QString>

namespace Konversation
{

    class ServerSettings
    {
        public:
            ServerSettings();
            ServerSettings(const ServerSettings& settings);
            explicit ServerSettings(const QString& host);
            ~ServerSettings() = default;

            ServerSettings &operator=(const ServerSettings& settings);

            void setHost(const QString& host);
            QString host() const { return m_host; }

            void setPort(int port) { m_port = port; }
            int port() const { return m_port;}

            void setPassword(const QString& password);
            QString password() const { return m_password; }

            void setSSLEnabled(bool enabled) { m_SSLEnabled = enabled; }
            bool SSLEnabled() const { return m_SSLEnabled; }

            void setBypassProxy(bool bypass) { m_bypassProxy = bypass; }
            bool bypassProxy() const { return m_bypassProxy; }

            bool operator== (const ServerSettings& settings) const;

        private:
            QString m_host;
            int m_port;
            QString m_password;
            bool m_SSLEnabled;
            bool m_bypassProxy;

    };

}
#endif
