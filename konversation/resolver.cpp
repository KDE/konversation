/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  resolver.cpp  -  A replacement for asyncLookup
  begin:     Fre Feb 28 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include "resolver.h"

Resolver::Resolver()
{
  socket=0;
}

Resolver::~Resolver()
{
}

void Resolver::setSocket(KExtendedSocket* newSocket)
{
  socket=newSocket;
}

void Resolver::run()
{
  if(socket)
  {
    int num=socket->lookup();
    emit lookupFinished(num);
  }
  else emit lookupFinished(-1);
}

#include "resolver.moc"
