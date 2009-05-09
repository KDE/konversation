/*
  receive a file on DCC protocol
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

#ifndef DCCTRANSFERRECV_H
#define DCCTRANSFERRECV_H

#include "transfer.h" ////// header renamed
// TODO: remove the dependence
#include "resumedialog.h" ////// header renamed

#include <QAbstractSocket>

class QTimer;
class QTcpServer;
class QTcpSocket;

namespace KIO
{
    class Job;
    class TransferJob;
}

class DccTransferRecvWriteCacheHandler;

class DccTransferRecv : public DccTransfer
{
    Q_OBJECT

    public:
        DccTransferRecv(QObject* parent);
        virtual ~DccTransferRecv();

        // REQUIRED
        void setPartnerIp( const QString& ip );
        // REQUIRED
        void setPartnerPort( uint port );
        // REQUIRED
        void setFileSize( unsigned long fileSize );
        // OPTIONAL, if not specified, "unnamed_file"
        // TODO: "$sendername-$receiveddate" is better
        void setFileName( const QString& fileName );
        // OPTIONAL, if not specified, default folder + the file name
        void setFileURL( const KUrl& url );
        // OPTIONAL
        void setReverse( bool reverse, const QString& reverseToken );

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
        void startResume( unsigned long position );

    protected slots:
        // Local KIO
        void slotLocalCanResume( KIO::Job* job, KIO::filesize_t size );
        void slotLocalGotResult( KJob* job );
        void slotLocalReady( KIO::Job* job );
        void slotLocalWriteDone();
        void slotLocalGotWriteError( const QString& errorString );

        // Remote DCC
        void connectWithSender();
        void startReceiving();
        void connectionFailed( QAbstractSocket::SocketError errorCode );
        void readData();
        void sendAck();
        void connectionTimeout();
        //void slotSocketClosed(); //same as connectionFailed

        // Reverse DCC
        void slotServerSocketReadyAccept();
        void slotServerSocketGotError( int errorCode );

    protected:
        void cleanUp();

                                                  // (startPosition == 0) means "don't resume"
        void prepareLocalKio( bool overwrite, bool resume, KIO::fileoffset_t startPosition = 0 );
        void askAndPrepareLocalKio( const QString& message, int enabledActions, DccResumeDialog::ReceiveAction defaultAction, KIO::fileoffset_t startPosition = 0 );

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

        void startConnectionTimer( int sec );
        void stopConnectionTimer();

    protected:
        KUrl m_saveToTmpFileURL;
        ///Current filesize of the file saved on the disk.
        KIO::filesize_t m_saveToFileSize;
        ///Current filesize of the file+".part" saved on the disk.
        KIO::filesize_t m_partialFileSize;
        DccTransferRecvWriteCacheHandler* m_writeCacheHandler;
        bool m_saveToFileExists;
        bool m_partialFileExists;
        QTimer* m_connectionTimer;

        QTcpServer* m_serverSocket;
        QTcpSocket* m_recvSocket;

        ///We need the original name for resume communication, as sender depends on it
        QString m_saveFileName;

    private:
        virtual QString getTypeText() const;
        virtual QPixmap getTypeIcon() const;
};

class DccTransferRecvWriteCacheHandler : public QObject
{
    Q_OBJECT

        public:
        explicit DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob );
        virtual ~DccTransferRecvWriteCacheHandler();

        void append( char* data, int size );
        bool write( bool force = false );
        void close();
        void closeNow();

        signals:
        void dataFinished();                      // ->  m_transferJob->slotFinished()
        void done();                              // ->  DccTransferRecv::writeDone()
                                                  // ->  DccTransferRecv::slotWriteError()
        void gotError( const QString& errorString );

    protected slots:
                                                  // <-  m_transferJob->dataReq()
        void slotKIODataReq( KIO::Job* job, QByteArray& data );
        void slotKIOResult( KJob* job );          // <-  m_transferJob->result()

    protected:
        KIO::TransferJob* m_transferJob;
        bool m_writeAsyncMode;
        bool m_writeReady;

        QList<QByteArray> m_cacheList;
        QDataStream* m_cacheStream;
};

#endif  // DCCTRANSFERRECV_H
