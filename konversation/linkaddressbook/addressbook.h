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

#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

namespace Konversation {
class Addressbook
{
  public:
    static KABC::Addressee getKABCAddresseeFromNick(const QString &ircnick);
    static bool hasNick(const KABC::Addressee &addressee, const QString &ircnick);
    static void unassociateNick(KABC::Addressee addressee, const QString &ircnick, KABC::AddressBook* addressBook);
    static void associateNick(KABC::Addressee addressee, const QString &ircnick, KABC::AddressBook* addressBook);
    static bool associateNickAndUnassociateFromEveryoneElseAndSave(KABC::Addressee addressee, const QString &ircnick);

    static bool saveAddressee(KABC::Addressee addressee);
    static bool saveAddressbook(KABC::AddressBook* addressBook);
	    
  private:
    Addressbook();
};
}

#endif
