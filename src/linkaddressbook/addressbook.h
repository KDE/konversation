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


#include "../viewer/images.h"
#include "../irc/nickinfo.h"
#include "addressbook_base.h"

namespace Konversation
{
    struct AddressbookSingleton;

    class Addressbook : public AddressbookBase
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.KIM")

            friend struct AddressbookSingleton;

            public:

            virtual ~Addressbook();               // This needs to be public so it can be deleted by our static pointer
            static Addressbook *self();

        public slots:
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
            void sendFile(const QString &uid, const KUrl &sourceURL,
                const QString &altFileName = QString(), quint64 fileSize = 0);

        public:
            /**
             * Lets outsiders tell us to emit presenceChanged signal.
             */
            void emitContactPresenceChanged( const QString &uid, int presence);
            /**
             * Lets outsiders tell us to emit presenceChanged signal.
             */
            void emitContactPresenceChanged(const QString &uid);

        public slots:
            bool addContact( const QString &contactId, const QString &protocolId );

        signals:
            void contactPresenceChanged( const QString &uid, const QString &appId, int presence );

        protected:
            Addressbook();
    };

}
#endif
