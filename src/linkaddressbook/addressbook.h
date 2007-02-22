/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class contains functions that interact with kaddressbook.
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
#include "addressbook_base.h"

namespace Konversation
{
    class Addressbook : public AddressbookBase
    {
        Q_OBJECT
            public:

            virtual ~Addressbook();               // This needs to be public so it can be deleted by our static pointer
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
            virtual int presenceStatus(const QString &uid);
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
                const QString &altFileName = QString(), uint fileSize = 0);

            /**
             * Lets outsiders tell us to emit presenceChanged signal.
             */
            void emitContactPresenceChanged( const QString &uid, int presence);
            /**
             * Lets outsiders tell us to emit presenceChanged signal.
             */
            void emitContactPresenceChanged(const QString &uid);

            bool addContact( const QString &contactId, const QString &protocolId );

        protected:
            Addressbook();

            static Addressbook *m_instance;
    };

    static KStaticDeleter<Addressbook> sd;

}
#endif
