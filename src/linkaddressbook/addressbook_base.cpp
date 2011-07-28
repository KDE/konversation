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
#include "../irc/server.h"
#include "../application.h"
#include "../mainwindow.h"
#include "../irc/channel.h"


#include <KStringHandler>
#include <KRun>
#include <KMessageBox>
#include <KToolInvocation>
#include <KProcess>

namespace Konversation
{

    AddressbookBase::AddressbookBase()
    {
        KABC::StdAddressBook::setAutomaticSave( false );
        m_ticket=NULL;
    }

    AddressbookBase::~AddressbookBase()
    {
    }

    KABC::AddressBook *AddressbookBase::getAddressBook() { return addressBook; }

    KABC::Addressee AddressbookBase::getKABCAddresseeFromNick(const QString &ircnick, const QString &servername, const QString &servergroup)
    {
        KABC::AddressBook::Iterator it;

        for( it = addressBook->begin(); it != addressBook->end(); ++it )
        {
            if(hasNick(*it, ircnick, servername, servergroup))
                return (*it);
        }
        return KABC::Addressee();
    }
    KABC::Addressee AddressbookBase::getKABCAddresseeFromNick(const QString &nick_server)
    {
        KABC::AddressBook::Iterator it;

        for( it = addressBook->begin(); it != addressBook->end(); ++it )
        {
            if(hasNick(*it, nick_server))
                return (*it);
        }
        return KABC::Addressee();
    }

    bool AddressbookBase::hasNick(const KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup)
    {

        QString lnick = ircnick.toLower();
        QString lnick_servername;
        QString lnick_servergroup;
        if(!servername.isEmpty())
            lnick_servername = lnick + QChar(0xE120) + servername.toLower();
        if(!servergroup.isEmpty())
            lnick_servergroup = lnick + QChar(0xE120) + servergroup.toLower();

        QString lit;
        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        QStringList::iterator end = addresses.end();
        for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
        {
            lit = (*it).toLower();
            if(lit == lnick || lit == lnick_servername || lit == lnick_servergroup)
                return true;
        }
        return false;

    }

    QStringList AddressbookBase::allContactsNicksForServer(const QString &servername, const QString &servergroup)
    {
        QStringList contacts;
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
            contacts += getNicks(*it, servername, servergroup);
        return contacts;

    }

    QStringList AddressbookBase::getNicks(const KABC::Addressee &addressee, const QString &servername, const QString &servergroup)
    {
        QStringList nicks;

        QString lservername = servername.toLower();
        QString lservergroup = servergroup.toLower();

        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        QStringList::iterator end = addresses.end();
        for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
        {
            if(!(*it).contains(QChar( 0xE120)))
                nicks.append(*it);
            else
            {
                QString it_server = (*it).section(QChar( 0xE120), 0,0).toLower();
                if(it_server == lservername || it_server == lservergroup)
                    nicks.append((*it).section(QChar( 0xE120 ), 1,1));
            }
        }
        return nicks;
    }

    bool AddressbookBase::hasNick(const KABC::Addressee &addressee, const QString &nick_server)
    {
        QString lnick_server = nick_server.toLower();
        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        QStringList::iterator end = addresses.end();
        for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
        {
            QString it_server = (*it).section(QChar( 0xE120), 0,0).toLower();
            if(it_server ==lnick_server)
                return true;
        }
        return false;

    }

    QString AddressbookBase::getBestNick(const KABC::Addressee &addressee)
    {
        //Look for a nickinfo for this nick, and use that.  That way we turn a nick that is online.
        NickInfoPtr nickInfo = getNickInfo(addressee);
        if(nickInfo)
            return nickInfo->getNickname();
        //No online nickinfo - not connected to server maybe.  just return the first nick.

        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        if(!addresses.empty())
            return addresses.first();
        //No irc nicks- nothing left to try - return null
        return QString();
    }

