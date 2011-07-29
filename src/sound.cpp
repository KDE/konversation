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

#include "sound.h"

#include <KUrl>

#include <Phonon/AudioOutput>


namespace Konversation
{
    Sound::Sound(QObject* parent, const char* name)
        : QObject(parent)
    {
        setObjectName(name);
        m_mediaObject = new Phonon::MediaObject(this);
        m_audioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, this);
        Phonon::createPath(m_mediaObject, m_audioOutput);

        connect(m_mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
                this, SLOT(tryPlayNext(Phonon::State,Phonon::State)));

        m_played = false;
    }

    Sound::~Sound()
    {}

    void Sound::play(const KUrl& url)
    {
        if(m_played && ((m_mediaObject->state() != Phonon::PausedState && m_mediaObject->state() != Phonon::StoppedState) || !m_playQueue.isEmpty()))
        {
            if(m_mediaObject->currentSource().url() != url)
            {
                m_playQueue.enqueue(url);
            }

            return;
        }

        m_played = true;
        playSound(url);
    }

    void Sound::tryPlayNext(Phonon::State newState, Phonon::State oldState)
    {
        Q_UNUSED(oldState);

        if(newState == Phonon::PausedState && !m_playQueue.isEmpty())
        {
            playSound(m_playQueue.dequeue());
        }
    }

    void Sound::playSound(const KUrl& url)
    {
        m_mediaObject->setCurrentSource(Phonon::MediaSource(url));
        m_mediaObject->play();
    }
}

#include "sound.moc"
