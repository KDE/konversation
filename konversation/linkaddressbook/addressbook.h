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
#include "kimiface.h"

#include "../images.h"
#include "../nickinfo.h"

namespace Konversation {
class Addressbook : public QObject,public KIMIface
{
  Q_OBJECT
  public:
    KABC::Addressee getKABCAddresseeFromNick(const QString &ircnick, const QString &servername, const QString &servergroup);
    KABC::Addressee getKABCAddresseeFromNick(const QString &nick_server);
    bool hasNick(const KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup);
    bool hasNick(const KABC::Addressee &addressee, const QString &nick_server);
    void unassociateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup);
    void associateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup);
    bool associateNickAndUnassociateFromEveryoneElse(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup);
    /** If this user is online, return one of the nicks that they are
      * using.  Otherwise return the first nick listed.
      * If there are multiple matches, it will prefer ones that are not set to away.
      * @return online nick, first nick, or QString::null if they aren't known at all.
    */
    QString getBestNick(const KABC::Addressee &addressee);
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
    QStringList allContactsNicks();	    
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
    /** Return a NickInfo for this addressee.
      *  If there are multiple matches, it tries to pick one that is not away.
      *  @param addressee The addressee to get a nickInfo for
      *  @param onlineOnlyNicks If true, then return only a nick that is online, otherwise return 0
      *  @return A nickInfo.  It tries hard to return a nickInfo that is not away if one exists.
      */
    static NickInfoPtr getNickInfo(const KABC::Addressee &addressee, bool onlineOnlyNicks);

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

