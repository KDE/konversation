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

#include <qstring.h>
#include <qstringlist.h>
#include <kdebug.h>

#include "konviiphelper.h"


/*
  @author Eike Hein
*/


KonviIpHelper::KonviIpHelper(const QString target)
{
    parseTarget(target);
}

KonviIpHelper::~KonviIpHelper()
{
}

void KonviIpHelper::parseTarget(const QString& target)
{

    // Full-length IPv6 address with port
    // Example: RFC 2732 notation:     [2001:0DB8:0000:0000:0000:0000:1428:57ab]:6666
    // Example: Non-RFC 2732 notation: 2001:0DB8:0000:0000:0000:0000:1428:57ab:6666
    if (target.contains(':')==8)
    {
        m_host = target.section(':',0,-2).remove("[").remove("]");
        m_port = target.section(':',-1);
    }
    // Full-length IPv6 address without port or not-full-length IPv6 address with port
    // Example: Without port, RFC 2732 notation:     [2001:0DB8:0000:0000:0000:0000:1428:57ab]
    // Example: Without port, Non-RFC 2732 notation: 2001:0DB8:0000:0000:0000:0000:1428:57ab
    // Example: With port, RFC 2732 notation:        [2001:0DB8::1428:57ab]:6666
    else if (target.contains(':')>=4)
    {
        // Last segment does not end with ], but the next to last does;
        // Assume not-full-length IPv6 adddress with port
        // Example: [2001:0DB8::1428:57ab]:6666
        if (target.section(':',0,-2).endsWith("]") && !target.section(':',-1).endsWith("]"))
        {
            m_host = target.section(':',0,-2).remove("[").remove("]");
            m_port = target.section(':',-1);
        }
        else
        {
            QString targetCopy = target;
            m_host = targetCopy.remove("[").remove("]");
        }
    }
    // IPv4 address or ordinary hostname with port
    // Example: IPv4 address with port: 123.123.123.123:6666
    // Example: Hostname with port:     irc.bla.org:6666
    else if (target.contains(':')==1)
    {
        m_host = target.section(':',0,-2);
        m_port = target.section(':',-1);
    }
    else
    {
        m_host = target;
    }
}

QString KonviIpHelper::host()
{
    return m_host;
}
QString KonviIpHelper::port()
{
    return m_port;
}

#include "konviiphelper.moc"
