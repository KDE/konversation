/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
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
        Ignore(const Ignore& other) = default;
        ~Ignore();

        Ignore& operator=(const Ignore& other) = default;

        void setName(const QString &newName);
        void setFlags(int newFlags);
        QString getName() const;
        int getFlags() const;

    private:
        QString name;
        int flags;
};
#endif
