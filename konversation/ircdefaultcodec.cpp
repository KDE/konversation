// ircdefaultcodec.cpp
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qtextcodec.h>
#include <klocale.h>

#include "ircdefaultcodec.h"

QString IRCDefaultCodec::getDefaultLocaleCodec()
{
  return getCodecForLang( KLocale::defaultLanguage() );
}

QString IRCDefaultCodec::getCodecForLang( const QString& lang )  // static
{
  // don't add conditions for languages which QTextCodec::codecForLocale() returns a correct codec for.
  
  if ( lang == "ja_JP" )
    return "jis7";
  
  return QTextCodec::codecForLocale()->name(); 
}
