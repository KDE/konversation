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

namespace Konversation {

Addressbook::Addressbook()
{
	KABC::StdAddressBook::setAutomaticSave( false );
}

KABC::Addressee Addressbook::getKABCAddresseeFromNick(const QString &ircnick) {
	KABC::Addressee addr;
	KABC::AddressBook::Iterator it;
	QString lower_ircnick = ircnick.lower();
	
	KABC::AddressBook* addressBook = KABC::StdAddressBook::self( true );
	
	for( it = addressBook->begin(); it != addressBook->end(); ++it ) {
		if((*it).custom("messaging/irc", "All").lower() == lower_ircnick) 
			return (*it);
	}
	return KABC::Addressee();
}


}

