/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  addressbook.cpp  - This class contains functions that interact with kaddressbook.
  begin:     Fri 2004-07-23
  copyright: (C) 2004 by John Tapsell
  email:     john@geola.co.uk
*/

#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

#include <kstaticdeleter.h> 
#include <qobject.h>

namespace Konversation {
class Addressbook : public QObject
{
  Q_OBJECT
  public:
    KABC::Addressee getKABCAddresseeFromNick(const QString &ircnick);
    bool hasNick(const KABC::Addressee &addressee, const QString &ircnick);
    void unassociateNick(KABC::Addressee &addressee, const QString &ircnick);
    void associateNick(KABC::Addressee &addressee, const QString &ircnick);
    bool associateNickAndUnassociateFromEveryoneElse(KABC::Addressee &addressee, const QString &ircnick);
    bool getAndCheckTicket();
    bool saveTicket();	
    void releaseTicket();
    bool saveAddressee(KABC::Addressee &addressee);
    bool saveAddressbook();
    KABC::AddressBook *getAddressBook();
    
    static Addressbook *self();
  private:
    Addressbook();
    static Addressbook *m_instance;

    KABC::AddressBook* addressBook;
    KABC::Ticket *m_ticket;
    
  signals:
    void addresseesChanged();
};


static KStaticDeleter<Addressbook> sd;

}

#endif