    NickInfoPtr AddressbookBase::getNickInfo(const KABC::Addressee &addressee)
    {
        NickInfoPtr lastNickInfo;
        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        QStringList::iterator end = addresses.end();
        for ( QStringList::iterator it = addresses.begin(); it != end; ++it )
        {
            QString ircnick;
            QString serverOrGroup;
            Application::splitNick_Server(*it, ircnick, serverOrGroup);
            NickInfoPtr nickInfo =
                dynamic_cast<Application*>(kapp)->getNickInfo(ircnick, serverOrGroup);
            if(nickInfo)
            {
                if(!nickInfo->isAway())
                    return nickInfo;
                //This nick is away.  Keep looking, but use it if we can't find one that's on
                lastNickInfo = nickInfo;
            }
        }
        //Use a nick that's either away, or non-existent.
        return lastNickInfo;
    }

    bool AddressbookBase::hasAnyNicks(const KABC::Addressee &addressee)
    {
        return !addressee.custom("messaging/irc", "All").isEmpty();
    }
    void AddressbookBase::unassociateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup)
    {

        kDebug() << "in unassociatenick for '" << ircnick;
        if(ircnick.isEmpty()) return;

        QString lnick = ircnick.toLower();
        QString lnick_servername;
        QString lnick_servergroup;
        if(!servername.isEmpty())
            lnick_servername = lnick + QChar(0xE120) + servername.toLower();
        if(!servergroup.isEmpty())
            lnick_servergroup = lnick + QChar(0xE120) + servergroup.toLower();

        //We should now have lnick = ircnick, and versions with servername and servergroup -
        // like johnflux, johnflux@freenode, or johnflux@irc.kde.org    except with the unicode
        // separator char 0xe120 instead of the @

        kDebug() << "nick" << ircnick;
        bool changed = false;
        if(addressee.isEmpty())
        {
            kDebug() << "Ignoring unassociation command for empty addressee for nick " << ircnick;
        }
        QString lit;
        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        QStringList::iterator it = addresses.begin();
        while(it != addresses.end())
        {
            lit = (*it).toLower();
            if(lit == lnick || lit == lnick_servername || lit == lnick_servergroup)
            {
                changed = true;
                it = addresses.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if(!changed)
            return;

        //if(!getAndCheckTicket())
        //	return;
        QString new_custom = addresses.join( QChar( 0xE000 ));
        if(new_custom.isEmpty())
            addressee.removeCustom("messaging/irc", "All");
        else
            addressee.insertCustom("messaging/irc", "All", new_custom);

        addressBook->insertAddressee(addressee);
        //saveTicket();
    }
    void AddressbookBase::focusAndShowErrorMessage(const QString &errorMsg)
    {
        static_cast<Application *>(kapp)->getMainWindow()->focusAndShowErrorMessage(errorMsg);
    }
    /**For a given contact, adds the ircnick if they don't already have it.  If you pass an addressBook, the contact is inserted
     * if it has changed. */
    void AddressbookBase::associateNick(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup)
    {
        //It's easiest to just remove it from the list if it's there already
        unassociateNick(addressee, ircnick, servername, servergroup);
        QString nick_server = ircnick;
        if(!servergroup.isEmpty())
            nick_server += QChar(0xE120) + servergroup;
        else if(!servername.isEmpty())
            nick_server += QChar(0xE120) + servername;
        QStringList addresses = addressee.custom("messaging/irc", "All").split(QChar(0xE000), QString::SkipEmptyParts);
        //For sanity reasons, don't let the number of irc nicks go above 10.
        //To do this, just remove the irc nick at the end of the list.
        if(addresses.count() >= 10)
            addresses.pop_back();
        addresses.append(nick_server);
        addressee.insertCustom("messaging/irc", "All", addresses.join( QChar( 0xE000 )));

        addressBook->insertAddressee(addressee);
    }
    /** This function associates the nick for a person, then iterates over all the contacts unassociating the nick from everyone else. It saves the addressses that have changed.*/
    bool AddressbookBase::associateNickAndUnassociateFromEveryoneElse(KABC::Addressee &addressee, const QString &ircnick, const QString &servername, const QString &servergroup)
    {
        for( KABC::AddressBook::Iterator it = addressBook->begin(); it != addressBook->end(); ++it )
            if((*it).uid() != addressee.uid())
                unassociateNick(*it, ircnick, servername, servergroup);
        associateNick(addressee, ircnick, servername, servergroup);
        return true;;
    }

    bool AddressbookBase::getAndCheckTicket()
    {
        if(m_ticket)
        {
            kError() << "Internal error - getting new ticket without saving old" << endl;
            return false;
        }
        m_ticket = addressBook->requestSaveTicket();
        if ( !m_ticket )
        {
            kError() << "Resource is locked by other application!" << endl;
            //emit error
            return false;
        }
        kDebug() << "gotTicketSuccessfully";
        return true;
    }
    void AddressbookBase::releaseTicket()
    {
        addressBook->releaseSaveTicket(m_ticket);
        m_ticket = NULL;
    }

    bool AddressbookBase::saveTicket()
    {
        if(!addressBook->save( m_ticket) )
        {
            kError() << "Saving failed!" << endl;
            addressBook->releaseSaveTicket(m_ticket);
            m_ticket = NULL;
            return false;
        }
        kDebug() << "saveTicket() was successful";
        m_ticket= NULL;
        emit addresseesChanged();
        return true;
    }

    bool AddressbookBase::saveAddressbook()
    {
        if(m_ticket)
        {
            kError() << "Internal error - getting new ticket without saving old" << endl;
            return false;
        }
        m_ticket = addressBook->requestSaveTicket();
        if ( !m_ticket )
        {
            kError() << "Resource is locked by other application!" << endl;
            return false;
        }
        else
        {
            if ( !addressBook->save( m_ticket ) )
            {
                kError() << "Saving failed!" << endl;
                addressBook->releaseSaveTicket(m_ticket);
                m_ticket = NULL;
                return false;
            }
        }
        kDebug() << "Save was successful";
        m_ticket = NULL;
        emit addresseesChanged();
        return true;
    }

    bool AddressbookBase::saveAddressee(KABC::Addressee &addressee)
    {
        Q_ASSERT(&addressee);
        addressBook->insertAddressee(addressee);
        bool success = saveAddressbook();
        if(success)
            emitContactPresenceChanged(addressee.uid(), presenceStatusByAddressee(addressee));
        return success;
    }

    /**
     * Indicate the presence as a number.  Checks all the nicks that that person has.
     * If we find them, return 4 (online).
     * If we are connected to any of the servers that they are on, and we don't find them, return 1 (offline)
     * If we find them, but they are set to away then return 3 (away)
     * If we are connected to none of the servers that they are on, return 0 (unknown)
     * @param addressee Addressbook contact you want to know of the presence of
     * @return 0 (unknown), 1 (offline), 3 (away), 4 (online)
     */
    int AddressbookBase::presenceStatusByAddressee(const KABC::Addressee &addressee)
    {
        Q_ASSERT(&addressee);
        NickInfoPtr nickInfo = getNickInfo(addressee);

        if(!nickInfo)
        {
            if(hasAnyNicks(addressee))
                return 1;                         //either offline, or we aren't on the same server.
            else
                return 0;                         //Not known to us
        }
        if(nickInfo->isAway()) return 3;
        return 4;

    }
    bool AddressbookBase::isOnline(KABC::Addressee &addressee)
    {
        return !!getNickInfo(addressee);
    }

    bool AddressbookBase::editAddressee(const QString &uid)
    {
        Q_ASSERT(!uid.isEmpty());
        //FIXME:  This hack can be removed.  I fixed the below mentioned bug in kde 3.4 cvs - 2005-01-02
        //
        //Because of stupid bugs in kaddressbook, first load kaddressbook using startServiceByDesktopPath
        // then call it on the command line to actually put it in edit mode.  This is stupid :(
        KToolInvocation::startServiceByDesktopName( "kaddressbook" );

        KProcess *proc = new KProcess;
        *proc << "kaddressbook";
        *proc << "--editor-only" << "--uid" << uid;
        kDebug() << "running kaddressbook --editor-only --uid " << uid;
        if(!proc->startDetached())
        {
            KMessageBox::error(0, i18n("Could not run the address book program (kaddressbook) - this is most likely because it is not installed.  Please install the 'kdepim' packages."));
            return false;
        }
        return true;
    }

    bool AddressbookBase::sendEmail(const KABC::Addressee &addressee)
    {
        if(addressee.preferredEmail().isEmpty())
        {
            KMessageBox::sorry(0, i18n("The contact that you have selected does not have an email address associated with them. "), i18n("Cannot Send Email"));
            return false;
        }
        return runEmailProgram(addressee.fullEmail());
    }

    bool AddressbookBase::runEmailProgram(const QString &mailtoaddress)
    {
        KRun *proc = new KRun(KUrl(QString("mailto:") + KStringHandler::from8Bit(mailtoaddress.toAscii())),0);
        kDebug() << "Sending email to " << mailtoaddress;
        if(proc->hasError())
        {
            KMessageBox::error(0, i18n("Could not run your email program.  This is possibly because one is not installed.  To install the KDE email program (kmail) please install the 'kdepim' packages."));
            return false;
        }
        return true;

    }
    bool AddressbookBase::sendEmail(const NickInfoList& nicks)
    {
        if(nicks.isEmpty())
            return false;

        QString mailto;

        QStringList nicksWithoutAddressee;
        QStringList nicksWithoutEmails;
        QStringList nicksWithEmails;

        foreach(NickInfoPtr nickInfo, nicks)
        {
            if (nickInfo.isNull())
                continue;

            KABC::Addressee addr = nickInfo->getAddressee();
            if(addr.isEmpty())
            {
                nicksWithoutAddressee.append(nickInfo->getNickname());
            }
            else if(addr.preferredEmail().isEmpty())
            {
                nicksWithoutEmails.append(nickInfo->getNickname());
            }
            else
            {
                nicksWithEmails.append(nickInfo->getNickname());
                if(!mailto.isEmpty())
                    mailto += ", ";
                mailto += addr.fullEmail();
            }
        }
        if(!nicksWithoutAddressee.isEmpty() || !nicksWithoutEmails.isEmpty())
        {
            QString message;
            if(nicksWithoutEmails.isEmpty())
            {
                if(nicksWithEmails.isEmpty())
                {
                    if(nicksWithoutAddressee.count() > 1)
                        message = i18n("None of the selected contacts are associated with address book entries. ");
                    else
                        message = i18n("The selected contact is not associated with an address book entry. ");
                }
                else
                {
                    if(nicksWithoutAddressee.count() > 1)
                        message = i18n("Some of the contacts (%1) that you have selected are not associated with address book entries. ",nicksWithoutAddressee.join(", "));
                    else
                        message = i18n("One of the contacts (%1) that you have selected is not associated with an address book entry. ",nicksWithoutAddressee.join(", "));
                }
                message += i18n("You can right click on a contact and choose to edit the Address Book Associations to link them to a contact in your address book.");
            }
            else if(nicksWithoutAddressee.isEmpty())
            {
                if(nicksWithEmails.isEmpty())
                {
                    if(nicksWithoutEmails.count() > 1)
                        message = i18n("None of the selected contacts have an email address associated with them. ");
                    else
                        message = i18n("The selected contact does not have an associated email address. ");
                }
                else
                {
                    if(nicksWithoutEmails.count() > 1)
                        message = i18n("Some of the contacts (%1) that you have selected do not have email addresses associated with them. ",nicksWithoutEmails.join(", "));
                    else
                        message = i18n("One of the contacts (%1) that you have selected does not have an email address associated with them. ",nicksWithoutEmails.join(", "));
                }
                message += i18n("You can right click on a contact and edit the corresponding address book entry to add an email address for them.");
            }
            else
            {
                message = i18n("Some of the contacts (%1) that you have selected are not associated with address book entries, and some of the contacts (%2) do not have email addresses associated with them.  ",nicksWithoutAddressee.join(", "),nicksWithoutEmails.join(", "));
                message += i18n("For the former contacts, this can be resolved by right clicking on a contact and choosing to edit the Address Book Associations, thereby linking them to a contact in your address book.  For the latter, by choosing to editing the corresponding address book contact you can specify an email address for them.");
            }
            if(nicksWithEmails.isEmpty())
            {
                KMessageBox::sorry(0, message, i18n("Cannot Send Email"));
                return false;
            }
            else
            {
                message += i18n("\nDo you want to send an email anyway to the nicks that do have email addresses?");
                int result = KMessageBox::questionYesNo(0, message, i18n("Send Email"), KGuiItem(i18n("&Send Email...")), KGuiItem(i18n("&Cancel")));
                if(result == KMessageBox::No)
                {
                    return false;
                }
            }
        }

        return runEmailProgram(mailto);
    }

}                                                 // NAMESPACE

#include "addressbook_base.moc"
