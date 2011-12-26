/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverison.h  -  Class to give a list of all the nicks known to the
                   addressbook and watchednick list that are on this
           server.  There is one instance of this class for
           each Server object.
  begin:     Fri Sep 03 2004
  copyright: (C) 2004 by John Tapsell
  email:     john@geola.co.uk
*/

#include "serverison.h"
#include "server.h"
#include "addressbook.h"
#include "application.h"
#include "nickinfo.h"
#include "viewcontainer.h"

#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>


ServerISON::ServerISON(Server* server) : m_server(server)
{
    m_ISONList_invalid = true;
    //We need to know when the addressbook changes because if the info for an offline nick changes,
    //we won't get a nickInfoChanged signal.

    connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL(addressBookChanged(AddressBook*)),
        this, SLOT(addressbookChanged()) );
    connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()),
        this, SLOT(addressbookChanged()));

    connect( m_server, SIGNAL(nickInfoChanged(Server*,NickInfoPtr)),
        this, SLOT(nickInfoChanged(Server*,NickInfoPtr)));
    connect( m_server,
        SIGNAL(channelMembersChanged(Server*,QString,bool,bool,QString)),
        this,
        SLOT(slotChannelMembersChanged(Server*,QString,bool,bool,QString)));
    connect( m_server,
        SIGNAL(channelJoinedOrUnjoined(Server*,QString,bool)),
        this,
        SLOT(slotChannelJoinedOrUnjoined(Server*,QString,bool)));
    connect(Application::instance(), SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)),
        this, SLOT(slotServerGroupsChanged()));
}

QStringList ServerISON::getWatchList()
{
    if(m_ISONList_invalid)
        recalculateAddressees();
    return m_watchList;
}

QStringList ServerISON::getISONList()
{
    if(m_ISONList_invalid)
        recalculateAddressees();
    return m_ISONList;
}

QStringList ServerISON::getAddressees()
{
    if(m_ISONList_invalid)
        recalculateAddressees();
    return m_addresseesISON;
}

KABC::Addressee ServerISON::getOfflineNickAddressee(QString& nickname)
{
    QString lcNickname = nickname.toLower();
    if(m_ISONList_invalid)
        recalculateAddressees();
    if (m_offlineNickToAddresseeMap.contains(lcNickname))
        return m_offlineNickToAddresseeMap[lcNickname];
    else
        return KABC::Addressee();
}

