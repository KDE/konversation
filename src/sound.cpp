/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "sound.h"

namespace Konversation
{
    Sound::Sound(QObject* parent, const QString& name)
        : QObject(parent)
        , m_mediaObject(new QMediaPlayer(this, QMediaPlayer::LowLatency))
        , m_played(false)
    {
        setObjectName(name);

        m_mediaObject->setAudioRole(QAudio::NotificationRole);
        connect(m_mediaObject, &QMediaPlayer::stateChanged, this, &Sound::tryPlayNext);
    }

    Sound::~Sound()
    {}

    void Sound::play(const QUrl &url)
    {
        if (m_played && (m_mediaObject->state() == QMediaPlayer::PlayingState || !m_playQueue.isEmpty())) {
            if (m_currentUrl != url) {
                m_playQueue.enqueue(url);
            }

            return;
        }

        m_played = true;
        playSound(url);
    }

    void Sound::tryPlayNext(QMediaPlayer::State newState)
    {
        if (newState == QMediaPlayer::StoppedState && !m_playQueue.isEmpty()) {
            playSound(m_playQueue.dequeue());
        }
    }

    void Sound::playSound(const QUrl &url)
    {
        m_currentUrl = url;
        m_mediaObject->setMedia(url);
        m_mediaObject->play();
    }
}


