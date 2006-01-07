/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Helper class to parse and disassemble IP+port combinations.
  WARNING: Does not attempt to validate IP addresses.
  begin:     Fri Jan 6 2006
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#ifndef KONVIIPHELPER_H
#define KONVIIPHELPER_H

#include <qobject.h>
#include <qstring.h>

/*
  @author Eike Hein
*/

class KonviIpHelper : QObject
{
    Q_OBJECT

        public:
        KonviIpHelper(const QString target);
        ~KonviIpHelper();

        QString host();
        QString port();

        private:
        void parseTarget(const QString& target);

        QString m_host;
        QString m_port;
};

#endif
