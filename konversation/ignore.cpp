/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ignore.cpp  -  description
  begin:     Mon Jun 24 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kdebug.h>

#include "ignore.h"

Ignore::Ignore(QString newName,int newFlags)
{
  setFlags(newFlags);
  setName(newName);
}

Ignore::~Ignore()
{
  kdDebug() << "Ignore::~Ignore(" << name << ")" << endl;
}
