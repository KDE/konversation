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
	KABC::StdAddressBook::setAutomaticSave( false );
	addressBook = KABC::StdAddressBook::self(true);
	m_ticket=NULL;
}

Addressbook *Addressbook::self() {
     if (!m_instance) { sd.setObject(m_instance, new Addressbook()); }
     return m_instance;
}

KABC::AddressBook *Addressbook::getAddressBook() { return addressBook; }



KABC::Addressee Addressbook::getKABCAddresseeFromNick(const QString &ircnick, const QString &servername, const QString &servergroup) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	
	for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
		if(hasNick(*it, ircnick, servername, servergroup))
			return (*it);
	}
	return KABC::Addressee();
}
KABC::Addressee Addressbook::getKABCAddresseeFromNick(const QString &nick_server) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	
	for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
		if(hasNick(*it, nick_server))
			return (*it);
	}
	return KABC::Addressee();
}
bool Addressbook::hasNick(const KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
	
	QString lnick = ircnick.lower();
	QString lnick_servername;
	QString lnick_servergroup;
	if(!servername.isEmpty())
		lnick_servername = lnick + QChar(0xE120) + servername.lower();
	if(!servergroup.isEmpty())
		lnick_servergroup = lnick + QChar(0xE120) + servergroup.lower();
		
	QString lit;
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		lit = (*it).lower();
		if(lit == lnick || lit == lnick_servername || lit == lnick_servergroup)
			return true;
	}
	return false;

}

bool Addressbook::hasNick(const KABC::Addressee &addressee, const QString &nick_server) {
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		if((*it).lower() ==nick_server)
			return true;
	}
	return false;

}

QString Addressbook::getBestNick(const KABC::Addressee &addressee) {
	//Look for a nickinfo for this nick, and use that.  That way we turn a nick that is online.
	NickInfoPtr nickInfo = getNickInfo(addressee, true);
	if(nickInfo)
		return nickInfo->getNickname();
	//No online nickinfo - not connected to server maybe.  just return the first nick.
	
	
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	if(!addresses.empty())
		return addresses.first();
	//No irc nicks- nothing left to try - return null
	return QString::null;
}

NickInfoPtr Addressbook::getNickInfo(const KABC::Addressee &addressee, bool onlineOnlyNicks)
{
	NickInfoPtr lastNickInfo;
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it ) {
		QString ircnick;
		QString serverOrGroup;
		KonversationApplication::splitNick_Server(*it, ircnick, serverOrGroup);
		NickInfoPtr nickInfo;
		
		if(onlineOnlyNicks) {
			nickInfo = dynamic_cast<KonversationApplication*>(kapp)->getOnlineNickInfo(ircnick, serverOrGroup);
			if(nickInfo) {
				if(!nickInfo->isAway())
					return nickInfo;
				//This nick is away.  Keep looking, but use it if we can't find one that's on
				lastNickInfo = nickInfo;
			}
		} else {
			nickInfo = dynamic_cast<KonversationApplication*>(kapp)->getNickInfo(ircnick, serverOrGroup);
			if(nickInfo) return nickInfo;
		}
	}
	//Use a nick that's either away, or non-existant.
	return lastNickInfo;
}

bool Addressbook::hasAnyNicks(const KABC::Addressee &addressee, const QString &/*server*/) {
	return !addressee.custom("messaging/irc", "All").isEmpty();
}
/** For a given contact, remove the ircnick if they have it. If you
 *  pass an addressBook, the contact is inserted if it has changed. 
 */
