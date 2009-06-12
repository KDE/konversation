/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class gives function that interact with kaddressbook.
  begin:     Fri 2004-07-23
  copyright: (C) 2004 by John Tapsell
  email:     john@geola.co.uk
*/

#include "addressbook.h"
#include "viewcontainer.h"
#include "mainwindow.h"
#include "server.h"
#include "channel.h"
#include "application.h"

#include <QStringList>

#include <KMessageBox>
#include <KWindowSystem>


namespace Konversation
{
    struct AddressbookSingleton
    {
        Addressbook addressBook;
    };
}

K_GLOBAL_STATIC(Konversation::AddressbookSingleton, s_addessbook)

namespace Konversation
{
    Addressbook::Addressbook()
    {
        addressBook = KABC::StdAddressBook::self(true);
        m_ticket=NULL;
    }
    Addressbook::~Addressbook()
    {
    }

    Addressbook *Addressbook::self()
    {
        return &s_addessbook->addressBook;
    }

    QStringList Addressbook::allContacts()
    {
        QStringList contactUIDS;
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
            if(hasAnyNicks(*it)) contactUIDS.append((*it).uid());
        return contactUIDS;
    }
    //Produces a string list of all the irc nicks that are known.
    QStringList Addressbook::allContactsNicks()
    {
        QStringList contacts;
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
            contacts += (*it).custom("messaging/irc", "All").split(QChar(0xE000),  QString::SkipEmptyParts);
        return contacts;
    }

    QStringList Addressbook::onlineContacts()
    {
        QStringList contactUIDS;
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
            if(isOnline(*it)) contactUIDS.append((*it).uid());

        return contactUIDS;
    }
    QStringList Addressbook::reachableContacts()
    {
        return onlineContacts();
    }
    QStringList Addressbook::fileTransferContacts()
    {
        return onlineContacts();
    }
    bool Addressbook::isPresent(const QString &uid)
    {
        return hasAnyNicks(addressBook->findByUid(uid));
    }
    QString Addressbook::displayName(const QString &uid)
    {
        return getBestNick(addressBook->findByUid(uid));
    }
    QString Addressbook::presenceString(const QString &uid)
    {
        if(uid.isEmpty())
        {
            kDebug() << "Called with an empty uid";
            return QString("Error");
        }
        switch( presenceStatus(uid))
        {
            case 0:
                return "";
            case 1:
                return i18n("Offline");
            case 2:
                return i18n("Connecting");        //Shouldn't happen - not supported.
            case 3:
                return i18n("Away");
            case 4:
                return i18n("Online");
        }
        return i18n("Error");
    }
    int Addressbook::presenceStatus(const QString &uid)
    {
        return presenceStatusByAddressee(addressBook->findByUid(uid));
    }

    bool Addressbook::canReceiveFiles(const QString &uid)
    {
        if(uid.isEmpty())
        {
            kDebug() << "Called with empty uid";
            return false;
        }
        int presence = presenceStatus(uid);

        return (presence == 4) || (presence == 3);
    }
    bool Addressbook::canRespond(const QString &uid)
    {
        if(uid.isEmpty())
        {
            kDebug() << "Called with empty uid";
            return false;
        }
        //this should return false if they are offline.
        int result = presenceStatus(uid);
        if(result == 3 || result == 4) return true;
        return false;
    }
    QString Addressbook::locate(const QString &contactId, const QString &protocol)
    {
        if(contactId.isEmpty())
        {
            kDebug() << "Called with empty contactId";
            return QString();
        }
        if(protocol != "messaging/irc")
            return QString();

        return getKABCAddresseeFromNick(contactId).uid();
    }
    QPixmap Addressbook::icon(const QString &uid)
    {

        Images* icons = Application::instance()->images();
        QIcon currentIcon;
        if(!isPresent(uid))
            return QPixmap();

        switch(presenceStatus(uid))
        {
            case 0:                               //Unknown
            case 1:                               //Offline
            case 2:                               //connecting - invalid for us?
                currentIcon = icons->getKimproxyOffline();
                break;
            case 3:                               //Away
                currentIcon = icons->getKimproxyAway();
                break;
            case 4:                               //Online
                currentIcon = icons->getKimproxyOnline();
                break;
            default:
                //error
                kDebug() << "Unknown status " << uid;
                return QPixmap();
        }

        QPixmap joinedIcon = currentIcon.pixmap(22, QIcon::Active, QIcon::On);
        return joinedIcon;
    }
    QString Addressbook::context(const QString &uid)
    {
        if(uid.isEmpty())
        {
            kDebug() << "Called with empty uid";
            return QString();
        }
        QString context;
        return context;
    }
    QStringList Addressbook::protocols()
    {
        QStringList protocols;
        protocols.append("messaging/irc");
        return protocols;
    }

