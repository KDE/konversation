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
#include <qregexp.h>
#include "config.h"
#ifdef HAVE_KIMIFACE
#include "kimiface.h"
#else
#include "kimiface_fake.h"
#endif

#include "../images.h"

namespace Konversation {
class Addressbook : public QObject,public KIMIface
{
  Q_OBJECT
  public:
    KABC::Addressee getKABCAddresseeFromNick(const QString &ircnick);
    bool hasNick(const KABC::Addressee &addressee, const QString &ircnick);
    void unassociateNick(KABC::Addressee &addressee, const QString &ircnick);
    void associateNick(KABC::Addressee &addressee, const QString &ircnick);
    bool associateNickAndUnassociateFromEveryoneElse(KABC::Addressee &addressee, const QString &ircnick);
    QString getMainNick(const KABC::Addressee &addressee);
    bool hasAnyNicks(const KABC::Addressee &addresse, const QString &server);
    int presenceStatus(const KABC::Addressee &addressee);
    int presenceStatusByNick(const QString &ircnick, const QString &server);
    bool isOnline(KABC::Addressee &addressee);
    bool isOnline(const QString &ircnick, const QString &server);
    bool getAndCheckTicket();
    bool saveTicket();	
    void releaseTicket();
    bool saveAddressee(KABC::Addressee &addressee);
    bool saveAddressbook();
    KABC::AddressBook *getAddressBook();
    
    static Addressbook *self();
    QStringList allContacts();
    QStringList reachableContacts();
    QStringList onlineContacts();
    QStringList fileTransferContacts();
    bool isPresent( const QString &uid );
    QString displayName( const QString &uid );
    QString presenceString( const QString &uid );
    bool canReceiveFiles( const QString &uid );
    bool canRespond( const QString &uid );
    QString locate( const QString &contactId, const QString &protocol );
// metadata
    QPixmap icon( const QString &uid );
    QString context( const QString &uid );
    int presenceStatus(const QString &uid);
// App capabilities
    QStringList protocols();
    
    /**
     * Message a contact by their metaContactId, aka their uid in KABC.
     */
    void messageContact( const QString &uid, const QString& message );

    /**
     * Open a chat to a contact, and optionally set some initial text
     */
    void messageNewContact(  const QString &contactId, const QString &protocolId );

    /**
     * Message a contact by their metaContactId, aka their uid in KABC.
     */
    void chatWithContact( const QString &uid );

    /**
     * Send the file to the contact
     */
    void sendFile(const QString &uid, const KURL &sourceURL,
    const QString &altFileName = QString::null, uint fileSize = 0);

    void emitContactPresenceChanged( QString uid, int presence);
    void emitContactPresenceChanged(QString uid);
	

// MUTATORS
// Contact list
    bool addContact( const QString &contactId, const QString &protocolId );
    
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

