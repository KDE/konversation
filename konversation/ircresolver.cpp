/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircresolver.cpp  -  A replacement for asyncLookup
  begin:     Fre Feb 28 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kdebug.h>

#include <qapp.h>
#include <qevent.h>

#include "ircresolver.h"

/*
  We use postEvent for messaging here rather than signals/slots since signals
  don't work well with threads
*/

IRCResolver::IRCResolver()
{
  socket=0;
  recipient=0;
}

IRCResolver::~IRCResolver()
{
}

void IRCResolver::setRecipient(QObject* newRecipient)
{
  recipient=newRecipient;
}

void IRCResolver::setSocket(KExtendedSocket* newSocket)
{
  socket=newSocket;
}

void IRCResolver::run()
{
  resolver_lock.lock();
  if(socket && recipient)
    socket->lookup();
  else
    kdDebug() << "IRCResolver::run(): Resolver needs a socket and a recipient!" << endl;

  QApplication::postEvent(recipient,new QEvent(QEvent::User));
  resolver_lock.unlock();
}
