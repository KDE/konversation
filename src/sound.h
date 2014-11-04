// Copyright 2009  Peter Simonsson <peter.simonsson@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License or (at your option) version 3 or any later version
// accepted by the membership of KDE e.V. (or its successor approved
// by the membership of KDE e.V.), which shall act as a proxy
// defined in Section 14 of version 3 of the license.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef KONVERSATIONKONVERSATIONSOUND_H
#define KONVERSATIONKONVERSATIONSOUND_H

#include <QObject>
#include <QQueue>

#include <Phonon/MediaObject>

class QUrl;

namespace Phonon
{
    class AudioOutput;
}

namespace Konversation
{

    /**
    Class that handles sounds
    */
    class Sound : public QObject
    {
        Q_OBJECT

        public:
            explicit Sound(QObject *parent = 0, const QString &name = QString());
            ~Sound();

        public Q_SLOTS:
            void play(const QUrl &url);

        protected Q_SLOTS:
            void tryPlayNext(Phonon::State newState, Phonon::State oldState);

        protected:
            void playSound(const QUrl &url);

        private:
            Phonon::MediaObject* m_mediaObject;
            Phonon::AudioOutput* m_audioOutput;

            QQueue<QUrl> m_playQueue;
            bool m_played;
    };
}
#endif
