/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "sound.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#endif

namespace Konversation
{
    Sound::Sound(QObject* parent, const QString& name)
        : QObject(parent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        , m_mediaObject(new QMediaPlayer(this, QMediaPlayer::LowLatency))
#else
        , m_mediaObject(new QMediaPlayer(this))
        , m_audioOutput(new QAudioOutput(this))
#endif
        , m_played(false)
    {
        setObjectName(name);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_mediaObject->setAudioRole(QAudio::NotificationRole);
        connect(m_mediaObject, &QMediaPlayer::stateChanged, this, &Sound::tryPlayNext);
#else
        m_mediaObject->setAudioOutput(m_audioOutput);
        connect(m_mediaObject, &QMediaPlayer::playbackStateChanged, this, &Sound::tryPlayNext);
#endif
    }

    Sound::~Sound()
    {}

    void Sound::play(const QUrl &url)
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (m_played && (m_mediaObject->state() == QMediaPlayer::PlayingState || !m_playQueue.isEmpty())) {
#else
        if (m_played && (m_mediaObject->playbackState() == QMediaPlayer::PlayingState || !m_playQueue.isEmpty())) {
#endif
            if (m_currentUrl != url) {
                m_playQueue.enqueue(url);
            }

            return;
        }

        m_played = true;
        playSound(url);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void Sound::tryPlayNext(QMediaPlayer::State newState)
#else
    void Sound::tryPlayNext(QMediaPlayer::PlaybackState newState)
#endif
    {
        if (newState == QMediaPlayer::StoppedState && !m_playQueue.isEmpty()) {
            playSound(m_playQueue.dequeue());
        }
    }

    void Sound::playSound(const QUrl &url)
    {
        m_currentUrl = url;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_mediaObject->setMedia(url);
#else
        m_mediaObject->setSource(url);
#endif
        m_mediaObject->play();
    }
}

#include "moc_sound.cpp"
