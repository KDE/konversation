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
#include "servergroupsettings.h"

#include "konversationapplication.h"

namespace Konversation {

int ServerGroupSettings::s_availableId = 0;

ServerGroupSettings::ServerGroupSettings()
  : KShared()
{
  m_id = s_availableId;
  s_availableId++;
  m_autoConnect = false;
  m_identityId = 0;
}

ServerGroupSettings::ServerGroupSettings(int id)
  : KShared()
{
  if(id < 0) {
    m_id = s_availableId;
    s_availableId++;
  } else {
    m_id = id;
  }

  m_autoConnect = false;
  m_identityId = 0;
}

ServerGroupSettings::ServerGroupSettings(const ServerGroupSettings& settings)
  : KShared()
{
  setName(settings.name());
  setGroup(settings.group());
  setServerList(settings.serverList());
  setIdentityId(settings.identityId());
  setChannelList(settings.channelList());
  setConnectCommands(settings.connectCommands());
  setAutoConnectEnabled(settings.autoConnectEnabled());
  m_id = settings.id();
}

ServerGroupSettings::ServerGroupSettings(const QString& name)
  : KShared()
{
  setName(name);
  m_id = s_availableId;
  s_availableId++;
  m_autoConnect = false;
  m_identityId = 0;
}

ServerGroupSettings::~ServerGroupSettings()
{
}

void ServerGroupSettings::setServerList(const ServerList& list)
{
  m_serverList.clear();
  m_serverList = list;
}

ServerSettings ServerGroupSettings::serverByIndex(unsigned int index) const
{
  if(index < m_serverList.count()) {
    return m_serverList[index];
  }

  return ServerSettings();
}

void ServerGroupSettings::setChannelList(const ChannelList& list)
{
  m_channelList.clear();
  m_channelList = list;
}

ChannelSettings ServerGroupSettings::channelByIndex(unsigned int index) const
{
  if(index < m_channelList.count()) {
    return m_channelList[index];
  }

  return ChannelSettings();
}

IdentityPtr ServerGroupSettings::identity() const
{
  return KonversationApplication::preferences.getIdentityById(m_identityId);
}

//
// ChannelSettings
//

ChannelSettings::ChannelSettings()
{
  setName("");
  setPassword("");
}

ChannelSettings::ChannelSettings(const ChannelSettings& settings)
{
  setName(settings.name());
  setPassword(settings.password());
}

ChannelSettings::ChannelSettings(const QString& name)
{
  setName(name);
  setPassword("");
}

ChannelSettings::~ChannelSettings()
{
}

}
