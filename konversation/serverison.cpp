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

#include "server.h"
#include "serverison.h"
#include "addressbook.h"
#include "konversationapplication.h"
#include <qstring.h>
#include <qstringlist.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

ServerISON::ServerISON(const Server *const server) : m_server(server) {
  connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( recalculateAddressees() ) );
  connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(recalculateAddressees()));
  connect( m_server, SIGNAL(nickInfoChanged(Server*, const NickInfoPtr)), this, SLOT(nickInfoChanged(Server*, const NickInfoPtr)));
			  
}
    
QString ServerISON::getISON() {
  //TODO unique this list?  Be better, but probably not worth it.
  return getAddressees().join(" ") + " " + getWatchedNicks();
  
}

QStringList ServerISON::getAddressees(){
//TODO - remove the below line when we don't need it anymore.  It _should_ work, but leave it in while testing, to iron out bugs first
  recalculateAddressees();
  return m_addresseesISON;
}
void ServerISON::recalculateAddressees() {
  m_addresseesISON.clear();
  for( KABC::AddressBook::ConstIterator it = Konversation::Addressbook::self()->getAddressBook()->begin(); 
		  it != Konversation::Addressbook::self()->getAddressBook()->end(); ++it ) {
    if(Konversation::Addressbook::self()->hasAnyNicks(*it)) {
      QString uid = (*it).uid();
      bool found = false;
      //First check if we already know that this nick is online.
      //TODO we really should have a map :( 
      for(NickInfoMap::ConstIterator nickInfoIt=m_server->getNicksOnline()->begin(); 
		                     nickInfoIt != m_server->getNicksOnline()->end(); ++nickInfoIt)
      {
        NickInfoPtr nickInfo = nickInfoIt.data();
        if(nickInfo->getAddressee().uid() == uid) {
          m_addresseesISON.append(nickInfo->getNickname());  //They are online.  Use the nickname they are online as, to make a short string.
  	  found = true;
        }
      }
      if(!found) {
        QString lserverName = m_server->getServerName().lower();
        QString lserverGroup = m_server->getServerGroup().lower();
	//Okay we can't find the nick online, 
        QStringList nicks = QStringList::split( QChar( 0xE000 ), (*it).custom("messaging/irc", "All") );
        for( QStringList::Iterator nicksIt = nicks.begin(); nicksIt != nicks.end(); ++nicksIt ) {
          QString lserverOrGroup = (*nicksIt).section(QChar(0xE120),1).lower();
          if(lserverOrGroup == lserverName || lserverOrGroup == lserverGroup || lserverOrGroup.isEmpty()) {
	    QString nickname = (*nicksIt).section(QChar(0xE120),0,0).lower();
            m_addresseesISON.append(nickname);  //Add all the nicks that are on this server.
          }
        }
      }
    }    
  }
}

QString ServerISON::getWatchedNicks() {
  return KonversationApplication::preferences.getNotifyStringByGroup(m_server->getServerGroup());
}

void ServerISON::nickInfoChanged(Server* /*server*/, const NickInfoPtr /*nickInfo*/) {
  //TODO only reculate that one nickinfo somehow.. this is such a waste of time :(
  //not sure how though
  recalculateAddressees();
}
 

#include "serverison.moc"

