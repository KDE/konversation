/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircserversocket.h  -  description
  begin:     Sat Jan 19 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

// #include "newkextsock.h"
#include <kextsock.h>

#ifndef IRCSERVERSOCKET_H
#define IRCSERVERSOCKET_H

/*
  @author Dario Abatianni
*/

class IRCServerSocket : public KExtendedSocket
{
  Q_OBJECT

  public:
    IRCServerSocket(const char *server,unsigned short int port,int timeout=30);
    ~IRCServerSocket();
};

#endif
