// defaultcodec.h
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// See COPYING file for licensing information

#ifndef _KONVERSATION_DEFAULTCODEC_H_
#define _KONVERSATION_DEFAULTCODEC_H_

#include <qtextcodec.h>

#include <klocale.h>

class DefaultCodec
{
  public:
    static QString getCodecForLang( const QString& lang = KLocale::defaultLanguage() );
};

QString DefaultCodec::getCodecForLang( const QString& lang )  // static
{
  // don't add a condition for languages which QTextCodec::codecForLocale() returns a correct codec for.
  
  if ( lang == "ja_JP" )
    return "jis7";
  
  return QTextCodec::codecForLocale()->name(); 
}

#endif  // _KONVERSATION_DEFAULTCODEC_H_
