/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  localestring.h  -  Class that sorts in localeAware order.
  begin:     Wed Aug 04  2004
  copyright: (C) 2002,2003,2004 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef LOCALE_STRING_H
#define LOCALE_STRING_H

/** A LocaleString is used as a key to a QMap.  Unlike QString, it sorts the QMap
  * in localeAware order.
  */
class LocaleString : public QString
{
  public:
    LocaleString() : QString() {};
    LocaleString(const QString& s) : QString(s) {}
    LocaleString(const LocaleString& s) : QString(s) {}
    LocaleString& operator=(const QString& s) {
      QString::operator=(s);
      return *this; }
    LocaleString& operator=(const LocaleString& s) {
      QString::operator=(s);
      return *this; }
    inline bool operator<( const LocaleString &s1) { return (localeAwareCompare(s1) < 0); }
    inline bool operator<( const QString &s1) { return (localeAwareCompare(s1) < 0); }
    inline bool operator<( const char *s1) { return (localeAwareCompare(s1) < 0); }
    inline bool operator<( QChar c) { return (localeAwareCompare(c) < 0); }
    inline bool operator<( char ch) { return (localeAwareCompare(&ch) < 0); }
};

#endif
