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


#include <kurl.h>

#ifdef USE_ARTS
#include <arts/kartsserver.h>
#include <arts/kartsdispatcher.h>
#include <arts/kplayobject.h>
#include <arts/kplayobjectfactory.h>
#endif

namespace Konversation {
  Sound::Sound(QObject *parent, const char *name)
    : QObject(parent, name)
  {
#ifdef USE_ARTS
    soundServer = new KArtsServer;
    dispatcher = new KArtsDispatcher;
#endif
  }
  
  Sound::~Sound()
  {
#ifdef USE_ARTS
    delete soundServer;
    soundServer = 0;
    delete dispatcher;
    dispatcher = 0;
#endif
  }

  void Sound::play(const KURL& url)
  {
#ifdef USE_ARTS
    if(url.isEmpty() || !dispatcher || !soundServer) {
      return;
    }
    
    KPlayObjectFactory factory(soundServer->server());
    KPlayObject* playObj = factory.createPlayObject(url, true);

    if(playObj) {
      playObj->play();
    }
#endif
  }
}

#include "konversationsound.moc"

