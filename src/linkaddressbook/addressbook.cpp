/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class gives function that interact with kaddressbook.
  begin:     Fri 2004-07-23
  copyright: (C) 2004 by John Tapsell
  email:     john@geola.co.uk
*/

#include "addressbook.h"
#include <qstringlist.h>
#include "../konversationmainwindow.h"
#include "qwidget.h"
#include <klocale.h>
#include <kmessagebox.h>
#include "../server.h"
#include "../channel.h"
#include "../konversationapplication.h"
#include <kapplication.h>
#include <dcopclient.h>
#include <kwin.h>

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
		return "";
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
	//this should return false if they are offline.
	int result = presenceStatus(uid);
	if(result == 3 || result == 4) return true;
	return false;
}
QString Addressbook::locate(const QString &contactId, const QString &protocol) {
	if(contactId.isEmpty()) {
		kdDebug() << "Addressbook::locate called with empty contactId" << endl;
		return QString::null;
	}
	if(protocol != "messaging/irc")
		return QString::null;
	
	return getKABCAddresseeFromNick(contactId).uid();
}
QPixmap Addressbook::icon(const QString &uid) {
	
	Images* icons = KonversationApplication::instance()->images();
	QIconSet currentIcon;
	if(!isPresent(uid))
		return QPixmap();

	switch(presenceStatus(uid)) {
	  case 0: //Unknown
	  case 1: //Offline
	  case 2: //connecting - invalid for us?
		currentIcon = icons->getKimproxyOffline();
		break;
	  case 3: //Away
		currentIcon = icons->getKimproxyAway();
		break;
	  case 4: //Online
		currentIcon = icons->getKimproxyOnline();
		break;
	  default:
		//error
		kdDebug() << "Unknown status " << uid << endl;
		return QPixmap();
	}

        QPixmap joinedIcon = currentIcon.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);
	return joinedIcon;
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
	protocols.append("messaging/irc");
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
	if(uid.isEmpty()) {
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but did not specify any contact to send the message to.  This is probably a bug in the other application."));
		return;
	}
	KABC::Addressee addressee = addressBook->findByUid(uid);
        if(addressee.isEmpty()) {
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but Konversation could not find the specified contact in the KDE address book."));
                return;
	}
	NickInfoPtr nickInfo = getNickInfo(addressee);
	if(!nickInfo) {
		QString user = addressee.fullEmail();
		if(!user.isEmpty()) user = " (" + user + ")";
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but the requested user%1 is not online.").arg(user));
		return;
	}

	nickInfo->getServer()->dcopSay(nickInfo->getNickname(), message);
}



/**
 * Open a chat to a contact, and optionally set some initial text
 */
void Addressbook::messageNewContact( const QString &contactId, const QString &/*protocol*/ ) {
	if(contactId.isEmpty() ) {
	        kdDebug() << "Addressbook::messageNewContact called with empty contactid" << endl;
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but did not specify any contact to send the message to.  This is probably a bug in the other application."));
		return;
	}
	messageContact(contactId, QString::null);
}

/**
 * Start a chat session with the specified addressee
 * @param uid the KABC uid you want to chat with.
 */
void Addressbook::chatWithContact( const QString &uid ) {
	if(uid.isEmpty()) {
		kdDebug() << "Addressbook::chatWithContact called with empty uid" << endl;
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but did not specify any contact to send the message to.  This is probably a bug in the other application."));
		return;
	}
	messageContact(uid, QString::null);
}

/**
 * Send the file to the contact
 * @param uid the KABC uid you are sending to.
 * @param sourceURL a @ref KURL to send.
 * @param altFileName an alternate filename describing the file
 * @param fileSize file size in bytes
 */
void Addressbook::sendFile(const QString &uid, const KURL &sourceURL, const QString &altFileName, uint fileSize) {
	if(uid.isEmpty()) {
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to send a file to a contact, but did not specify any contact to send the file to.  This is probably a bug in the other application."));
		return;
	}
	KABC::Addressee addressee = addressBook->findByUid(uid);
	if(addressee.isEmpty()) {
		focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to send a file to a contact, but Konversation could not find the specified contact in the KDE address book."));
		return;
	}
	NickInfoPtr nickInfo = getNickInfo(addressee);
        if(!nickInfo) {
		QString user = addressee.fullEmail();
                if(!user.isEmpty()) user = " (" + user + ")";
                focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to send a file to a contact, but the requested user%1 is not currently online.").arg(user));
        	return;
        }
        nickInfo->getServer()->addDccSend(nickInfo->getNickname(), sourceURL, altFileName, fileSize);
	QWidget *widget = nickInfo->getServer()->getMainWindow();
	KWin::demandAttention(widget->winId()); //If activeWindow request is denied, at least demand attention!
	KWin::activateWindow(widget->winId());  //May or may not work, depending on focus stealing prevention.


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
	focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to add a contact.  Konversation does support this."));
	return false;
	//Nicks are auto added if they are put in the addressbook - I don' think there is anything useful I can do.
}

void Addressbook::emitContactPresenceChanged(const QString &uid, int presence) {
	if(uid.isEmpty()) {
		//This warning below is annoying.  FIXME - disabled because it's too verbose
//		kdDebug() << "Addressbook::emitContactPresenceChanged was called with empty uid" << endl;
		return;
	}
	Q_ASSERT(kapp->dcopClient());
	emit contactPresenceChanged(uid, kapp->dcopClient()->appId(), presence);
//	kdDebug() << "Presence changed for uid " << uid << " to " << presence << endl;
}

void Addressbook::emitContactPresenceChanged(const QString &uid) {
	if(uid.isEmpty()) {
		kdDebug() << "Addressbook::emitContactPresenceChanged was called with empty uid" << endl;
		return;
	};
	
	emitContactPresenceChanged(uid, presenceStatus(uid));
}

}//NAMESPACE

#include "addressbook.moc"

