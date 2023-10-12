/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#ifndef KONVERSATIONKONVERSATIONSOUND_H
#define KONVERSATIONKONVERSATIONSOUND_H

#include <QMediaPlayer>
#include <QObject>
#include <QQueue>
#include <QUrl>

class QAudioOutput;

namespace Konversation
{

    /**
    Class that handles sounds
    */
    class Sound : public QObject
    {
        Q_OBJECT

        public:
            explicit Sound(QObject *parent = nullptr, const QString &name = QString());
            ~Sound() override;

        public Q_SLOTS:
            void play(const QUrl &url);

        private Q_SLOTS:
            void tryPlayNext(QMediaPlayer::PlaybackState newState);

        private:
            void playSound(const QUrl &url);

        private:
            QMediaPlayer *const m_mediaObject;
            QAudioOutput *const m_audioOutput;

            QQueue<QUrl> m_playQueue;
            QUrl m_currentUrl;
            bool m_played;

            Q_DISABLE_COPY(Sound)
    };
}
#endif
