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


namespace Konversation {

	
Addressbook *Addressbook::m_instance=0L;

Addressbook::Addressbook()
{
	KABC::StdAddressBook::setAutomaticSave( true );
	addressBook = KABC::StdAddressBook::self(true);
	m_ticket=NULL;
}

Addressbook *Addressbook::self() {
     if (!m_instance) { sd.setObject(m_instance, new Addressbook()); }
     return m_instance;
}

KABC::AddressBook *Addressbook::getAddressBook() { return addressBook; }
    

KABC::Addressee Addressbook::getKABCAddresseeFromNick(const QString &ircnick) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	
	for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
		if(hasNick(*it, ircnick))
			return (*it);
	}
	return KABC::Addressee();
}

bool Addressbook::hasNick(const KABC::Addressee &addressee, const QString &ircnick) {
	QString lower_ircnick = ircnick.lower();
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		if((*it).lower() == lower_ircnick)
			return true;
	}
	return false;
}
/** For a given contact, remove the ircnick if they have it. If you pass an addressBook, the contact is inserted
 *  if it has changed. */
void Addressbook::unassociateNick(KABC::Addressee &addressee, const QString &ircnick) {
	
	kdDebug() << "in unassociatenick" << endl;
	
	QString lower_ircnick = ircnick.lower();
	
	kdDebug() << "nick" << lower_ircnick<< endl;
	bool changed = false;
	if(addressee.isEmpty()) {
		kdDebug() << "Ignoring unassociation command for empty addressee for nick " << ircnick << endl;
	}
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	kdDebug() << "irc address list is:  " << addressee.custom("messaging/irc", "All") << endl; 
	for ( QStringList::iterator it = addresses.begin(); it != addresses.end(); ++it )
	{
		kdDebug() << "found a nick" << endl;
		kdDebug() << "it is " << (*it) << endl;
		if((*it).lower() == lower_ircnick) {
			changed = true;
			addresses.remove(it);
			break; //We have to break.  Our iterator is not safe now.
		}
	}
	if(!changed)
		return;
	
	if(!getAndCheckTicket())
		return;
	QString new_custom = addresses.join( QChar( 0xE000 ));
	if(new_custom.isEmpty())
		addressee.removeCustom("messaging/irc", "All");
	else
		addressee.insertCustom("messaging/irc", "All", new_custom);
	kdDebug() << "final irc address is '" << new_custom << "'" << endl;

	addressBook->insertAddressee(addressee);
	saveTicket();
}

/**For a given contact, adds the ircnick if they don't already have it.  If you pass an addressBook, the contact is inserted
 * if it has changed. */
void Addressbook::associateNick(KABC::Addressee &addressee, const QString &ircnick) {
	QString lower_ircnick = ircnick.lower();
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		if((*it).lower() == lower_ircnick) {
			return; //It's already there.  No need to do anything.
		}
	}
	if(!getAndCheckTicket()) return;
	addresses.append(ircnick);
	addressee.insertCustom("messaging/irc", "All", addresses.join( QChar( 0xE000 )));
	
	addressBook->insertAddressee(addressee);
	saveTicket();
}
/** This function associates the nick for a person, then iterates over all the contacts unassociating the nick from everyone else. It saves the addressses that have changed.*/
bool Addressbook::associateNickAndUnassociateFromEveryoneElseAndSave(KABC::Addressee &addressee, const QString &ircnick) {
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if((*it).uid() != addressee.uid())
			unassociateNick(*it, ircnick);
	associateNick(addressee, ircnick);
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
	addressBook->insertAddressee(addressee);
	return saveAddressbook();
}
    

}

#include "addressbook.moc"
