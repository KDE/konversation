// ircdefaultcodec.h
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef _KONVERSATION_IRCDEFAULTCODEC_H_
#define _KONVERSATION_IRCDEFAULTCODEC_H_

class QString;

class IRCDefaultCodec
{
  public:
    static QString getDefaultLocaleCodec();
    static QString getCodecForLang( const QString& lang );
};

#endif  // _KONVERSATION_IRCDEFAULTCODEC_H_
