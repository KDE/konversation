/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/

#include "common.h"

#include <qstring.h>
#include <qregexp.h>

namespace Konversation
{
  QString removeIrcMarkup(const QString& text)
  {
    QString escaped = text;
    QRegExp colorRegExp("((\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)|\x02|\x09|\x13|\x16|\x1f)");
    // Escape text decoration
    escaped.remove(colorRegExp);
    
    return escaped;
  }
}
