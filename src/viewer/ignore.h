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
            Channel=1,
            Query=2,
            Notice=4,
            CTCP=8,
            DCC=16,
            Exception=32,
            All=31
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
