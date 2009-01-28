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

#ifndef KONVERSATIONKONVERSATIONSOUND_H
#define KONVERSATIONKONVERSATIONSOUND_H

#include <qobject.h>


class KUrl;

namespace Konversation
{

    /**
    Class that handles sounds
    */
    class Sound : public QObject
    {
        Q_OBJECT

        public:
            explicit Sound(QObject *parent = 0, const char *name = 0);
            ~Sound();

        public slots:
            void play(const KUrl& url);

    };
}
#endif
