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

#include <qtooltip.h>
#include "server.h"
/*
  @author Gary Cramblitt
*/

/*
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.
  If using code alters the NickInfo object, it should call Server::nickInfoUpdated to
  let Server know that the object has been modified.
*/

NickInfo::NickInfo(const QString& nick, Server* server): KShared()
{
  addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(nick);
  nickname = nick;
  owningServer = server;
  away = false;
  notified = false;

  if(!addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid(), 4);
}
NickInfo::~NickInfo()
{
  if(!addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid());
}


// Get properties of NickInfo object.
QString NickInfo::getNickname() const { return nickname; }
QString NickInfo::getHostmask() const { return hostmask; }
bool NickInfo::isAway() { return away; }
QString NickInfo::getAwayMessage() { return awayMessage; }
QString NickInfo::getIdentdInfo() { return identdInfo; }
QString NickInfo::getVersionInfo() { return versionInfo; }
bool NickInfo::isNotified() { return notified; }
QString NickInfo::getRealName() { return realName; }
QString NickInfo::getNetServer() { return netServer; }
QString NickInfo::getNetServerInfo() { return netServerInfo; }
QDateTime NickInfo::getOnlineSince() { return onlineSince; }
     
// Return the Server object that owns this NickInfo object.
Server* NickInfo::getServer() { return owningServer; }
 
// Set properties of NickInfo object.
void NickInfo::setNickname(const QString& newNickname) {
  Q_ASSERT(!newNickname.isEmpty());
  if(newNickname == nickname) return;

  KABC::Addressee newaddressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(newNickname);

  if(addressee.isEmpty() && !newaddressee.isEmpty()) { //We now know who this person is
    Konversation::Addressbook::self()->associateNick(newaddressee,nickname);  //Associate the old nickname with new contact
    Konversation::Addressbook::self()->saveAddressee(newaddressee);
  } else if(!addressee.isEmpty() && newaddressee.isEmpty()) {
    Konversation::Addressbook::self()->associateNick(addressee, newNickname);
    Konversation::Addressbook::self()->saveAddressee(newaddressee);	
    newaddressee = addressee;
  }

  addressee = newaddressee;
  nickname = newNickname; 
  
  QString realname = addressee.realName();
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();

}
void NickInfo::setHostmask(const QString& newMask) { 
  if (newMask.isEmpty() || newMask == hostmask) return;
  hostmask = newMask;
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setAway(bool state) { 
  if(state == away) return;
  away = state; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
  if(!addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid());
}
void NickInfo::setAwayMessage(const QString& newMessage) { 
  if(awayMessage == newMessage) return;
  awayMessage = newMessage; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setIdentdInfo(const QString& newIdentdInfo) {
  if(identdInfo == newIdentdInfo) return;
  identdInfo = newIdentdInfo;
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setVersionInfo(const QString& newVersionInfo) {
  if(versionInfo == newVersionInfo) return;
  versionInfo = newVersionInfo; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setNotified(bool state) { 
  if(state == notified) return;
  notified = state; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setRealName(const QString& newRealName) { 
  if (newRealName.isEmpty() || realName == newRealName) return;
  realName = newRealName; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setNetServer(const QString& newNetServer) { 
  if (newNetServer.isEmpty() || netServer == newNetServer) return;
  netServer = newNetServer; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setNetServerInfo(const QString& newNetServerInfo) {
  if (newNetServerInfo.isEmpty() || newNetServerInfo == netServerInfo) return;
  netServerInfo = newNetServerInfo;
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setOnlineSince(const QDateTime& datetime) {
  if (datetime.isNull() || datetime == onlineSince) return;
  onlineSince = datetime; 
  owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}

KABC::Addressee NickInfo::getAddressee() { return addressee;}

void NickInfo::refreshAddressee() {
  emit nickInfoChanged();
}

