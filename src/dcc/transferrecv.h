/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2004, 2005 John Tapsell <john@geola.co.uk>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERRECV_H
#define TRANSFERRECV_H

#include "transfer.h"
// TODO: remove the dependence
#include "resumedialog.h"

#include <QAbstractSocket>

class QTimer;
class QTcpServer;
class QTcpSocket;

namespace KIO
{
    class Job;
    class TransferJob;
}

namespace Konversation
{
    namespace DCC
    {
        class TransferRecvWriteCacheHandler;

        /**
         * Receive a file on DCC protocol
         */
        class TransferRecv : public Transfer
        {
            Q_OBJECT

            public:
                explicit TransferRecv(QObject *parent);
                ~TransferRecv() override;

                // REQUIRED
                void setPartnerIp(const QString &ip);
                // REQUIRED
                void setPartnerPort(quint16 port);
                // REQUIRED
                void setFileSize(quint64 fileSize);
                // OPTIONAL, if not specified, "unnamed_file"
                void setFileName(const QString &fileName);
                // OPTIONAL, if not specified, default folder + the file name
                void setFileURL(const QUrl &url);
                // OPTIONAL
                void setReverse(bool reverse, const QString &reverseToken);

            public Q_SLOTS:
                bool queue() override;

                /** The user has accepted the download.
                 *  Check we are saving it somewhere valid, create any directories needed, and
                 *  connect to remote host.
                 */
                void start() override;
                /** The user has chosen to abort.
                 *  Either by chosen to abort directly, or by choosing cancel when
                 *  prompted for information on where to save etc.
                 *  Not called when it fails due to another problem.
                 */
                void abort() override;
                void startResume(quint64 position);

            protected:
                void cleanUp() override;

            private Q_SLOTS:
                // Local KIO
                void slotLocalCanResume(KIO::Job *job, KIO::filesize_t size);
                void slotLocalGotResult(KJob *job);
                void slotLocalReady(KIO::Job *job);
                void slotLocalWriteDone();
                void slotLocalGotWriteError(const QString &errorString);

                // Remote DCC
                void connectWithSender();
                void startReceiving();
                void connectionFailed(QAbstractSocket::SocketError errorCode);
                void readData();
                void sendAck();
                void connectionTimeout();

                // Reverse DCC
                void slotServerSocketReadyAccept();

                void sendReverseAck(bool error, quint16 port);

            private:
                                                          // (startPosition == 0) means "don't resume"
                void prepareLocalKio(bool overwrite, bool resume, KIO::fileoffset_t startPosition = 0);
                void askAndPrepareLocalKio(const QString &message, int enabledActions, ResumeDialog::ReceiveAction defaultAction, KIO::fileoffset_t startPosition = 0);

                /**
                 * This calls KIO::NetAccess::mkdir on all the subdirectories of dirURL, to
                 * create the given directory.  Note that a url like  file:/foo/bar  will
                 * make sure both foo and bar are created.  It assumes everything in the path is
                 * a directory.
                 * Note: If the directory already exists, returns true.
                 *
                 * @param dirURL A url for the directory to create.
                 * @return True if the directory now exists.  False if there was a problem and the directory doesn't exist.
                 */
                bool createDirs(const QUrl &dirURL) const;

                void requestResume();
                // for non-reverse DCC
                void connectToSendServer();
                // for reverse DCC
                bool startListeningForSender();

                void startConnectionTimer(int secs);
                void stopConnectionTimer();

            private:
                QUrl m_saveToTmpFileURL;
                TransferRecvWriteCacheHandler *m_writeCacheHandler;
                QTimer *m_connectionTimer;

                QTcpServer *m_serverSocket;
                QTcpSocket *m_recvSocket;

                ///We need the original name for resume communication, as sender depends on it
                QString m_saveFileName;

                Q_DISABLE_COPY(TransferRecv)
        };

        class TransferRecvWriteCacheHandler : public QObject
        {
            Q_OBJECT

            public:
                explicit TransferRecvWriteCacheHandler(KIO::TransferJob *transferJob);
                ~TransferRecvWriteCacheHandler() override;

                void append(char *data, int size);
                bool write(bool force = false);
                void close();
                void closeNow();

                Q_SIGNALS:
                void done();                              // ->  DccTransferRecv::writeDone()
                                                          // ->  DccTransferRecv::slotWriteError()
                void gotError(const QString &errorString);

            private Q_SLOTS:
                                                          // <-  m_transferJob->dataReq()
                void slotKIODataReq(KIO::Job *job, QByteArray &data);
                void slotKIOResult(KJob *job);          // <-  m_transferJob->result()

            private:
                KIO::TransferJob *m_transferJob;
                bool m_writeAsyncMode;
                bool m_writeReady;

                QList<QByteArray> m_cacheList;
                QDataStream *m_cacheStream;

                Q_DISABLE_COPY(TransferRecvWriteCacheHandler)
        };
    }
}

#endif  // TRANSFERRECV_H
