/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  addressbook.cpp  - This class gives function that interact with kaddressbook.
  begin:     Fri 2004-07-23
  copyright: (C) 2004 by John Tapsell
  email:     john@geola.co.uk
*/

#include "addressbook.h"
#include <qstringlist.h>
#include <klocale.h>
#include "../server.h"
#include "../konversationapplication.h"
#include <kapplication.h>
#include <dcopclient.h>

namespace Konversation {

	
Addressbook *Addressbook::m_instance=0L;

Addressbook::Addressbook() : DCOPObject( "KIMIface")
{
	addressBook = KABC::StdAddressBook::self(true);
	m_ticket=NULL;
}
Addressbook::~Addressbook() {
}

Addressbook *Addressbook::self() {
     if (!m_instance) { sd.setObject(m_instance, new Addressbook()); }
     return m_instance;
}

QStringList Addressbook::allContacts() {
	QStringList contactUIDS;
	for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if(hasAnyNicks(*it)) contactUIDS.append((*it).uid());
	return contactUIDS;
}
//Produces a string list of all the irc nicks that are known.
QStringList Addressbook::allContactsNicks() {
	QStringList contacts;
	for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		contacts += QStringList::split( QChar( 0xE000 ), (*it).custom("messaging/irc", "All") );
	return contacts;
}

QStringList Addressbook::onlineContacts() {
	QStringList contactUIDS;
	for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if(isOnline(*it)) contactUIDS.append((*it).uid());
		
	return contactUIDS;
}
QStringList Addressbook::reachableContacts() {
	return onlineContacts();
}
QStringList Addressbook::fileTransferContacts() {
	return onlineContacts();
}
bool Addressbook::isPresent(const QString &uid) {
	return hasAnyNicks(addressBook->findByUid(uid));
}
QString Addressbook::displayName(const QString &uid) {
	return getBestNick(addressBook->findByUid(uid));
}
QString Addressbook::presenceString(const QString &uid) {
	if(uid.isEmpty()) {
	  kdDebug() << "Addressbook::presenceString() called with an empty uid" << endl;
	  return QString("Error");
	}
	switch( presenceStatus(uid)) {
	  case 0:
		return i18n("On different servers to us");
	  case 1:
		return i18n("Offline");
	  case 2:
		return i18n("Connecting"); //Shouldn't happen - not supported.
	  case 3:
		return i18n("Away");
	  case 4:
		return i18n("Online");
	}
	return QString("Error");
}
int Addressbook::presenceStatus(const QString &uid) {
	return presenceStatusByAddressee(addressBook->findByUid(uid));
}

bool Addressbook::canReceiveFiles(const QString &uid) {
	if(uid.isEmpty()) {
		kdDebug() << "Addressbook::canReceiveFiles() called with empty uid" << endl;
		return false;
	}
	int presence = presenceStatus(uid);

	return (presence == 4) || (presence == 3);
}
bool Addressbook::canRespond(const QString &uid) {
	if(uid.isEmpty()) {
		kdDebug() << "Addressbook::canRespond called with empty uid" << endl;
		return false;
	}
	//FIXME:  Check with bille what to do when contact is offline
	return true;
}
QString Addressbook::locate(const QString &contactId, const QString &/*protocol*/) {
	if(contactId.isEmpty()) {
		kdDebug() << "Addressbook::locate called with empty contactId" << endl;
		return QString::null;
	}
	//FIXME the below lines - what protocol are we using for irc?
	//if(protocol != "IRCProtocol")
		//return false;
	
	return getKABCAddresseeFromNick(contactId).uid();
}
QPixmap Addressbook::icon(const QString &uid) {
	
	Images leds;
	QIconSet currentLeds;
	if(!isPresent(uid))
		return QPixmap();

	switch(presenceStatus(uid)) {
	  case 0: //Unknown
	  case 1: //Offline
	  case 2: //connecting - invalid for us?
		currentLeds = leds.getRedLed(false);
		break;
	  case 3: //Away
		currentLeds = leds.getYellowLed(false);
		break;
	  case 4: //Online
		currentLeds = leds.getGreenLed(false);
		break;
	  default:
		//error
		kdDebug() << "Unknown status " << uid << endl;
		return QPixmap();
	}

        QPixmap joinedLed = currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);
	return joinedLed;
}
QString Addressbook::context(const QString &uid) {
	if(uid.isEmpty()) {
	       kdDebug() << "Addressbook::contact called with empty uid" << endl;
	       return QString::null;
	}
	QString context;
	return context;
}
QStringList Addressbook::protocols() {
	QStringList protocols;
	protocols.append("IRCProtocols");
	return protocols;
}

