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
#include <klocale.h>
#include "linkaddressbook/addressbook.h"
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
  addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(nick, server->getServerName(), server->getServerGroup());
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
    Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid(), 1);
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
QString NickInfo::getPrettyOnlineSince() { 
  QString prettyOnlineSince;
  int daysto = onlineSince.date().daysTo( QDate::currentDate());
  if(daysto == 0) prettyOnlineSince = "Today";
  else if(daysto == 1) prettyOnlineSince = "Yesterday";
  else prettyOnlineSince = onlineSince.toString("ddd d MMMM yyyy");
  
  prettyOnlineSince += " " + onlineSince.toString("h:mm ap");
  

	return prettyOnlineSince; 
}
     
// Return the Server object that owns this NickInfo object.
Server* NickInfo::getServer() { return owningServer; }
 
// Set properties of NickInfo object.
void NickInfo::setNickname(const QString& newNickname) {
  Q_ASSERT(!newNickname.isEmpty());
  if(newNickname == nickname) return;

  KABC::Addressee newaddressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(newNickname, owningServer->getServerName(), owningServer->getServerGroup());

  if(addressee.isEmpty() && !newaddressee.isEmpty()) { //We now know who this person is
    Konversation::Addressbook::self()->associateNick(newaddressee,nickname, owningServer->getServerName(), owningServer->getServerGroup());  //Associate the old nickname with new contact
    Konversation::Addressbook::self()->saveAddressee(newaddressee);
  } else if(!addressee.isEmpty() && newaddressee.isEmpty()) {
    Konversation::Addressbook::self()->associateNick(addressee, newNickname, owningServer->getServerName(), owningServer->getServerGroup());
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
  addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(nickname, owningServer->getServerName(), owningServer->getServerGroup());
  emit nickInfoChanged();
  
  if(!addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid(), 4);
}

QString NickInfo::tooltip() {

  QString strTooltip;
  QTextStream tooltip( &strTooltip, IO_WriteOnly );
  tooltip << "<qt>";
  
  tooltip << "<table cellspacing=\"0\" cellpadding=\"0\">";
  tooltipTableData(tooltip);
  tooltip << "</table></qt>";
  return strTooltip;
}
    
void NickInfo::tooltipTableData(QTextStream &tooltip) {
  tooltip << "<tr><td colspan=\"2\" valign=\"top\">";

  bool dirty = false;
  KABC::Picture photo = addressee.photo();
  KABC::Picture logo = addressee.logo();
  bool isimage=false;
   if(photo.isIntern()) {
    QMimeSourceFactory::defaultFactory()->setImage( "photo", photo.data() );
    tooltip << "<img src=\"photo\">";
    dirty=true;
    isimage=true;
  } else if(!photo.url().isEmpty()) {
    //JOHNFLUX FIXME TODO:
    //Are there security problems with this?  loading from an external refrence?
    //Assuming not. 
    tooltip << "<img src=\"" << photo.url() << "\">";
    dirty=true;
    isimage=true;
  }
  if(logo.isIntern()) {
    QMimeSourceFactory::defaultFactory()->setImage( "logo", logo.data() );
    tooltip << "<img src=\"logo\">";
    dirty=true;
    isimage=true;
  } else if(!logo.url().isEmpty()) {
    //JOHNFLUX FIXME TODO:
    //Are there security problems with this?  loading from an external refrence?
    //Assuming not. 
    tooltip << "<img src=\"" << logo.url() << "\">";
    dirty=true;
    isimage=true;
  }
  tooltip << "<b>" << (isimage?"":"<center>");
  if(!addressee.formattedName().isEmpty()) {
    tooltip << addressee.formattedName();
    dirty = true;
  } else if(!addressee.realName().isEmpty()) {
    tooltip << addressee.realName();
    dirty = true;
  } else if(!getRealName().isEmpty() && getRealName().lower() != getNickname().lower()) {
    tooltip << getRealName();
    dirty = true;
  }
  else {
    tooltip << getNickname();
    //Don't set dirty if all we have is their nickname
  }
  tooltip << (isimage?"":"</center>") << "</b>";
  

  
  tooltip << "</td></tr>";
  if(!addressee.emails().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Email") << ": </b></td><td>";
    tooltip << addressee.emails().join(", ");
    tooltip << "</td></tr>";
    dirty=true;
  }

  if(!addressee.organization().isEmpty()) {
    tooltip << "<tr><td><b>" << addressee.organizationLabel() << ": </b></td><td>" << addressee.organization() << "</td></tr>";
    dirty=true;
  }
  if(!addressee.role().isEmpty()) {
    tooltip << "<tr><td><b>" << addressee.roleLabel() << ": </b></td><td>" << addressee.role() << "</td></tr>";
    dirty=true;
  }
  KABC::PhoneNumber::List numbers = addressee.phoneNumbers();
  for( KABC::PhoneNumber::List::Iterator it = numbers.begin(); it != numbers.end(); ++it) {
    tooltip << "<tr><td><b>" << (*it).label() << ": </b></td><td>" << (*it).number() << "</td></tr>";
    dirty=true;
  }
  if(!addressee.birthday().toString().isEmpty() ) {
    tooltip << "<tr><td><b>" << addressee.birthdayLabel() << ": </b></td><td>" << addressee.birthday().toString("ddd d MMMM yyyy") << "</td></tr>";
    dirty=true;
  }
  if(!getHostmask().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Hostmask") << ": </b></td><td>" << getHostmask() << "</td></tr>";
    dirty=true;
  }
  if(isAway() && !getAwayMessage().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Away Message") << ": </b></td><td>" << getAwayMessage() << "</td></tr>";
    dirty=true;
  }
  if(!getOnlineSince().toString().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Online Since") << ": </b></td><td>" << getPrettyOnlineSince() << "</td></tr>";
    dirty=true;
  }
				
}
    

#include "nickinfo.moc"