void Addressbook::unassociateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
	
	kdDebug() << "in unassociatenick for '" << ircnick << "'" << endl;
	if(ircnick.isEmpty()) return;
	
	QString lnick = ircnick.lower();
	QString lnick_servername;
	QString lnick_servergroup;
	if(!servername.isEmpty())
		lnick_servername = lnick + QChar(0xE120) + servername.lower();
	if(!servergroup.isEmpty())
		lnick_servergroup = lnick + QChar(0xE120) + servergroup.lower();  
	
	//We should now have lnick = ircnick, and versions with servername and servergroup - 
	// like johnflux, johnflux@freenode, or johnflux@irc.kde.org    except with the unicode
	// seperator char 0xe120 instead of the @
	
	kdDebug() << "nick" << ircnick<< endl;
	bool changed = false;
	if(addressee.isEmpty()) {
		kdDebug() << "Ignoring unassociation command for empty addressee for nick " << ircnick << endl;
	}
	QString lit;
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator it = addresses.begin();
	while(it != addresses.end()) {
		lit = (*it).lower();
		if(lit == lnick || lit == lnick_servername || lit == lnick_servergroup) {
			changed = true;
			QStringList::iterator todeleteit = ++it;
			addresses.remove(todeleteit);
		} else {
			it++;
		}
	}
	if(!changed)
		return;
	
	//if(!getAndCheckTicket())
	//	return;
	QString new_custom = addresses.join( QChar( 0xE000 ));
	if(new_custom.isEmpty())
		addressee.removeCustom("messaging/irc", "All");
	else
		addressee.insertCustom("messaging/irc", "All", new_custom);

	addressBook->insertAddressee(addressee);
	//saveTicket();
}

/**For a given contact, adds the ircnick if they don't already have it.  If you pass an addressBook, the contact is inserted
 * if it has changed. */
void Addressbook::associateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
	//It's easiest to just remove it from the list if it's there already
	unassociateNick(addressee, ircnick, servername, servergroup);
	QString nick_server = ircnick;
	if(!servergroup.isEmpty())
		nick_server += QChar(0xE120) + servergroup;
	else if(!servername.isEmpty())
		nick_server += QChar(0xE120) + servername;
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	addresses.append(nick_server);
	addressee.insertCustom("messaging/irc", "All", addresses.join( QChar( 0xE000 )));
	
	addressBook->insertAddressee(addressee);
}
/** This function associates the nick for a person, then iterates over all the contacts unassociating the nick from everyone else. It saves the addressses that have changed.*/
bool Addressbook::associateNickAndUnassociateFromEveryoneElse(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if((*it).uid() != addressee.uid())
			unassociateNick(*it, ircnick, servername, servergroup);
	associateNick(addressee, ircnick, servername, servergroup);
	return true;;
}

bool Addressbook::getAndCheckTicket() {
	if(m_ticket) {
		kdError() << "Internal error - getting new ticket without saving old" << endl;
		return false;
	}
	m_ticket = addressBook->requestSaveTicket();
	if ( !m_ticket ) {
		kdError() << "Resource is locked by other application!" << endl;
		//emit error
		return false;
	}
	kdDebug() << "gotTicketSuccessfully" << endl;
	return true;
}
void Addressbook::releaseTicket() {
	addressBook->releaseSaveTicket(m_ticket);
	m_ticket = NULL;			
}

bool Addressbook::saveTicket() {
	if(!addressBook->save( m_ticket) )
	{
		kdError() << "Saving failed!" << endl;
		addressBook->releaseSaveTicket(m_ticket);
		m_ticket = NULL;
		return false;
	}
	kdDebug() << "saveTicket() was successful" << endl;
	m_ticket= NULL;
	emit addresseesChanged();
	return true;
}

bool Addressbook::saveAddressbook(){
	if(m_ticket) {
		kdError() << "Internal error - getting new ticket without saving old" << endl;
		return false;
	}
	m_ticket = addressBook->requestSaveTicket();
	if ( !m_ticket )
	{
		kdError() << "Resource is locked by other application!" << endl;
		return false;
	}
	else
	{
		if ( !addressBook->save( m_ticket ) )
		{
			kdError() << "Saving failed!" << endl;
			addressBook->releaseSaveTicket(m_ticket);
			m_ticket = NULL;
			return false;
		}
	}
	kdDebug() << "Save was successful" << endl;	
	m_ticket = NULL;
	emit addresseesChanged();
	return true;
}

