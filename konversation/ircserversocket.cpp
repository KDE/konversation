/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircserversocket.cpp  -  description
  begin:     Sat Jan 19 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include "ircserversocket.h"

#include <kdebug.h>

IRCServerSocket::IRCServerSocket(const char *server,unsigned short int port,int timeout) :
                 KExtendedSocket(server,port)
{
  kdDebug() << "IRCServerSocket::IRCServerSocket(" << server << "," << port << "): Socket=" << fd() << endl;
}

IRCServerSocket::~IRCServerSocket()
{
  kdDebug() << "IRCServerSocket::~IRCServerSocket()" << endl;
}
