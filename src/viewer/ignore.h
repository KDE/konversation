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

#ifndef IGNORE_H
#define IGNORE_H

#include <QString>


class Ignore
{
    public:
        enum Type
        {
            Channel = 1 << 0,
            Query   = 1 << 1,
            Notice  = 1 << 2,
            CTCP    = 1 << 3,
            DCC     = 1 << 4,
            Invite  = 1 << 5,
            All     = Channel|Query|Notice|CTCP|DCC|Invite,

            Exception = 1 << 31
        };

        Ignore(const QString &name,int flags);
        ~Ignore();

        void setName(const QString &newName);
        void setFlags(int newFlags);
        QString getName() const;
        int getFlags() const;

    protected:
        QString name;
        int flags;
};
#endif
