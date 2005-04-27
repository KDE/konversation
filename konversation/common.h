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

#ifndef COMMON_H
#define COMMON_H

class QCString;
class QString;
class QBitmap;
class QPixmap;

#include <qregexp.h>
#include "guess_ja.h"

namespace Konversation
{
  QString removeIrcMarkup(const QString& text);
  QString tagURLs(const QString& text, const QString& fromNick, bool useCustomColor = true);
  QBitmap overlayMasks( const QBitmap *under, const QBitmap *over );
  QPixmap overlayPixmaps(const QPixmap &under, const QPixmap &over);
  bool isUtf8(const QCString& text);
  JapaneseCode::Type guess_ja(const char* text, int length);

  static QRegExp colorRegExp("((\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)|\x02|\x09|\x13|\x16|\x1f)");
  static QRegExp urlPattern("((www\\.(?!\\.)|(fish|(f|ht)tp(|s))://)([\\d\\w\\./,\\':~\\?=;#@\\-\\+\\%\\*\\{\\}\\!]|&amp;)+)|"
			    "([-.\\d\\w]+@[-.\\d\\w]{2,}\\.[\\w]{2,})");
}

#endif
