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
  m_addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(nick, server->getServerName(), server->getServerGroup());
  m_nickname = nick;
  m_owningServer = server;
  m_away = false;
  m_notified = false;
  m_identified = false;
  if(!m_addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid(), 4);

  connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( refreshAddressee() ) );
  connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(refreshAddressee()));
}
NickInfo::~NickInfo()
{
  if(!m_addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid(), 1);
}


// Get properties of NickInfo object.
QString NickInfo::getNickname() const { return m_nickname; }
QString NickInfo::getHostmask() const { return m_hostmask; }

bool NickInfo::isAway() const { return m_away; }
QString NickInfo::getAwayMessage() const { return m_awayMessage; }
QString NickInfo::getIdentdInfo() const { return m_identdInfo; }
QString NickInfo::getVersionInfo() const { return m_versionInfo; }
bool NickInfo::isNotified() const { return m_notified; }
QString NickInfo::getRealName() const { return m_realName; }
QString NickInfo::getNetServer() const { return m_netServer; }
QString NickInfo::getNetServerInfo() const { return m_netServerInfo; }
QDateTime NickInfo::getOnlineSince() const { return m_onlineSince; }
bool NickInfo::isIdentified() const { return m_identified; }

QString NickInfo::getPrettyOnlineSince() const { 
  QString prettyOnlineSince;
  int daysto = m_onlineSince.date().daysTo( QDate::currentDate());
  if(daysto == 0) prettyOnlineSince = i18n("Today");
  else if(daysto == 1) prettyOnlineSince = i18n("Yesterday");
  else prettyOnlineSince = m_onlineSince.toString("ddd d MMMM yyyy");
  //TODO - we should use KLocale for this
  prettyOnlineSince += ", " + m_onlineSince.toString("h:mm ap");
  
  return prettyOnlineSince; 
}
     
// Return the Server object that owns this NickInfo object.
Server* NickInfo::getServer() const { return m_owningServer; }
 
