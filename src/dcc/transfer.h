/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2004, 2005 John Tapsell <john@geola.co.uk>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFER_H
#define TRANSFER_H

#include <KIO/Global>

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QUrl>

namespace Konversation
{
    namespace DCC
    {
        using transferspeed_t = qreal;

        class Transfer : public QObject
        {
            Q_OBJECT

            public:
                enum Type
                {
                    Receive = 1,
                    Send    = 1 << 1
                };

                enum Status
                {
                    Configuring = 0,                      // Not queud yet (this means that user can't see the item at this time)
                    Queued,                               // Newly added DCC, waiting user's response
                    Preparing,                            // Opening KIO to write received data
                    WaitingRemote,                        // Waiting for remote host's response
                    Connecting,                           // RECV: trying to connect to the server
                    Transferring,
                    Done,
                    Failed,
                    Aborted
                };

                enum UnavailableStatus
                {
                    Calculating = -1,
                    NotInTransfer = -2,
                    InfiniteValue = -3
                };

                Transfer(Type dccType, QObject *parent);
                ~Transfer() override;

                Type               getType()                  const;
                Status             getStatus()                const;
                const QString &    getStatusDetail()          const;
                QDateTime          getTimeOffer()             const;
                int                getConnectionId()          const;
                QString            getOwnIp()                 const;
                quint16            getOwnPort()               const;
                QString            getPartnerNick()           const;
                QString            getPartnerIp()             const;
                quint16            getPartnerPort()           const;
                QString            getFileName()              const;
                KIO::filesize_t    getFileSize()              const;
                KIO::fileoffset_t  getTransferringPosition()  const;
                KIO::fileoffset_t  getTransferStartPosition() const;
                QUrl               getFileURL()               const;
                bool               isResumed()                const;
                bool               isReverse()                const;
                QString            getReverseToken()          const;
                transferspeed_t    getAverageSpeed()          const;
                transferspeed_t    getCurrentSpeed()          const;
                int                getTimeLeft()              const;
                int                getProgress()              const;
                QDateTime          getTimeTransferStarted()   const;
                QDateTime          getTimeTransferFinished()  const;

                // common settings for DccTransferRecv / DccTransferSend

                // REQUIRED
                void setConnectionId(int connectionId);
                // REQUIRED
                void setPartnerNick(const QString &nick);

                void removedFromView();

            Q_SIGNALS:
                void transferStarted(Konversation::DCC::Transfer *item);
                //done is when the transfer is done, it will not get deleted after emiting this signal
                void done(Konversation::DCC::Transfer *item);
                void statusChanged(Konversation::DCC::Transfer *item, int newStatus, int oldStatus);
                //removed is when the transfer is removed from all visible views and ready to get deleted
                void removed(Konversation::DCC::Transfer *item);

            public Q_SLOTS:
                virtual bool queue();
                virtual void start() {}
                virtual void abort() {}
                void runFile();

            protected:
                virtual void cleanUp();
                void failed(const QString &errorMessage = QString());

                /**
                 * setStatus behavior changed:
                 * Now make sure to run functions that change transfer information before setStatus.
                 * For example cleanUp();
                 *
                 * If you call setStatus(..) and change the "Started at:"-time afterwards,
                 * the transferpanel won't notice it
                 */
                void setStatus(Status status, const QString &statusDetail = QString());
                void startTransferLogger();
                void finishTransferLogger();

                static QString transferFileName(const QString &fileName);
                static QString sanitizeFileName(const QString &fileName);

            protected Q_SLOTS:
                void logTransfer();

            protected:
                // transfer information
                Type m_type;
                Status m_status;
                QString m_statusDetail;
                bool m_resumed;
                bool m_reverse;
                QString m_reverseToken;
                KIO::fileoffset_t m_transferringPosition;
                KIO::fileoffset_t m_transferStartPosition;

                // we'll communicate with the partner via this server
                int m_connectionId;
                QString m_partnerNick;
                QString m_partnerIp;                      // null when unknown
                quint16 m_partnerPort;
                QString m_ownIp;
                quint16 m_ownPort;

                unsigned long m_bufferSize;
                char *m_buffer;

                /**
                 * The filename. Clean filename without any "../" or extra "
                 */
                QString m_fileName;

                /** The file size of the complete file sending/recieving. */
                KIO::filesize_t m_fileSize;

                /**
                 * If we are sending a file, this is the url of the file we are sending.
                 * If we are recieving a file, this is the url of the file we are saving
                 * to in the end (Temporararily it will be filename+".part" ).
                 */
                QUrl m_fileURL;

            private:
                void updateTransferMeters();

            private:
                QDateTime m_timeOffer;
                QDateTime m_timeTransferStarted;
                //QDateTime m_timeLastActive;
                QDateTime m_timeTransferFinished;

                QTimer m_loggerTimer;
                QElapsedTimer m_loggerBaseTime;  // for calculating CPS
                QList<int> m_transferLogTime;
                QList<KIO::fileoffset_t> m_transferLogPosition;

                transferspeed_t m_averageSpeed;
                transferspeed_t m_currentSpeed;
                int m_timeLeft;

                Q_DISABLE_COPY(Transfer)
        };
    }
}

#endif  // TRANSFER_H
