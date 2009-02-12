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

#include "../viewer/images.h"
#include "../irc/nickinfo.h"
#include "../irc/channelnick.h"

#include <qobject.h>
#include <qregexp.h>


#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>


namespace Konversation
{
    class AddressbookBase : public QObject//, public KIMIface
    {
        Q_OBJECT
            public:
            virtual ~AddressbookBase();           // This needs to be public so it can be deleted by our static pointer
            KABC::Addressee getKABCAddresseeFromNick(const QString &ircnick, const QString &servername, const QString &servergroup);
            KABC::Addressee getKABCAddresseeFromNick(const QString &nick_server);
            bool hasNick(const KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup);
            bool hasNick(const KABC::Addressee &addressee, const QString &nick_server);

            /** For a given contact, remove the ircnick if they have it. If you
             *  pass an addressBook, the contact is inserted if it has changed.
             */
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

            QStringList allContactsNicksForServer(const QString &servername, const QString &servergroup);
            QStringList getNicks(const KABC::Addressee &addressee, const QString &servername, const QString &servergroup);

            KABC::AddressBook *getAddressBook();

            /**  Return an online NickInfo for this addressee.
             *  If there are multiple matches, it tries to pick one that is not away.
             *  Note: No NickInfo is returned if the addressee is offline.
             *  NickInfo's are for online and away nicks only.
             *  @param addressee The addressee to get a nickInfo for
             *  @return A nickInfo.  It tries hard to return a nickInfo that is not away if one exists.
             */
            static NickInfoPtr getNickInfo(const KABC::Addressee &addressee);
            /**
             * Lets outsiders tell us to emit presenceChanged signal.
             */
            virtual void emitContactPresenceChanged(const QString &uid, int presence) = 0;
            /**
             * Lets outsiders tell us to emit presenceChanged signal.
             */
            virtual void emitContactPresenceChanged(const QString &uid) = 0;

            /**
             *  Run kmail (or whatever the users email client is)
             *  to create a single email addressed to all of the nicks passed in.
             *  Gives an error dialog to the user if any of the contacts don't have an
             *  email address associated, and gives the user the option to continue
             *  with the contacts that did have email addresses.
             */
            bool sendEmail(const ChannelNickList &nicklist);
            /**
             *  Run kmail (or whatever the users email client is)
             *  to create a single email addressed to the addressee passed in.
             *  Gives an error dialog to the user if the addressee doesn't have an email address associated.
             */
            bool sendEmail(const KABC::Addressee &addressee);
            /**
             *  Run kaddressbook to edit the addressee passed in.
             */
            bool editAddressee(const QString &uid);
            /**
             *  Run the users email program (e.g. kmail) passing "mailto:" + mailtoaddress.
             *  Note that mailto:  will be prepended for you.
             *  @param mailtoaddress A comma delimited set of email address topass as "mailto:"
             *  @return True if there were no problems running the email program.  An error will be shown to the user if there was.
             */
            bool runEmailProgram(const QString &mailtoaddress);

            /** Just calls KonversationMainWindow::focusAndShowErrorMessage(const QString *errorMsg)
             *
             *  @see KonversationMainWindow::focusAndShowErrorMessage(const QString *errorMsg)
             */
            void focusAndShowErrorMessage(const QString &errorMsg);
            signals:
            void addresseesChanged();

        protected:
            AddressbookBase();
            KABC::AddressBook* addressBook;
            KABC::Ticket *m_ticket;
    };

}                                                 //NAMESPACE
#endif
