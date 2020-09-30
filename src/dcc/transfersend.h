/*
  send a file on DCC protocol
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

#ifndef TRANSFERSEND_H
#define TRANSFERSEND_H

#include "transfer.h"

#include <QFile>
#include <QAbstractSocket>

class QTimer;
class QTcpServer;
class QTcpSocket;
class QTemporaryFile;

namespace Konversation
{
    namespace DCC
    {
        class TransferSend : public Transfer
        {
            Q_OBJECT

            public:
                explicit TransferSend(QObject *parent);
                ~TransferSend() override;

                // REQUIRED
                void setFileURL(const QUrl &url);
                // OPTIONAL
                void setFileName(const QString &fileName);
                // OPTIONAL
                void setOwnIp(const QString &ownIp);
                // OPTIONAL
                void setFileSize(KIO::filesize_t fileSize);
                // OPTIONAL
                void setReverse(bool reverse);

                bool setResume(quint64 position);

                // send got rejected
                void reject();

            public Q_SLOTS:
                bool queue() override;
                void start() override;
                void abort() override;

                // invoked when the receiver accepts the offer (Reverse DCC)
                void connectToReceiver(const QString &partnerHost, quint16 partnerPort);

            protected Q_SLOTS:
                void acceptClient();
                // it must be invoked when m_sendSocket is ready
                void startSending();
                void writeData();
                void bytesWritten(qint64 bytes);
                void getAck();
                void slotGotSocketError(QAbstractSocket::SocketError errorCode);
                void slotConnectionTimeout();
                void sendRequest(bool error, quint16 port);
                void slotLocalCopyReady(KJob *job);

            protected:
                void cleanUp() override;

                void startConnectionTimer(int secs);
                void stopConnectionTimer();

                QString getQFileErrorString(int code);

                QFile m_file;

                /* A temporary file that contains a local copy of a file.
                 *
                 * If a URL like "ftp://somewhere/file.txt" is specified,
                 * it is downloaded to QDir::tempPath and deleted when the
                 * client exits or the download is complete.
                 */
                QTemporaryFile *m_tmpFile;

                QTcpServer *m_serverSocket;
                QTcpSocket *m_sendSocket;
                bool m_fastSend;

                QTimer *m_connectionTimer;
        };
    }
}

#endif  // TRANSFERSEND_H
