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

#include "konversationsound.h"
#include <config.h>

#ifdef USE_ARTS

#include <kurl.h>

#include <arts/kartsserver.h>
#include <arts/kartsdispatcher.h>
#include <arts/kplayobject.h>
#include <arts/kplayobjectfactory.h>

namespace Konversation {
  Sound::Sound(QObject *parent, const char *name)
    : QObject(parent, name)
  {
    soundServer = new KArtsServer;
    dispatcher = new KArtsDispatcher;
  }
  
  Sound::~Sound()
  {
    delete soundServer;
    soundServer = 0;
    delete dispatcher;
    dispatcher = 0;
  }

  void Sound::play(const KURL& url)
  {
    if(url.isEmpty() || !dispatcher || !soundServer) {
      return;
    }
    
    KPlayObjectFactory factory(soundServer->server());
    KPlayObject* playObj = factory.createPlayObject(url, true);

    if(playObj) {
      playObj->play();
    }
  }
}
#endif

#include "konversationsound.moc"

