/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  resolver.h  -  A replacement for asyncLookup
  begin:     Fre Feb 28 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef RESOLVER_H
#define RESOLVER_H

#include <qthread.h>
#include <qobject.h>

#include <kextendedsocket.h>

/*
  @author Dario Abatianni
*/

class Resolver : public QObject, public QThread
{
  Q_OBJECT

  public: 
    Resolver();
    ~Resolver();

    void setSocket(KExtendedSocket* newSocket);
    void run();

  signals:
    void lookupFinished(int num);

  protected:
    KExtendedSocket* socket;
};

#endif
