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


/*
  @author John Tapsell
*/

class Server;

typedef QMap<KABC::Addressee,QString> AddresseeNickMap;

class ServerISON : public QObject
{
  Q_OBJECT

  public:
    ServerISON(const Server *const server);
    /** Returns a list of nicks that we want to know whether they are online
      * of offline.  This function is called, and the result sent to the
      * server as an /ISON command.
      * 
      * Calls getAddressees() and getWatachedNicks(), and returns the merged result.
      * The resulting nicks don't have the servername/servergroup attached.
      *
      * Read the documentation of the other two functions.
      * 
      * @returns A space delimited list of nicks that we want to know if they are on or not.
      * 
      * @see getAddressees()
      * @see getWatchedNicks()
      */
    QString getISON();
    /** Returns _some_ of the nicks that the addressees have.
     *  It loops through all the addressees that have nickinfos.
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
    /** Looks in the preferences for which nicks the user wants to watch for.
     *  Filters for only the ones for this server.  Strips off the server
     *  name/group.
     */
    QString getWatchedNicks();
  public slots:
    void nickInfoChanged(Server* server, const NickInfoPtr nickInfo);
       
  private:
    /** A pointer to the server we are a member of.
     */
    const Server *const m_server;
    /** A map of the nicks that are on this server,
     *  mapped against the nick they are using if online,
     *  TODO: actually use
     */
    AddresseeNickMap m_addresseeNickMap;

    void recalculateAddressees();
    /** The return result for getAddressees()
     *  @see getAddressees()
     *  @return A list of nicks for this server that we need to do /ISON on.
     */
    QStringList m_addresseesISON;
};


#endif
