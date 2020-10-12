/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "sound.h"

#include <QUrl>

#include <Phonon/AudioOutput>


namespace Konversation
{
    Sound::Sound(QObject* parent, const QString& name)
        : QObject(parent)
    {
        setObjectName(name);
        m_mediaObject = new Phonon::MediaObject(this);
        m_audioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, this);
        Phonon::createPath(m_mediaObject, m_audioOutput);

        connect(m_mediaObject, &Phonon::MediaObject::stateChanged, this, &Sound::tryPlayNext);

        m_played = false;
    }

    Sound::~Sound()
    {}

    void Sound::play(const QUrl &url)
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

    void Sound::playSound(const QUrl &url)
    {
        m_mediaObject->setCurrentSource(Phonon::MediaSource(url));
        m_mediaObject->play();
    }
}


