/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
*/

#include "connectionsettings.h"
#include "preferences.h"

#include <QRegularExpression>

ConnectionSettings::ConnectionSettings()
{
    m_reconnectCount = 0;
}

ConnectionSettings::ConnectionSettings(const QString& target, const QString& port, const QString& password, const QString& nick, const QString& channel, bool useSSL)
{
    m_reconnectCount = 0;

    if (target.startsWith(QLatin1String("irc://")) || target.startsWith(QLatin1String("ircs://")))
        decodeIrcUrl(target);
    else
    {
        decodeAddress(target);

        if (!port.isEmpty())
            m_server.setPort(port.toInt());

        if (!password.isEmpty())
            m_server.setPassword(password);

        if (useSSL)
            m_server.setSSLEnabled(true);

        if (!nick.isEmpty())
            m_initialNick = nick;

        if (!channel.isEmpty())
        {
            Konversation::ChannelSettings channelSettings(channel);
            m_oneShotChannelList = {channelSettings};
        }
    }
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
    m_oneShotChannelList = list;
}

void ConnectionSettings::decodeIrcUrl(const QString& url)
{
    if (!url.startsWith(QLatin1String("irc://")) && !url.startsWith(QLatin1String("ircs://")))
        return;

    QString mangledUrl = url;

    mangledUrl.remove(QRegularExpression(QStringLiteral("^ircs?:/+")));

    if (mangledUrl.isEmpty())
        return;

    // Parsing address and channel.
    QStringList mangledUrlSegments;

    mangledUrlSegments = mangledUrl.split(QLatin1Char('/'), Qt::KeepEmptyParts);

    // Check for ",isserver".
    if (mangledUrlSegments[0].contains(QLatin1Char(',')))
    {
        QStringList addressSegments;
        bool checkIfServerGroup = true;

        addressSegments = mangledUrlSegments[0].split(QLatin1Char(','), Qt::KeepEmptyParts);

        if (!addressSegments.filter(QStringLiteral("isserver")).isEmpty())
            checkIfServerGroup = false;

        decodeAddress(addressSegments[0], checkIfServerGroup);
    }
    else
        decodeAddress(mangledUrlSegments[0]);

    QString channel;
    Konversation::ChannelSettings channelSettings;

    // Grabbing channel from in front of potential ?key=value parameters.
    if (mangledUrlSegments.size() > 1)
        channel = mangledUrlSegments[1].section(QLatin1Char('?'), 0, 0);

    if (!channel.isEmpty())
    {
        // Add default prefix to channel if necessary.
        if (!channel.contains(QRegularExpression(QStringLiteral("^[#+&]"))))
            channel.prepend(QLatin1Char('#'));

        // Qt already encoded |, we've forced # as well
        channel = QUrl::fromPercentEncoding(channel.toUtf8());

        channelSettings.setName(channel);
    }

    // Parsing ?key=value parameters.
    QString parameterString;

    if (mangledUrlSegments.size() > 1)
        parameterString = mangledUrlSegments[1].section(QLatin1Char('?'), 1);

    if (parameterString.isEmpty() && mangledUrlSegments.size() > 2)
        parameterString = mangledUrlSegments[2];

    if (!parameterString.isEmpty())
    {
        QRegularExpression parameterCatcher;
        QRegularExpressionMatch rmatch;

        parameterCatcher.setPattern(QStringLiteral("pass=([^&]+)"));
        if (parameterString.contains(parameterCatcher, &rmatch))
        {
            m_server.setPassword(rmatch.captured(1));
        }

        parameterCatcher.setPattern(QStringLiteral("key=([^&]+)"));

        if (parameterString.contains(parameterCatcher, &rmatch))
            channelSettings.setPassword(rmatch.captured(1));
    }

    // Assigning channel.
    if (!channelSettings.name().isEmpty())
    {
        m_oneShotChannelList = Konversation::ChannelList{channelSettings};
    }

    // Override SSL setting state with directive from URL.
    if (url.startsWith(QLatin1String("ircs://")))
    {
        m_server.setSSLEnabled(true);
    }
}

void ConnectionSettings::decodeAddress(const QString& address, bool checkIfServerGroup)
{
    QString host;
    QString port = QStringLiteral("6667");

    // Full-length IPv6 address with port
    // Example: RFC 2732 notation:     [2001:0DB8:0000:0000:0000:0000:1428:57ab]:6666
    // Example: Non-RFC 2732 notation: 2001:0DB8:0000:0000:0000:0000:1428:57ab:6666
    if (address.count(QLatin1Char(':'))==8)
    {
        host = address.section(QLatin1Char(':'),0,-2).remove(QLatin1Char('[')).remove(QLatin1Char(']'));
        port = address.section(QLatin1Char(':'),-1);
    }
    // Full-length IPv6 address without port or not-full-length IPv6 address with port
    // Example: Without port, RFC 2732 notation:     [2001:0DB8:0000:0000:0000:0000:1428:57ab]
    // Example: Without port, Non-RFC 2732 notation: 2001:0DB8:0000:0000:0000:0000:1428:57ab
    // Example: With port, RFC 2732 notation:        [2001:0DB8::1428:57ab]:6666
    else if (address.count(QLatin1Char(':'))>=4)
    {
        // Last segment does not end with ], but the next to last does;
        // Assume not-full-length IPv6 address with port
        // Example: [2001:0DB8::1428:57ab]:6666
        if (address.section(QLatin1Char(':'),0,-2).endsWith(QLatin1Char(']')) && !address.section(QLatin1Char(':'),-1).endsWith(QLatin1Char(']')))
        {
            host = address.section(QLatin1Char(':'),0,-2).remove(QLatin1Char('[')).remove(QLatin1Char(']'));
            port = address.section(QLatin1Char(':'),-1);
        }
        else
        {
            QString addressCopy = address;
            host = addressCopy.remove(QLatin1Char('[')).remove(QLatin1Char(']'));
        }
    }
    // IPv4 address or ordinary hostname with port
    // Example: IPv4 address with port: 123.123.123.123:6666
    // Example: Hostname with port:     irc.bla.org:6666
    else if (address.count(QLatin1Char(':'))==1)
    {
        host = address.section(QLatin1Char(':'),0,-2);
        port = address.section(QLatin1Char(':'),-1);
    }
    else
        host = address;

    // Try to assign server group.
    if (checkIfServerGroup && Preferences::isServerGroup(host))
    {
        // If host is found to be the name of a server group.

        int serverGroupId = Preferences::serverGroupIdsByName(host).first();

        Konversation::ServerGroupSettingsPtr serverGroup;

        serverGroup = Preferences::serverGroupById(serverGroupId);

        setServerGroup(serverGroup);

        if (!serverGroup->serverList().isEmpty())
            setServer(serverGroup->serverList()[0]);
    }
    else
    {
        QList<Konversation::ServerGroupSettingsPtr> groups = Preferences::serverGroupsByServer(host);
        if (!groups.isEmpty())
        {
            // If the host is found to be part of a server group's server list.

            Konversation::ServerGroupSettingsPtr serverGroup = groups.first();

            setServerGroup(serverGroup);
        }

        m_server.setHost(host);
        m_server.setPort(port.toInt());
    }
}