bool Addressbook::saveAddressee(KABC::Addressee &addressee) {
	Q_ASSERT(&addressee);
	addressBook->insertAddressee(addressee);
	bool success = saveAddressbook();
	if(success)
		emitContactPresenceChanged(addressee.uid(), presenceStatus(addressee));
	return success;
}

/**
 * Indicate the presence as a number.  Checks all the nicks that that person has.
 * If we find them, return 4 (online).
 * If we are connected to any of the servers that they are on, and we don't find them, return 1 (offline)
 * If we find them, but have the string "[^a-zA-Z]away[^a-zA-Z]" in their nick, then return 3 (away)
 * If we are connected to none of the servers that they are on, return 0 (unknown)
 * @param Addressbook contact you want to know of the presence of
 * @return 0 (unknown), 1 (offline), 3 (away), 4 (online)
 */
int Addressbook::presenceStatus(const KABC::Addressee &addressee) {
	Q_ASSERT(&addressee);
	int presenceStat = 0;
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		int presence = presenceStatusByNick(*it, "");
		if(presence == 4) 
			return 4; //The ultimate goal - online and not away.
		if(presence == 3)
			return 3; //Be happy with away as well.  Or should we keep searching for an online? hmm
		if(presence == 1)
			presenceStat = 1; //well.. at least we are connected to their server. But lets try the other nicks
		//Otherwise unknown - keep going, might find better.
	}
	return presenceStat;
}


/**
 * Indicate the presence as a number.
 * @param ircnick of the person we want to know if they are online.
 * @param server name of the person that the ircnick is for.  Set to null or empty for all servers
 * @return 0 (we aren't connected to the server), 1 (offline), 3 (away), 4 (online)
 */
int Addressbook::presenceStatusByNick(const QString &ircnick, const QString &server) {
	if(ircnick.isEmpty() /* || server.isEmpty()*/) {  //server stuff not implemented yet.  we need to hurry up on this. FIXME
		kdDebug() << "presenceStatusByNick called, but ircnick or server was empty/null" << endl;
		return 0;
	}
	bool foundaserver = false;
	QPtrList<Server> serverlist;
	serverlist =dynamic_cast<KonversationApplication*>(kapp)->getServerList();
	Server* lookServer=serverlist.first();
	while(lookServer)
	{
		if(lookServer->getServerName()==server || server.isEmpty())
		{
			if(lookServer->isNickOnline(ircnick)) {
				//Found nick..  are they online?
				if(ircnick.find( QRegExp( "[^a-zA-Z]away[^a-zA-Z]", FALSE)) >= 0)
					return 3; //AWAY
				return 4; //ONLINE
			}
			foundaserver = true;
		}
		lookServer = serverlist.next();
	}
	if(foundaserver) return 1;
	return 0;
}



QStringList Addressbook::allContacts() {
	QStringList contactUIDS;
	for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if(hasAnyNicks(*it,"")) contactUIDS.append((*it).uid());
	return contactUIDS;
}
//Produces a string list of all the irc nicks that are known.
QStringList Addressbook::allContactsNicks() {
	QStringList contacts;
	for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		contacts += QStringList::split( QChar( 0xE000 ), (*it).custom("messaging/irc", "All") );
	return contacts;
}

bool Addressbook::isOnline(KABC::Addressee &addressee) {
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
		if(isOnline(*it, "")) return true;
	return false;
}

bool Addressbook::isOnline(const QString &ircnick, const QString &server) {
	if(presenceStatusByNick(ircnick, server) >=3)
		return true;
	return false;
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
	return hasAnyNicks(addressBook->findByUid(uid), "");
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
	return presenceStatus(addressBook->findByUid(uid));
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

}

#include "addressbook.moc"

