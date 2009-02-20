/*
  send a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef DCCTRANSFERSEND_H
#define DCCTRANSFERSEND_H

#include "transfer.h" ////// header renamed

#include <qfile.h>
#include <QAbstractSocket>

class QTimer;
class QTcpServer;
class QTcpSocket;

class DccTransferSend : public DccTransfer
{
    Q_OBJECT

        public:
        DccTransferSend(QObject* parent);
        virtual ~DccTransferSend();

        // REQUIRED
        void setFileURL( const KUrl& url );
        // OPTIONAL
        void setFileName( const QString& fileName );
        // REQUIED
        // FIXME: this setting should be an optional one or be removed: make DccTransferSend itself read the configuration
        void setOwnIp( const QString& ownIp );
        // OPTIONAL
        void setFileSize( KIO::filesize_t fileSize );
        // OPTIONAL
        void setReverse( bool reverse );

        bool setResume( unsigned long position );

    public slots:
        virtual bool queue();
        virtual void start();
        virtual void abort();

        // invoked when the receiver accepts the offer (Reverse DCC)
        void connectToReceiver( const QString& partnerHost, uint partnerPort );

    protected slots:
        void acceptClient();
        // it must be invoked when m_sendSocket is ready
        void startSending();
        void writeData();
        void getAck();
        void slotGotSocketError( int errorCode );
        void slotConnectionTimeout();
        void slotConnectionFailed( QAbstractSocket::SocketError errorCode );
        void slotSendSocketClosed();
        void slotServerSocketClosed();

    protected:
        void cleanUp();
        void failed(const QString& errorMessage = QString() );

        void startConnectionTimer( int sec );
        void stopConnectionTimer();

        QString getQFileErrorString( int code );

        QFile m_file;

        /*The filename of the temporary file that we downloaded.  So if send a file ftp://somewhere/file.txt
         * Then this will be downloaded to /tmp.
         */
        QString m_tmpFile;

        QTcpServer *m_serverSocket;
        QTcpSocket *m_sendSocket;
        bool m_fastSend;

        QTimer* m_connectionTimer;
};

#endif  // DCCTRANSFERSEND_H
