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
  connect( Konversation::Addressbook::self()->getAddressBook(),
    SIGNAL( addressBookChanged( AddressBook * ) ),
    this, SLOT( recalculateAddressees() ) );
  connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()),
    this, SLOT(recalculateAddressees()));
  connect( m_server, SIGNAL(nickInfoChanged(Server*, const NickInfoPtr)),
    this, SLOT(nickInfoChanged(Server*, const NickInfoPtr)));
  connect(m_server->getMainWindow(),SIGNAL(prefsChanged()),
    this,SLOT(slotPrefsChanged()));
}
    
QStringList ServerISON::getISONList() { return m_ISONList; }

QStringList ServerISON::getAddressees() { return m_addresseesISON; }

KABC::Addressee ServerISON::getOfflineNickAddresse(QString& nickname)
{
  if (m_offlineNickToAddresseeMap.contains(nickname))
    return m_offlineNickToAddresseeMap[nickname];
  else
    return KABC::Addressee();
}

void ServerISON::recalculateAddressees()
{
  kdDebug() << "ServerISON::recalculateAddressees" << endl;
  // Get all nicks known to be online.
  const NickInfoMap* allNicks = m_server->getAllNicks();
  // If not watching nicks, no need to build notify list.
  if (m_useNotify)
  {
    // Build a map of online nicknames with associated addressbook entry,
    // indexed by KABC::Addressee uid.
    // Note that there can be more than one nick associated with an addressee.
    QMap<QString,QStringList> addresseeToOnlineNickMap;
    NickInfoMap::ConstIterator nickInfoItEnd = allNicks->end();
    for(NickInfoMap::ConstIterator nickInfoIt=allNicks->begin();
      nickInfoIt != nickInfoItEnd; ++nickInfoIt)
    {
      NickInfoPtr nickInfo = nickInfoIt.data();
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
    QString lserverName = m_server->getServerName().lower();
    QString lserverGroup = m_server->getServerGroup().lower();
  
    // Build notify list from nicks in addressbook.
    m_addresseesISON.clear();
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
            m_addresseesISON.append(nicknames[index]);
        }
        else
        {
          // If addressee is not known to be online, add all of the nicknames
          // of the addressee associated with this server or server group (if any)
          // to the notify list.
          // Simultaneously, build a map of all offline nicks and corresponding
          // KABC::Addressee, indexed by nickname.
          QStringList nicks = QStringList::split( QChar( 0xE000 ),
            (*it).custom("messaging/irc", "All") );
          for( QStringList::Iterator nicksIt = nicks.begin(); nicksIt != nicks.end(); ++nicksIt )
          {
            QString lserverOrGroup = (*nicksIt).section(QChar(0xE120),1).lower();
            if(lserverOrGroup == lserverName || lserverOrGroup == lserverGroup ||
              lserverOrGroup.isEmpty())
            {
              QString nickname = (*nicksIt).section(QChar(0xE120),0,0).lower();
              m_addresseesISON.append(nickname);
              m_offlineNickToAddresseeMap.insert(nickname, *it, true);
            }
          }
        }
      }    
    }
    // Merge with watch list from prefs.
    m_ISONList = m_prefsWatchList;
    for (unsigned int index=0; index<m_addresseesISON.count(); index++)
    {
      QString nickname = m_addresseesISON[index];
      if (!m_ISONList.contains(nickname)) m_ISONList.append(nickname);
    }
  }
}

// When user changes preferences and has nick watching turned on, rebuild notify list.
void ServerISON::slotPrefsChanged()
{
  kdDebug() << "ServerISON::slotPrefsChanged" << endl;
  bool useNotify = KonversationApplication::preferences.getUseNotify();
  if (useNotify)
  {
    bool turnedOn = !m_useNotify;
    m_useNotify = true;
    // Get (possibly) modified Nick Watch List from preferences.
    QString groupName = m_server->getServerGroup();
    m_prefsWatchList =
      KonversationApplication::preferences.getNotifyListByGroup(groupName);
    // If user just turned on nick watching, build addressbook watch list
    // and merge with user prefs watch list.
    if (turnedOn)
      recalculateAddressees();
    else
    {
      // Merge (possibly) modified Watch List from preferences with previously
      // calculated addressbook watch list.
      m_ISONList = m_prefsWatchList;
      for (unsigned int index=0; index<m_addresseesISON.count(); index++)
      {
        QString nickname = m_addresseesISON[index];
        if (!m_ISONList.contains(nickname)) m_ISONList.append(nickname);
      }
    }
  }
  // If nick watching is off, clear list.
  else m_ISONList.clear();
  m_useNotify = useNotify;
}

void ServerISON::nickInfoChanged(Server* /*server*/, const NickInfoPtr /*nickInfo*/) {
  // TODO only reculate that one nickinfo somehow.. this is such a waste of time :(
  // not sure how though
  // I would hope we can get rid of this routine altogther..gary.
  recalculateAddressees();
}
 
#include "serverison.moc"

