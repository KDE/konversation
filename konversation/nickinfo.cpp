/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nickinfo.cpp    -  Nick Information
  begin:     Sat Jan 17 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#include "nickinfo.h"

/*
  @author Gary Cramblitt
*/

/*
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.
  If using code alters the NickInfo object, it should call Server::nickInfoUpdated to
  let Server know that the object has been modified.
*/

NickInfo::NickInfo(const QString& nick, Server* server)
{
  nickname = nick;
  owningServer = server;
}
NickInfo::~NickInfo()
{
}


// Get properties of NickInfo object.
QString NickInfo::getNickname() { return nickname; }
QString NickInfo::getHostmask() { return hostmask; }
bool NickInfo::isAway() { return away; }
QString NickInfo::getAwayMessage() { return awayMessage; }
QString NickInfo::getIdentdInfo() { return identdInfo; }
QString NickInfo::getVersionInfo() { return versionInfo; }
bool NickInfo::isNotified() { return notified; }
     
// Return the Server object that owns this NickInfo object.
Server* NickInfo::getServer() { return owningServer; }
     
// Set properties of NickInfo object.
// If any of these are called, call Server::nickInfoUpdated to let Server know about the change.
void NickInfo::setNickname(const QString& newNickname) { nickname = newNickname; }
void NickInfo::setHostmask(const QString& newMask)
{
  if (!newMask.isEmpty()) hostmask = newMask;
}
void NickInfo::setAway(bool state) { away = state; }
void NickInfo::setAwayMessage(const QString& newMessage) { awayMessage = newMessage; }
void NickInfo::setIdentdInfo(const QString& newIdentdInfo) {identdInfo = newIdentdInfo; }
void NickInfo::setVersionInfo(const QString& newVersionInfo) { versionInfo = newVersionInfo; }
void NickInfo::setNotified(bool state) { notified = state; }

