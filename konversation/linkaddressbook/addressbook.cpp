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

Addressbook::Addressbook()
{
	KABC::StdAddressBook::setAutomaticSave( false );
}

KABC::Addressee Addressbook::getKABCAddresseeFromNick(const QString &ircnick) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	
	KABC::AddressBook* addressBook = KABC::StdAddressBook::self( true );
	
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
void Addressbook::unassociateNick(KABC::Addressee addressee, const QString &ircnick, KABC::AddressBook* addressBook) {

	
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
	addressee.insertCustom("messaging/irc", "All", addresses.join( QChar( 0xE000 )));
	
	if(addressBook)
		addressBook->insertAddressee(addressee);
}

/**For a given contact, adds the ircnick if they don't already have it.  If you pass an addressBook, the contact is inserted
 * if it has changed. */
void Addressbook::associateNick(KABC::Addressee addressee, const QString &ircnick, KABC::AddressBook* addressBook) {
	QString lower_ircnick = ircnick.lower();
	QStringList addresses = QStringList::split( QChar( 0xE000 ), addressee.custom("messaging/irc", "All") );
	QStringList::iterator end = addresses.end();
	for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
	{
		if((*it).lower() == lower_ircnick) {
			return; //It's already there.  No need to do anything.
		}
	}
	addresses.append(ircnick);
	addressee.insertCustom("messaging/irc", "All", addresses.join( QChar( 0xE000 )));
	
	if(addressBook)
		addressBook->insertAddressee(addressee);
}
/** This function associates the nick for a person, then iterates over all the contacts unassociating the nick from everyone else. It saves the addressses that have changed.*/
bool Addressbook::associateNickAndUnassociateFromEveryoneElseAndSave(KABC::Addressee addressee, const QString &ircnick) {
	KABC::AddressBook* addressBook = KABC::StdAddressBook::self( true );
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
		if((*it).uid() != addressee.uid())
			unassociateNick(*it, ircnick, addressBook);
	associateNick(addressee, ircnick, addressBook);
	return saveAddressbook(addressBook);
}

bool Addressbook::saveAddressbook(KABC::AddressBook* addressBook){
	KABC::Ticket *ticket = addressBook->requestSaveTicket();
	if ( !ticket )
	{
		kdError() << "Resource is locked by other application!" << endl;
		return false;
	}
	else
	{
		if ( !addressBook->save( ticket ) )
		{
			kdError() << "Saving failed!" << endl;
			return false;
		}
	}
	return true;
}

bool Addressbook::saveAddressee(KABC::Addressee addressee) {
	KABC::AddressBook* addressBook = KABC::StdAddressBook::self( true );
	addressBook->insertAddressee(addressee);
	return saveAddressbook(addressBook);
}
    

}

