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

// Qt includes.
#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

// Konversation includes.
#include "server.h"
#include "serverison.h"
#include "addressbook.h"
#include "konversationapplication.h"
#include "nickinfo.h"
#include "konversationmainwindow.h"

ServerISON::ServerISON(Server* server) : m_server(server) {
  m_ISONList_invalid = true;
  //We need to know when the addressbook changes because if the info for an offline nick changes, 
  //we won't get a nickInfoChanged signal.
  connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL( addressBookChanged( AddressBook * ) ), 
    this, SLOT( addressbookChanged() ) );
  connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()),
    this, SLOT(addressbookChanged()));
  connect( m_server, SIGNAL(nickInfoChanged(Server*, const NickInfoPtr)),
    this, SLOT(nickInfoChanged(Server*, const NickInfoPtr)));
  connect(m_server->getMainWindow(), SIGNAL(prefsChanged()),
    this, SLOT(slotPrefsChanged()));
}
    
QStringList ServerISON::getISONList() {  
  if(m_ISONList_invalid)
    recalculateAddressees();
  return m_ISONList; 
}

QStringList ServerISON::getAddressees() { 
  if(m_ISONList_invalid)
    recalculateAddressees();
  return m_addresseesISON; 
}

KABC::Addressee ServerISON::getOfflineNickAddressee(QString& nickname)
{
  if(m_ISONList_invalid)
    recalculateAddressees();
  if (m_offlineNickToAddresseeMap.contains(nickname))
    return m_offlineNickToAddresseeMap[nickname];
  else
    return KABC::Addressee();
}

void ServerISON::recalculateAddressees()
{
  kdDebug() << "ServerISON::recalculateAddressees" << endl;
  // If not watching nicks, no need to build notify list.
  if (KonversationApplication::preferences.getUseNotify())
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
      NickInfoPtr nickInfo = nickInfoIt.data();
      KABC::Addressee addressee = nickInfo->getAddressee();
      if (!addressee.isEmpty())
      {
        QString uid = addressee.uid();
        QStringList nicknames = addresseeToOnlineNickMap[uid];
        nicknames.append(nickInfo->getNickname());
	//TODO: Check if this nick is in any channel, and if it is, don't add, since we know they are online if they are in the channel with us.
        addresseeToOnlineNickMap[uid] = nicknames;
      }
    }
  
    // Lowercase server name and server group.
    QString lserverName = m_server->getServerName().lower();
    QString lserverGroup = m_server->getServerGroup().lower();
  
    // Build notify list from nicks in addressbook, eliminating dups (case insensitive).
    QMap<QString,QString> ISONMap;
    m_offlineNickToAddresseeMap.clear();
    for( KABC::AddressBook::ConstIterator it = 
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
          for (unsigned int index=0; index<nicknames.count(); index++)
            ISONMap.insert(nicknames[index].lower(), nicknames[index], true);
        }
        else
        {
          // If addressee is not known to be online, add all of the nicknames
          // of the addressee associated with this server or server group (if any)
          // to the notify list.
          // Simultaneously, build a map of all offline nicks and corresponding
          // KABC::Addressee, indexed by lowercase nickname.
          QStringList nicks = QStringList::split( QChar( 0xE000 ),
            (*it).custom("messaging/irc", "All") );
          QStringList::ConstIterator nicksItEnd = nicks.constEnd();
          for( QStringList::ConstIterator nicksIt = nicks.constBegin();
            nicksIt != nicksItEnd; ++nicksIt )
          {
            QString lserverOrGroup = (*nicksIt).section(QChar(0xE120),1).lower();
            if(lserverOrGroup == lserverName || lserverOrGroup == lserverGroup ||
              lserverOrGroup.isEmpty())
            {
              QString nickname = (*nicksIt).section(QChar(0xE120),0,0);
              QString lcNickname = nickname.lower();
              ISONMap.insert(lcNickname, nickname, true);
              m_offlineNickToAddresseeMap.insert(lcNickname, *it, true);
            }
          }
        }
      }    
    }
    // The part of the ISON list due to the addressbook.
    m_addresseesISON = ISONMap.values();
    // Merge with watch list from prefs, eliminating dups (case insensitive).
    QStringList prefsWatchList =
      KonversationApplication::preferences.getNotifyListByGroup(m_server->getServerGroup());
    for (unsigned int index=0; index<prefsWatchList.count(); index++)
      ISONMap.insert(prefsWatchList[index].lower(), prefsWatchList[index], true);
    // Build final ISON list.
    m_ISONList = ISONMap.values();
  }
  else
  {
    m_addresseesISON.clear();
    m_ISONList.clear();
  }
  m_ISONList_invalid = false;
}

// When user changes preferences and has nick watching turned on, rebuild notify list.
void ServerISON::slotPrefsChanged()
{
  kdDebug() << "ServerISON::slotPrefsChanged" << endl;
  m_ISONList_invalid = true;
}

void ServerISON::nickInfoChanged(Server* /*server*/, const NickInfoPtr /*nickInfo*/) {
  //We need to call recalculateAddressees before returning m_ISONList
  m_ISONList_invalid = true;
}
void ServerISON::addressbookChanged() {	
  //We need to call recalculateAddressees before returning m_ISONList
  m_ISONList_invalid = true;
}

#include "serverison.moc"