void ServerISON::recalculateAddressees()
{
    // If not watching nicks, no need to build notify list.
    if (Preferences::self()->useNotify())
    {
        // Get all nicks known to be online.
        const NickInfoMap* allNicks = m_server->getAllNicks();
        // Build a map of online nicknames with associated addressbook entry,
        // indexed by KABC::Addressee uid.
        // Note that there can be more than one nick associated with an addressee.
        QMap<QString,QStringList> addresseeToOnlineNickMap;
        NickInfoMap::ConstIterator nickInfoItEnd = allNicks->constEnd();
        for(NickInfoMap::ConstIterator nickInfoIt=allNicks->constBegin();
            nickInfoIt != nickInfoItEnd; ++nickInfoIt)
        {
            NickInfoPtr nickInfo = (*nickInfoIt);
            KABC::Addressee addressee = nickInfo->getAddressee();
            if (!addressee.isEmpty())
            {
                QString uid = addressee.uid();
                QStringList nicknames = addresseeToOnlineNickMap[uid];
                nicknames.append(nickInfo->getNickname());
                addresseeToOnlineNickMap[uid] = nicknames;
            }
        }

        // Lowercase server name and server group.
        QString lserverName = m_server->getServerName().toLower();
        QString lserverGroup = m_server->getDisplayName().toLower();

        // Build notify list from nicks in addressbook, eliminating dups (case insensitive).
        QMap<QString,QString> ISONMap;
        m_offlineNickToAddresseeMap.clear();

        for( KABC::AddressBook::Iterator it =
            Konversation::Addressbook::self()->getAddressBook()->begin();
            it != Konversation::Addressbook::self()->getAddressBook()->end(); ++it )
        {
            if(Konversation::Addressbook::self()->hasAnyNicks(*it))
            {
                QString uid = (*it).uid();
                // First check if we already know that this addressee is online.
                // If so, add all the nicks of the addressee that are online, but do not
                // add the offline nicks.  There is no point in monitoring such nicks.
                if (addresseeToOnlineNickMap.contains(uid))
                {
                    QStringList nicknames = addresseeToOnlineNickMap[uid];
                    QStringList::iterator itEnd = nicknames.end();

                    for(QStringList::iterator it = nicknames.begin(); it != itEnd; ++it)
                    {
                        ISONMap.insert((*it).toLower(), (*it));
                    }
                }
                else
                {
                    // If addressee is not known to be online, add all of the nicknames
                    // of the addressee associated with this server or server group (if any)
                    // to the notify list.
                    // Simultaneously, build a map of all offline nicks and corresponding
                    // KABC::Addressee, indexed by lowercase nickname.
                    QStringList nicks = (*it).custom("messaging/irc", "All").split( QChar( 0xE000 ) );
                    QStringList::ConstIterator nicksItEnd = nicks.constEnd();
                    for( QStringList::ConstIterator nicksIt = nicks.constBegin();
                        nicksIt != nicksItEnd; ++nicksIt )
                    {
                        QString lserverOrGroup = (*nicksIt).section(QChar(0xE120),1).toLower();
                        if(lserverOrGroup == lserverName || lserverOrGroup == lserverGroup ||
                            lserverOrGroup.isEmpty())
                        {
                            QString nickname = (*nicksIt).section(QChar(0xE120),0,0);
                            QString lcNickname = nickname.toLower();
                            ISONMap.insert(lcNickname, nickname);
                            m_offlineNickToAddresseeMap.insert(lcNickname, *it);
                        }
                    }
                }
            }
        }
        // The part of the ISON list due to the addressbook.
        m_addresseesISON = ISONMap.values();
        // Merge with watch list from prefs, eliminating dups (case insensitive).
        // TODO: Don't add nick on user watch list if nick is known to be online
        // under a different nickname?
        QStringList prefsWatchList =
            Preferences::notifyListByGroupId(m_server->getServerGroup()->id());
        QStringList::iterator itEnd = prefsWatchList.end();

        for(QStringList::iterator it = prefsWatchList.begin(); it != itEnd; ++it)
        {
            ISONMap.insert((*it).toLower(), (*it));
        }

        // Build final watch list.
        m_watchList = ISONMap.values();
        // Eliminate nicks that are online in a joined channel, since there is no point
        // in doing an ISON on such nicks.
        m_ISONList.clear();
        itEnd = m_watchList.end();

        for(QStringList::iterator it = m_watchList.begin(); it != itEnd; ++it)
        {
            if (m_server->getNickJoinedChannels(*it).isEmpty())
            {
                m_ISONList.append(*it);
            }
        }
    }
    else
    {
        m_addresseesISON.clear();
        m_ISONList.clear();
    }

    m_ISONList_invalid = false;
}

// When user changes preferences and has nick watching turned on, rebuild notify list.
void ServerISON::slotServerGroupsChanged()
{
    kDebug();
    m_ISONList_invalid = true;
}

void ServerISON::nickInfoChanged(Server* /*server*/, const NickInfoPtr /*nickInfo*/) {
//We need to call recalculateAddressees before returning m_ISONList

//Maybe we could do something like:
//if(m_ISONList.contains(nickInfo->getNickName())) return;
m_ISONList_invalid = true;
}

void ServerISON::addressbookChanged()
{
    //We need to call recalculateAddressees before returning m_ISONList
    m_ISONList_invalid = true;
}

void ServerISON::slotChannelMembersChanged(Server* /*server*/, const QString& /*channelName*/,
bool joined, bool parted, const QString& nickname)
{
    // Whenever a nick on the watch list leaves the last joined channel, must recalculate lists.
    // The nick will be added to the ISON list.
    if (joined && parted && m_watchList.contains(nickname))
        if (m_server->getNickJoinedChannels(nickname).isEmpty()) m_ISONList_invalid = true;
}

void ServerISON::slotChannelJoinedOrUnjoined(Server* /*server*/,
const QString& /*channelName*/, bool /*joined*/)
{
    // If user left or joined a channel, need to recalculate lists, since watched nicks
    // may need to be moved from/to ISON list.
    m_ISONList_invalid = true;
}

#include "serverison.moc"