// Set properties of NickInfo object.
void NickInfo::setNickname(const QString& newNickname) {
  Q_ASSERT(!newNickname.isEmpty());
  if(newNickname == m_nickname) return;

  KABC::Addressee newaddressee = Konversation::Addressbook::self()->getKABCAddresseeFromNick(newNickname, m_owningServer->getServerName(), m_owningServer->getServerGroup());
  if(m_addressee.isEmpty() && !newaddressee.isEmpty()) { //We now know who this person is
    Konversation::Addressbook::self()->associateNick(newaddressee,m_nickname, m_owningServer->getServerName(), m_owningServer->getServerGroup());  //Associate the old nickname with new contact
    Konversation::Addressbook::self()->saveAddressee(newaddressee);
  } else if(!m_addressee.isEmpty() && newaddressee.isEmpty()) {
    Konversation::Addressbook::self()->associateNick(m_addressee, newNickname, m_owningServer->getServerName(), m_owningServer->getServerGroup());
    Konversation::Addressbook::self()->saveAddressee(newaddressee);	
    newaddressee = m_addressee;
  }

  m_addressee = newaddressee;
  m_nickname = newNickname; 
  
  QString realname = m_addressee.realName();
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();

}
void NickInfo::setHostmask(const QString& newMask) { 
  if (newMask.isEmpty() || newMask == m_hostmask) return;
  m_hostmask = newMask;
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setAway(bool state) { 
  if(state == m_away) return;
  m_away = state; 
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
  if(!m_addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid());
}
void NickInfo::setIdentified(bool identified) {
  if(identified == m_identified) return;
  m_identified = identified;
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setAwayMessage(const QString& newMessage) { 
  if(m_awayMessage == newMessage) return;
  m_awayMessage = newMessage; 
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setIdentdInfo(const QString& newIdentdInfo) {
  if(m_identdInfo == newIdentdInfo) return;
  m_identdInfo = newIdentdInfo;
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setVersionInfo(const QString& newVersionInfo) {
  if(m_versionInfo == newVersionInfo) return;
  m_versionInfo = newVersionInfo; 
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setNotified(bool state) { 
  if(state == m_notified) return;
  m_notified = state; 
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setRealName(const QString& newRealName) { 
  if (newRealName.isEmpty() || m_realName == newRealName) return;
  m_realName = newRealName; 
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setNetServer(const QString& newNetServer) { 
  if (newNetServer.isEmpty() || m_netServer == newNetServer) return;
  m_netServer = newNetServer; 
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setNetServerInfo(const QString& newNetServerInfo) {
  if (newNetServerInfo.isEmpty() || newNetServerInfo == m_netServerInfo) return;
  m_netServerInfo = newNetServerInfo;
  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}
void NickInfo::setOnlineSince(const QDateTime& datetime) {
  if (datetime.isNull() || datetime == m_onlineSince) return;
  m_onlineSince = datetime; 


  m_owningServer->emitNickInfoChanged(this);
  emit nickInfoChanged();
}

KABC::Addressee NickInfo::getAddressee() const { return m_addressee;}

void NickInfo::refreshAddressee() {
  //m_addressee might not have changed, but information inside it may have.
  KABC::Addressee addressee=Konversation::Addressbook::self()->getKABCAddresseeFromNick(m_nickname, m_owningServer->getServerName(), m_owningServer->getServerGroup());
  if(!addressee.isEmpty() && addressee.uid() != m_addressee.uid()) {
    //This nick now belongs to a different addressee.  We need to update the status for both the old and new addressees.
    Konversation::Addressbook::self()->emitContactPresenceChanged(addressee.uid());
  }
  m_addressee = addressee;

  emit nickInfoChanged();
  m_owningServer->emitNickInfoChanged(this);
  
  if(!m_addressee.isEmpty())
    Konversation::Addressbook::self()->emitContactPresenceChanged(m_addressee.uid());
}

QString NickInfo::tooltip() const {

  QString strTooltip;
  QTextStream tooltip( &strTooltip, IO_WriteOnly );
  tooltip << "<qt>";
  
  tooltip << "<table cellspacing=\"0\" cellpadding=\"0\">";
  tooltipTableData(tooltip);
  tooltip << "</table></qt>";
  return strTooltip;
}
    
void NickInfo::tooltipTableData(QTextStream &tooltip) const {
  tooltip << "<tr><td colspan=\"2\" valign=\"top\">";

  bool dirty = false;
  KABC::Picture photo = m_addressee.photo();
  KABC::Picture logo = m_addressee.logo();
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
  if(!m_addressee.formattedName().isEmpty()) {
    tooltip << m_addressee.formattedName();
    dirty = true;
  } else if(!m_addressee.realName().isEmpty()) {
    tooltip << m_addressee.realName();
    dirty = true;
  } else if(!getRealName().isEmpty() && getRealName().lower() != getNickname().lower()) {
    tooltip << getRealName();
    dirty = true;
  }
  else {
    tooltip << getNickname();
    //Don't set dirty if all we have is their nickname
  }
  if(m_identified) tooltip << i18n(" (identified)");
  tooltip << (isimage?"":"</center>") << "</b>";
  

  
  tooltip << "</td></tr>";
  if(!m_addressee.emails().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Email") << ": </b></td><td>";
    tooltip << m_addressee.emails().join(", ");
    tooltip << "</td></tr>";
    dirty=true;
  }

  if(!m_addressee.organization().isEmpty()) {
    tooltip << "<tr><td><b>" << m_addressee.organizationLabel() << ": </b></td><td>" << m_addressee.organization() << "</td></tr>";
    dirty=true;
  }
  if(!m_addressee.role().isEmpty()) {
    tooltip << "<tr><td><b>" << m_addressee.roleLabel() << ": </b></td><td>" << m_addressee.role() << "</td></tr>";
    dirty=true;
  }
  KABC::PhoneNumber::List numbers = m_addressee.phoneNumbers();
  for( KABC::PhoneNumber::List::Iterator it = numbers.begin(); it != numbers.end(); ++it) {
    tooltip << "<tr><td><b>" << (*it).label() << ": </b></td><td>" << (*it).number() << "</td></tr>";
    dirty=true;
  }
  if(!m_addressee.birthday().toString().isEmpty() ) {
    tooltip << "<tr><td><b>" << m_addressee.birthdayLabel() << ": </b></td><td>" << m_addressee.birthday().toString("ddd d MMMM yyyy") << "</td></tr>";
    dirty=true;
  }
  if(!getHostmask().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Hostmask") << ": </b></td><td>" << getHostmask() << "</td></tr>";
    dirty=true;
  }
  if(isAway()) {
    tooltip << "<tr><td><b>" << i18n("Away Message") << ": </b></td><td>";
    if(!getAwayMessage().isEmpty())
      tooltip << getAwayMessage();
    else
      tooltip << i18n("(unknown)");
    tooltip << "</td></tr>";
    dirty=true;
  }
  if(!getOnlineSince().toString().isEmpty()) {
    tooltip << "<tr><td><b>" << i18n("Online Since") << ": </b></td><td>" << getPrettyOnlineSince() << "</td></tr>";
    dirty=true;
  }
				
}
    

#include "nickinfo.moc"

