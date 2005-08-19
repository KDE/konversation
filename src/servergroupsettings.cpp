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
  m_enableNotifications = true;
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
  m_enableNotifications = true;
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
  setNotificationsEnabled(settings.enableNotifications());
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
  m_enableNotifications = true;
}

ServerGroupSettings::~ServerGroupSettings()
{
}

ServerList ServerGroupSettings::serverList(bool hideQuickServer) const 
{ 
  if (!m_quickServerList.isEmpty() && !hideQuickServer) {
    return m_quickServerList+m_serverList; 
  } else {
    return m_serverList;    
  }
}

void ServerGroupSettings::setServerList(const ServerList& list)
{
  m_serverList.clear();
  m_serverList = list;
}

ServerSettings ServerGroupSettings::serverByIndex(unsigned int index) const
{
  ServerList servers = serverList();
  
  if(index < servers.count()) {
    return servers[index];
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

void ServerGroupSettings::appendChannelHistory(const ChannelSettings& channel)
{
  ChannelList::iterator endIt = m_channelHistory.end();

  for(ChannelList::iterator it = m_channelHistory.begin(); it != endIt; ++it) {
    if(channel.name() == (*it).name()) {
      (*it).setPassword(channel.password());
      (*it).setNotificationsEnabled(channel.enableNotifications());
      return;
    }
  }

  m_channelHistory.append(channel);
}

ChannelSettings ServerGroupSettings::channelByNameFromHistory(const QString& channelName)
{
  ChannelList::iterator endIt = m_channelHistory.end();

  for(ChannelList::iterator it = m_channelHistory.begin(); it != endIt; ++it) {
    if(channelName == (*it).name()) {
      return (*it);
    }
  }

  return ChannelSettings(channelName);
}

//
// ChannelSettings
//

ChannelSettings::ChannelSettings()
{
  setName("");
  setPassword("");
  setNotificationsEnabled(true);
}

ChannelSettings::ChannelSettings(const ChannelSettings& settings)
{
  setName(settings.name());
  setPassword(settings.password());
  setNotificationsEnabled(settings.enableNotifications());
}

ChannelSettings::ChannelSettings(const QString& name)
{
  setName(name);
  setPassword("");
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

ChannelSettings::~ChannelSettings()
{
}

}
