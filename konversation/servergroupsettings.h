/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONSERVERGROUPSETTINGS_H
#define KONVERSATIONSERVERGROUPSETTINGS_H

#include <qvaluelist.h>
#include <qstringlist.h>

#include "serversettings.h"
#include "identity.h"

namespace Konversation {

class ChannelSettings;
class ServerGroupSettings;

typedef QValueList<ServerGroupSettings> ServerGroupList;
typedef QValueList<ServerSettings> ServerList;
typedef QValueList<ChannelSettings> ChannelList;

class ServerGroupSettings
{
  public:
    ServerGroupSettings();
    ServerGroupSettings(int id);
    ServerGroupSettings(const ServerGroupSettings& settings);
    ServerGroupSettings(const QString& name);
    ~ServerGroupSettings();

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setServerList(const ServerList& list);
    void addServer(const ServerSettings& settings) { m_serverList.append(settings); }
    ServerList serverList() const { return m_serverList; }
    ServerSettings serverByIndex(unsigned int index) const;

    void setIdentity(IdentityPtr identity) { m_identity = identity; }
    IdentityPtr identity() const { return m_identity; }

    void setChannelList(const ChannelList& list);
    void addChannel(const ChannelSettings& channel) { m_channelList.append(channel); }
    ChannelList channelList() const { return m_channelList; }
    ChannelSettings channelByIndex(unsigned int index) const;

    void setConnectCommands(const QString& commands) { m_connectCommands = commands; }
    QString connectCommands() const { return m_connectCommands; }

    void setAutoConnectEnabled(bool enabled) { m_autoConnect = enabled; }
    bool autoConnectEnabled() const { return m_autoConnect; }

    void setGroup(const QString& group) { m_group = group; }
    QString group() const { return m_group; }

    int id() const { return m_id; }

  private:
    QString m_name;
    ServerList m_serverList;
    IdentityPtr m_identity;
    ChannelList m_channelList;
    QString m_connectCommands;
    bool m_autoConnect;
    QString m_group;
    int m_id;
    static int s_availableId;
};

class ChannelSettings
{
  public:
    ChannelSettings();
    ChannelSettings(const ChannelSettings& settings);
    ChannelSettings(const QString& name);
    ~ChannelSettings();

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setPassword(const QString& password) { m_password = password; }
    QString password() const { return m_password; }

  private:
    QString m_name;
    QString m_password;
};

};

#endif
