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

	
AddressbookBase::AddressbookBase()
{
	KABC::StdAddressBook::setAutomaticSave( false );
	m_ticket=NULL;
}

AddressbookBase::~AddressbookBase() {
}

KABC::AddressBook *AddressbookBase::getAddressBook() { return addressBook; }



KABC::Addressee AddressbookBase::getKABCAddresseeFromNick(const QString &ircnick, const QString &servername, const QString &servergroup) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	
	for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
		if(hasNick(*it, ircnick, servername, servergroup))
			return (*it);
	}
	return KABC::Addressee();
}
KABC::Addressee AddressbookBase::getKABCAddresseeFromNick(const QString &nick_server) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	
	for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
		if(hasNick(*it, nick_server))
			return (*it);
	}
	return KABC::Addressee();
}
bool AddressbookBase::hasNick(const KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
	
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

bool AddressbookBase::hasNick(const KABC::Addressee &addressee, const QString &nick_server) {
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		if((*it).lower() ==nick_server)
			return true;
	}
	return false;

}

QString AddressbookBase::getBestNick(const KABC::Addressee &addressee) {
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



NickInfoPtr AddressbookBase::getNickInfo(const KABC::Addressee &addressee, bool onlineOnlyNicks)
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

bool AddressbookBase::hasAnyNicks(const KABC::Addressee &addressee) {
	return !addressee.custom("messaging/irc", "All").isEmpty();
}
/** For a given contact, remove the ircnick if they have it. If you
 *  pass an addressBook, the contact is inserted if it has changed. 
 */
void AddressbookBase::unassociateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
	
	kdDebug() << "in unassociatenick for '" << ircnick << endl;
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
			it = addresses.remove(it);
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
void AddressbookBase::associateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
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
bool AddressbookBase::associateNickAndUnassociateFromEveryoneElse(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup) {
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if((*it).uid() != addressee.uid())
			unassociateNick(*it, ircnick, servername, servergroup);
	associateNick(addressee, ircnick, servername, servergroup);
	return true;;
}

bool AddressbookBase::getAndCheckTicket() {
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
void AddressbookBase::releaseTicket() {
	addressBook->releaseSaveTicket(m_ticket);
	m_ticket = NULL;			
}

bool AddressbookBase::saveTicket() {
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

bool AddressbookBase::saveAddressbook(){
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

bool AddressbookBase::saveAddressee(KABC::Addressee &addressee) {
	Q_ASSERT(&addressee);
	addressBook->insertAddressee(addressee);
	bool success = saveAddressbook();
	if(success)
		emitContactPresenceChanged(addressee.uid(), presenceStatusByAddressee(addressee));
	return success;
}

/**
 * Indicate the presence as a number.  Checks all the nicks that that person has.
 * If we find them, return 4 (online).
 * If we are connected to any of the servers that they are on, and we don't find them, return 1 (offline)
 * If we find them, but they are set to away then return 3 (away)
 * If we are connected to none of the servers that they are on, return 0 (unknown)
 * @param Addressbook contact you want to know of the presence of
 * @return 0 (unknown), 1 (offline), 3 (away), 4 (online)
 */
int AddressbookBase::presenceStatusByAddressee(const KABC::Addressee &addressee) {
	Q_ASSERT(&addressee);
	NickInfoPtr nickInfo = getNickInfo(addressee, true /*online only*/);
	if(!nickInfo) return 1; //either offline, or we aren't on the same server.  returning 0 not supported at the moment.  FIXME
	if(nickInfo->isAway()) return 3;
	return 4;

}
bool AddressbookBase::isOnline(KABC::Addressee &addressee) {
	return !!getNickInfo(addressee, true);
}




}  // NAMESPACE 
#include "addressbook_base.moc"
