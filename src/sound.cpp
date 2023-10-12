/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "sound.h"

#include <QAudioOutput>

namespace Konversation
{
    Sound::Sound(QObject* parent, const QString& name)
        : QObject(parent)
        , m_mediaObject(new QMediaPlayer(this))
        , m_audioOutput(new QAudioOutput(this))
        , m_played(false)
    {
        setObjectName(name);

        m_mediaObject->setAudioOutput(m_audioOutput);
        connect(m_mediaObject, &QMediaPlayer::playbackStateChanged, this, &Sound::tryPlayNext);
    }

    Sound::~Sound()
    {}

    void Sound::play(const QUrl &url)
    {
        if (m_played && (m_mediaObject->playbackState() == QMediaPlayer::PlayingState || !m_playQueue.isEmpty())) {
            if (m_currentUrl != url) {
                m_playQueue.enqueue(url);
            }

            return;
        }

        m_played = true;
        playSound(url);
    }

    void Sound::tryPlayNext(QMediaPlayer::PlaybackState newState)
    {
        if (newState == QMediaPlayer::StoppedState && !m_playQueue.isEmpty()) {
            playSound(m_playQueue.dequeue());
        }
    }

    void Sound::playSound(const QUrl &url)
    {
        m_currentUrl = url;
        m_mediaObject->setSource(url);
        m_mediaObject->play();
    }
}

#include "moc_sound.cpp"
