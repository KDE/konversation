/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircresolver.h  -  A replacement for asyncLookup
  begin:     Fre Feb 28 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef IRCRESOLVER_H
#define IRCRESOLVER_H

#include <qthread.h>

#include <kextendedsocket.h>

/*
  @author Dario Abatianni
*/

class IRCResolver : public QThread
{
  public:
    IRCResolver();
    ~IRCResolver();

    void setRecipient(QObject* recipient);
    void setSocket(KExtendedSocket* newSocket);
    void run();

  protected:
    QObject* recipient;
    KExtendedSocket* socket;
};

#endif
