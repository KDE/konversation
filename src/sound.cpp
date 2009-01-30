/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Thu Jan 29 2004
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/

#include "sound.h" ////// header renamed

#include <config-konvi.h>
#include <kurl.h>

#ifdef USE_KNOTIFY
#include <knotification.h>
#endif


namespace Konversation
{
    Sound::Sound(QObject* parent, const char* name)
        : QObject(parent)
        { setObjectName(name); }

    Sound::~Sound()
        {}

    void Sound::play(const KUrl& url)
    {
        #ifdef USE_KNOTIFY
        KNotifyClient::userEvent(0,QString(),1,1,url.path());
        #endif
    }
}

// #include "./sound.moc"
