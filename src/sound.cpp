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

#include <kurl.h>

#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>


namespace Konversation
{
    Sound::Sound(QObject* parent, const char* name)
        : QObject(parent)
        { setObjectName(name); }

    Sound::~Sound()
        {}

    void Sound::play(const KUrl& url)
    {
        // consider porting highlight settings to knotify (uwolfer)
        Phonon::MediaObject *mediaObject = new Phonon::MediaObject(this);
        mediaObject->setCurrentSource(Phonon::MediaSource(url));
        Phonon::AudioOutput *audioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, this);
        Phonon::Path path = Phonon::createPath(mediaObject, audioOutput);
        mediaObject->play();
    }
}

#include "./sound.moc"
