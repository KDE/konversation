/*
  receive a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
/*
  Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/
/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
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

        class TransferRecv : public Transfer
        {
            Q_OBJECT

            public:
                TransferRecv(QObject *parent);
                virtual ~TransferRecv();

                // REQUIRED
                void setPartnerIp(const QString &ip);
                // REQUIRED
                void setPartnerPort(quint16 port);
                // REQUIRED
                void setFileSize(quint64 fileSize);
                // OPTIONAL, if not specified, "unnamed_file"
                void setFileName(const QString &fileName);
                // OPTIONAL, if not specified, default folder + the file name
                void setFileURL(const KUrl &url);
                // OPTIONAL
                void setReverse(bool reverse, const QString &reverseToken);

            public slots:
                virtual bool queue();

                /** The user has accepted the download.
                 *  Check we are saving it somewhere valid, create any directories needed, and
                 *  connect to remote host.
                 */
                virtual void start();
                /** The user has chosen to abort.
                 *  Either by chosen to abort directly, or by choosing cancel when
                 *  prompted for information on where to save etc.
                 *  Not called when it fails due to another problem.
                 */
                virtual void abort();
                void startResume(quint64 position);

            protected slots:
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

            protected:
                void cleanUp();

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
                bool createDirs(const KUrl &dirURL) const;

                void requestResume();
                // for non-reverse DCC
                void connectToSendServer();
                // for reverse DCC
                bool startListeningForSender();

                void startConnectionTimer(int secs);
                void stopConnectionTimer();

            protected:
                KUrl m_saveToTmpFileURL;
                ///Current filesize of the file saved on the disk.
                KIO::filesize_t m_saveToFileSize;
                ///Current filesize of the file+".part" saved on the disk.
                KIO::filesize_t m_partialFileSize;
                TransferRecvWriteCacheHandler *m_writeCacheHandler;
                bool m_saveToFileExists;
                bool m_partialFileExists;
                QTimer *m_connectionTimer;

                QTcpServer *m_serverSocket;
                QTcpSocket *m_recvSocket;

                ///We need the original name for resume communication, as sender depends on it
                QString m_saveFileName;
        };

        class TransferRecvWriteCacheHandler : public QObject
        {
            Q_OBJECT

            public:
                explicit TransferRecvWriteCacheHandler(KIO::TransferJob *transferJob);
                virtual ~TransferRecvWriteCacheHandler();

                void append(char *data, int size);
                bool write(bool force = false);
                void close();
                void closeNow();

                signals:
                void done();                              // ->  DccTransferRecv::writeDone()
                                                          // ->  DccTransferRecv::slotWriteError()
                void gotError(const QString &errorString);

            protected slots:
                                                          // <-  m_transferJob->dataReq()
                void slotKIODataReq(KIO::Job *job, QByteArray &data);
                void slotKIOResult(KJob *job);          // <-  m_transferJob->result()

            protected:
                KIO::TransferJob *m_transferJob;
                bool m_writeAsyncMode;
                bool m_writeReady;

                QList<QByteArray> m_cacheList;
                QDataStream *m_cacheStream;
        };
    }
}

#endif  // TRANSFERRECV_H
