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

#ifndef SERVERISON_H
#define SERVERISON_H


/**
* @author John Tapsell
* @author Gary Cramblitt <garycramblitt@comcast.net>
*/

class Server;

typedef QMap<QString,KABC::Addressee> OfflineNickToAddresseeMap;

class ServerISON : public QObject
{
  Q_OBJECT

  public:
    ServerISON(Server* server);
    /**
    * Returns a list of nicks that we want to know whether they are online
    * of offline.  This function is called, and the result sent to the
    * server as an /ISON command.
    * 
    * Calls getAddressees() and merges with the Watch List from preferences.
    * The resulting nicks don't have the servername/servergroup attached.
    *
    * @returns              A list of nicks that we want to know if they are on or not.
    * 
    * @see getAddressees()
    */
    QStringList getISONList();
    
    /**
    * Returns _some_ of the nicks that the addressees have.
    * It loops through all the addressees that have nickinfos.
    *
    *  - If that addressee has some nicks, and at least one of them is in a
    *    channel we are in, then we know they are online, so don't add.
    *  - Otherwise, if that addressee has some nicks, and we think they are
    *    online, add the nick that they are currently online with.  This does
    *    mean that if they change their nick, they will appear offline for the
    *    duration between ISON's.
    *  - Otherwise, add all the nicks we know the addressee has.
    */
    QStringList getAddressees();
    
    /**
    * Given the nickname of nick that is offline (or at least not known to be online),
    * returns the addressbook entry (if any) for the nick.
    * @param nickname       Desired nickname.  Case sensitive.
    * @return               Addressbook entry of the nick or empty if not found.
    */
    KABC::Addressee getOfflineNickAddresse(QString& nickname);

  
  public slots:
    void nickInfoChanged(Server* server, const NickInfoPtr nickInfo);
    void slotPrefsChanged();
       
  private:
    /**
    * Rebuilds list of nicks to watch whenever an addressbook change occurs
    * or whenever user turns on nick watching.
    */
    void recalculateAddressees();
    
    /// Map of all offline nicks in the addressbook associated with this server
    /// or server group and their addressbook entry, indexed by nickname.
    /// TODO: Lowercase nickname?
    OfflineNickToAddresseeMap m_offlineNickToAddresseeMap;
    
    /// A pointer to the server we are a member of.
    Server* m_server;
    /// List of nicks to watch that come from addressbook.
    QStringList m_addresseesISON;
    /// List of nicks in the Nick Watch List (from preferences).
    QStringList m_prefsWatchList;
    /// Merged list of the two above.
    QStringList m_ISONList;
    /// State of UseNotify preference.
    bool m_useNotify;
};


#endif
