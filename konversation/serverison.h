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
class Query;
class StatusPanel;
class Identity;
class KonversationMainWindow;
class RawLog;
class ChannelListPanel;
class ScriptLauncher;

class ServerISON : public QObject
{
  Q_OBJECT

  public:
    ServerISON(Server *server);
    
    QString getISON();
    QStringList getAddressees();
    QString getWatchedNicks();

    
  protected:
    Server *m_server;
};

#endif
