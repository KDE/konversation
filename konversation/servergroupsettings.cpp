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

namespace Konversation {

int ServerGroupSettings::s_availableId = 0;

ServerGroupSettings::ServerGroupSettings()
{
  m_id = s_availableId;
  s_availableId++;
  m_autoConnect = false;
  m_identity = 0;
}

ServerGroupSettings::ServerGroupSettings(int id)
{
  if(id < 0) {
    m_id = s_availableId;
    s_availableId++;
  } else {
    m_id = id;
  }

  m_autoConnect = false;
  m_identity = 0;
}

ServerGroupSettings::ServerGroupSettings(const ServerGroupSettings& settings)
{
  setName(settings.name());
  setGroup(settings.group());
  setServerList(settings.serverList());
  setIdentity(settings.identity());
  setChannelList(settings.channelList());
  setConnectCommands(settings.connectCommands());
  setAutoConnectEnabled(settings.autoConnectEnabled());
  m_id = settings.id();
}

ServerGroupSettings::ServerGroupSettings(const QString& name)
{
  setName(name);
  m_id = s_availableId;
  s_availableId++;
  m_autoConnect = false;
  m_identity = 0;
}

ServerGroupSettings::~ServerGroupSettings()
{
}

void ServerGroupSettings::setServerList(const ServerList& list)
{
  m_serverList.clear();
  m_serverList = list;
}

void ServerGroupSettings::setChannelList(const ChannelList& list)
{
  m_channelList.clear();
  m_channelList = list;
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
