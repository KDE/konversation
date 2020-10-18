/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

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
            explicit Sound(QObject *parent = nullptr, const QString &name = QString());
            ~Sound();

        public Q_SLOTS:
            void play(const QUrl &url);

        private Q_SLOTS:
            void tryPlayNext(Phonon::State newState, Phonon::State oldState);

        private:
            void playSound(const QUrl &url);

        private:
            Phonon::MediaObject* m_mediaObject;
            Phonon::AudioOutput* m_audioOutput;

            QQueue<QUrl> m_playQueue;
            bool m_played;

            Q_DISABLE_COPY(Sound)
    };
}
#endif
