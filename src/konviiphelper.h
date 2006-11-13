/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Helper class to parse and disassemble IP+port combinations.
  WARNING: Does not attempt to validate IP addresses.

  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#ifndef KONVIIPHELPER_H
#define KONVIIPHELPER_H

#include <qobject.h>
#include <qstring.h>


class KonviIpHelper : QObject
{
    Q_OBJECT

    public:
        explicit KonviIpHelper(const QString target);
        ~KonviIpHelper();

        QString host();
        QString port();

    private:
        void parseTarget(const QString& target);

        QString m_host;
        QString m_port;
};

#endif
