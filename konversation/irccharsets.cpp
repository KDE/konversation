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

void IRCCharsets::private_init()  // static, private
{
  if ( !s_shortNames.isEmpty() )
    return;
  
  s_descriptiveNames = KGlobal::charsets()->descriptiveEncodingNames();
  
  QStringList::Iterator it = s_descriptiveNames.begin();
  while ( it != s_descriptiveNames.end() )
  {
    QString encodingName = KGlobal::charsets()->encodingForName( *it );
    // exclude utf16 and iso-10646*
    if ( encodingName == "utf16" || encodingName.startsWith( "iso-10646" ) )
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
