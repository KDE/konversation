/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
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
QString Ignore::getName() const       { return name; }
int Ignore::getFlags() const          { return flags; }
