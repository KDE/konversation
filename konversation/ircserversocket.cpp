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
*/

#include <iostream>

#include "ircserversocket.h"

IRCServerSocket::IRCServerSocket(const char *server,unsigned short int port,int timeout=30) :
                 KSocket(server,port,timeout)
{
  cerr << "IRCServerSocket::IRCServerSocket(" << server << "," << port << "): Socket=" << socket() << endl;
}

IRCServerSocket::~IRCServerSocket()
{
  cerr << "IRCServerSocket::~IRCServerSocket()" << endl;
}
