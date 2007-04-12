/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Mon Jun 24 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "ignore.h"


Ignore::Ignore(const QString &newName,int newFlags)
{
    setFlags(newFlags);
    setName(newName);
}

Ignore::~Ignore()
{
}

void Ignore::setName(const QString &newName) { name=newName; }
void Ignore::setFlags(int newFlags)   { flags=newFlags; }
QString Ignore::getName()             { return name; }
int Ignore::getFlags()                { return flags; }
