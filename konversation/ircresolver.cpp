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

#include "ircresolver.h"

IRCResolver::IRCResolver()
{
  socket=0;
}

IRCResolver::~IRCResolver()
{
}

void IRCResolver::setSocket(KExtendedSocket* newSocket)
{
  socket=newSocket;
}

void IRCResolver::run()
{
  if(socket)
  {
    int num=socket->lookup();
    emit lookupFinished(num);
  }
  else emit lookupFinished(-1);
}

#include "ircresolver.moc"
