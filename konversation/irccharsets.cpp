// irccharsets.cpp
// A wrapper for KCharsets
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qglobal.h>

#if QT_VERSION < 0x030300
#include <klocale.h>
#else
#include <qlocale.h>
#endif

#include <qtextcodec.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kglobal.h>

#include "irccharsets.h"

QStringList IRCCharsets::availableEncodingShortNames()  // static
{
  private_init();
  return s_shortNames;
}

QStringList IRCCharsets::availableEncodingDescriptiveNames()  // static
{
  private_init();
  return s_descriptiveNames;
}

int IRCCharsets::availableEncodingsCount()  // static
{
  private_init();
  return s_shortNames.count();
}

QString IRCCharsets::shortNameToDescriptiveName( const QString& shortName ) // static
{
  private_init();
  return availableEncodingDescriptiveNames()[shortNameToIndex(shortName)];
}

QString descriptiveNameToShortName( const QString& descriptiveName )  // static
{
  return KGlobal::charsets()->encodingForName( descriptiveName );
}

int IRCCharsets::shortNameToIndex( const QString& shortName )  // static
{
  private_init();
  int index = 0;
  for ( QStringList::iterator it = s_shortNames.begin() ; it != s_shortNames.end() ; ++it )
  {
    if ( (*it) == shortName )
      return index;
    ++index;
  }
  return -1;
}

bool IRCCharsets::isValidEncoding( const QString& shortName )  // static
{
  private_init();
  return ( s_shortNames.contains( shortName ) > 0 );
}

QString IRCCharsets::encodingForLocale()  // static
{
  private_init();
#if QT_VERSION < 0x030300
  QString locale = KLocale::defaultLanguage();
#else
  QString locale = QLocale::system().name();
#endif  
  
  // Special cases
  // don't add conditions for the languages which QTextCodec::codecForLocale() returns a correct codec for.
  if ( locale == "ja_JP" )
    return "jis7";
  
  // it's a little hacky..
  for ( QStringList::iterator it = s_shortNames.begin() ; it != s_shortNames.end() ; ++it )
    if ( QTextCodec::codecForName( (*it).ascii() ) == QTextCodec::codecForLocale() )
      return *it;
  
  return "utf8";
}

QString IRCCharsets::localeAlias(const QString& locale)
{
  private_init();
  return s_localeAliases[locale.lower()];
}


void IRCCharsets::private_init()  // static, private
{
  if(s_localeAliases.isEmpty())
    {
      // Setup locale aliases
      // Only need to alias the ones containing space
      s_localeAliases["cp1250"] = "cp 1250";
      s_localeAliases["cp1251"] = "cp 1251";
      s_localeAliases["cp1252"] = "cp 1252";
      s_localeAliases["cp1253"] = "cp 1253";
      s_localeAliases["cp1254"] = "cp 1254";
      s_localeAliases["cp1255"] = "cp 1255";
      s_localeAliases["cp1256"] = "cp 1256";
      s_localeAliases["cp1257"] = "cp 1257";

      s_localeAliases["iso8859-1"] = "iso 8859-1";
      s_localeAliases["iso8859-2"] = "iso 8859-2";
      s_localeAliases["iso8859-3"] = "iso 8859-3";
      s_localeAliases["iso8859-4"] = "iso 8859-4";
      s_localeAliases["iso8859-5"] = "iso 8859-5";
      s_localeAliases["iso8859-6"] = "iso 8859-6";
      s_localeAliases["iso8859-7"] = "iso 8859-7";
      s_localeAliases["iso8859-8"] = "iso 8859-8";
      s_localeAliases["iso8859-8-i"] = "iso 8859-8-i";
      s_localeAliases["iso8859-9"] = "iso 8859-9";
      s_localeAliases["iso8859-11"] = "iso 8859-11";
      s_localeAliases["iso8859-13"] = "iso 8859-13";
      s_localeAliases["iso8859-15"] = "iso 8859-15";

      s_localeAliases["pt154"] = "pt 154";
    }

  if ( !s_shortNames.isEmpty() )
    return;

  s_descriptiveNames = KGlobal::charsets()->descriptiveEncodingNames();
  
  QStringList::Iterator it = s_descriptiveNames.begin();
  while ( it != s_descriptiveNames.end() )
  {
    QString encodingName = KGlobal::charsets()->encodingForName( *it );
    // exclude utf16 and ucs2
    if ( encodingName == "utf16" || encodingName == "iso-10646-ucs-2" )
      it = s_descriptiveNames.remove( it );
    else
    {
      s_shortNames.append( encodingName );
      ++it;
    }
  }
}

QStringList IRCCharsets::s_shortNames;
QStringList IRCCharsets::s_descriptiveNames;
QMap<QString,QString> IRCCharsets::s_localeAliases;