    // ACTORS
    /**
     * Send a single message to the specified addressee
     * Any response will be handled by the IM client as a normal
     * conversation.
     * @param uid the KABC uid you want to chat with.
     * @param message the message to send them.
     */
    void Addressbook::messageContact( const QString &uid, const QString& message )
    {
        if(uid.isEmpty())
        {
            focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but did not specify any contact to send the message to.  This is probably a bug in the other application."));
            return;
        }
        KABC::Addressee addressee = addressBook->findByUid(uid);
        if(addressee.isEmpty())
        {
            focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but Konversation could not find the specified contact in the KDE address book."));
            return;
        }
        NickInfoPtr nickInfo = getNickInfo(addressee);
        if(!nickInfo)
        {
            QString user = addressee.fullEmail();
            if(!user.isEmpty()) user = " (" + user + ')';
            focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but the requested user %1 is not online.",user));
            return;
        }

        nickInfo->getServer()->dbusSay(nickInfo->getNickname(), message);
    }

    /**
     * Open a chat to a contact, and optionally set some initial text
     */
    void Addressbook::messageNewContact( const QString &contactId, const QString &/*protocol*/ ) {
    if(contactId.isEmpty() )
    {
        kDebug() << "Called with empty contactid";
        focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but did not specify any contact to send the message to.  This is probably a bug in the other application."));
        return;
    }
    messageContact(contactId, QString());
}

/**
 * Start a chat session with the specified addressee
 * @param uid the KABC uid you want to chat with.
 */
void Addressbook::chatWithContact( const QString &uid )
{
    if(uid.isEmpty())
    {
        kDebug() << "Called with empty uid";
        focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation for instant messaging, but did not specify any contact to send the message to.  This is probably a bug in the other application."));
        return;
    }
    messageContact(uid, QString());
}

/**
 * Send the file to the contact
 * @param uid the KABC uid you are sending to.
 * @param sourceURL a KUrl to send.
 * @param altFileName an alternate filename describing the file
 * @param fileSize file size in bytes
 */
void Addressbook::sendFile(const QString &uid, const KUrl &sourceURL, const QString &altFileName, uint fileSize)
{
    if(uid.isEmpty())
    {
        focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to send a file to a contact, but did not specify any contact to send the file to.  This is probably a bug in the other application."));
        return;
    }
    KABC::Addressee addressee = addressBook->findByUid(uid);
    if(addressee.isEmpty())
    {
        focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to send a file to a contact, but Konversation could not find the specified contact in the KDE address book."));
        return;
    }
    NickInfoPtr nickInfo = getNickInfo(addressee);
    if(!nickInfo)
    {
        QString user = addressee.fullEmail();
        if(!user.isEmpty()) user = " (" + user + ')';
        focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to send a file to a contact, but the requested user %1 is not currently online.",user));
        return;
    }
    nickInfo->getServer()->addDccSend(nickInfo->getNickname(), sourceURL, altFileName, fileSize);
    QWidget *widget = nickInfo->getServer()->getViewContainer()->getWindow();
    KWindowSystem::demandAttention(widget->winId());       //If activeWindow request is denied, at least demand attention!
    KWindowSystem::activateWindow(widget->winId());        //May or may not work, depending on focus stealing prevention.

}

// MUTATORS
// Contact list
/**
 * Add a contact to the contact list
 * @param contactId the protocol specific identifier for the contact, eg UIN for ICQ, screenname for AIM, nick for IRC.
 * @param protocolId the protocol, eg one of "AIMProtocol", "MSNProtocol", "ICQProtocol", ...
 * @return whether the add succeeded.  False may signal already present, protocol not supported, or add operation not supported.
 */
bool Addressbook::addContact( const QString &/*contactId*/, const QString &/*protocolId*/ ) {
focusAndShowErrorMessage(i18n("Another KDE application tried to use Konversation to add a contact.  Konversation does support this."));
return false;
//Nicks are auto added if they are put in the addressbook - I don' think there is anything useful I can do.
}

void Addressbook::emitContactPresenceChanged(const QString &uid, int presence)
{
    if(uid.isEmpty())
    {
        //This warning below is annoying.  FIXME - disabled because it's too verbose
        //		kDebug() << "Addressbook::emitContactPresenceChanged was called with empty uid";
        return;
    }
    emit contactPresenceChanged(uid, QString::fromLatin1("org.kde.konversation"), presence);
    //	kDebug() << "Presence changed for uid " << uid << " to " << presence;
}

void Addressbook::emitContactPresenceChanged(const QString &uid)
{
    if(uid.isEmpty())
    {
        kDebug() << "Called with empty uid";
        return;
    };

    emitContactPresenceChanged(uid, presenceStatus(uid));
}

}                                                 //NAMESPACE

#include "addressbook.moc"
