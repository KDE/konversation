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

#ifndef ADDRESSBOOKBASE_H
#define ADDRESSBOOKBASE_H

#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

#include <kstaticdeleter.h> 
#include <qobject.h>
#include <qregexp.h>
#include "config.h"

#include "../images.h"
#include "../nickinfo.h"

namespace Konversation {
class AddressbookBase : public QObject, public KIMIface
{
  Q_OBJECT
  public:

    virtual ~AddressbookBase(); // This needs to be public so it can be deleted by our static pointer
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
    bool hasAnyNicks(const KABC::Addressee &addresse);
    int presenceStatusByAddressee(const KABC::Addressee &addressee);
    bool isOnline(KABC::Addressee &addressee);
    bool getAndCheckTicket();
    bool saveTicket();	
    void releaseTicket();
    bool saveAddressee(KABC::Addressee &addressee);
    bool saveAddressbook();
    KABC::AddressBook *getAddressBook();
    
    /** Return a NickInfo for this addressee.
      *  If there are multiple matches, it tries to pick one that is not away.
      *  @param addressee The addressee to get a nickInfo for
      *  @return A nickInfo.  It tries hard to return a nickInfo that is not away if one exists.
      */
    static NickInfoPtr getNickInfo(const KABC::Addressee &addressee);
    /**
      * Lets outsiders tell us to emit presenceChanged signal.
      */
    virtual void emitContactPresenceChanged( QString uid, int presence) = 0;
    /**
      * Lets outsiders tell us to emit presenceChanged signal.
      */
    virtual void emitContactPresenceChanged(QString uid) = 0;

  signals:
    void addresseesChanged();

  protected:
    AddressbookBase();
    KABC::AddressBook* addressBook;
    KABC::Ticket *m_ticket;
    
};

} //NAMESPACE

#endif