// ACTORS
/**
 * Send a single message to the specified addressee
 * Any response will be handled by the IM client as a normal 
 * conversation.
 * @param uid the KABC uid you want to chat with.
 * @param message the message to send them.
 */
void Addressbook::messageContact( const QString &uid, const QString& message ) {
	if(uid.isEmpty() || message.isEmpty()) {
	        kdDebug() << "Addressbook::messageContact called with empty uid or message" << endl;
		return;
	}
	
	
}

/**
 * Open a chat to a contact, and optionally set some initial text
 */
void Addressbook::messageNewContact( const QString &contactId, const QString &/*protocol*/ ) {
	if(contactId.isEmpty() ) {
	        kdDebug() << "Addressbook::messageNewContact called with empty contactid" << endl;
		return;
	}
}

/**
 * Start a chat session with the specified addressee
 * @param uid the KABC uid you want to chat with.
 */
void Addressbook::chatWithContact( const QString &uid ) {
	if(uid.isEmpty()) {
		kdDebug() << "Addressbook::chatWithContact called with empty uid" << endl;
		return;
	}
}

/**
 * Send the file to the contact
 * @param uid the KABC uid you are sending to.
 * @param sourceURL a @ref KURL to send.
 * @param altFileName an alternate filename describing the file
 * @param fileSize file size in bytes
 */
void Addressbook::sendFile(const QString &uid, const KURL &/*sourceURL*/, const QString &/*altFileName*/, uint /*fileSize*/) {
	if(uid.isEmpty()) {
		 kdDebug() << "Addressbook::sendFile called with empty uid" << endl;
		 return;
	}
}

// MUTATORS
// Contact list
/**
 * Add a contact to the contact list
 * @param contactId the protocol specific identifier for the contact, eg UIN for ICQ, screenname for AIM, nick for IRC.
 * @param protocol the protocol, eg one of "AIMProtocol", "MSNProtocol", "ICQProtocol", ...
 * @return whether the add succeeded.  False may signal already present, protocol not supported, or add operation not supported.
 */
bool Addressbook::addContact( const QString &/*contactId*/, const QString &/*protocol*/ ) {
	return false;
	//Nicks are auto added if they are put in the addressbook/
}

void Addressbook::emitContactPresenceChanged(QString uid, int presence) {
	if(uid.isEmpty()) {
		//This warning below is annoying.  FIXME - disabled because it's too verbose
//		kdDebug() << "Addressbook::emitContactPresenceChanged was called with empty uid" << endl;
		return;
	}
	Q_ASSERT(kapp);
	Q_ASSERT(kapp->dcopClient());
	emit contactPresenceChanged(uid, kapp->dcopClient()->appId(), presence);
	kdDebug() << "Presence changed for uid " << uid << " to " << presence << endl;
}

void Addressbook::emitContactPresenceChanged(QString uid) {
	if(uid.isEmpty()) {
		kdDebug() << "Addressbook::emitContactPresenceChanged was called with empty uid" << endl;
		return;
	};
	
	emitContactPresenceChanged(uid, presenceStatus(uid));
}

}//NAMESPACE

#include "addressbook.moc"

