/*
  This class represents a DCC transfer.
*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>
*/

#ifndef DCCTRANSFER_H
#define DCCTRANSFER_H

#include <qdatetime.h>
#include <qobject.h>
#include <qtimer.h>

#include <kurl.h>
#include <kio/global.h>

class DccTransfer : public QObject
{
    Q_OBJECT

    public:
        enum DccType
        {
            Send,
            Receive,
            DccTypeCount
        };

        enum DccStatus
        {
            Queued = 0,                           // Newly added DCC, RECV: Waiting for local user's response
            Preparing,                            // Opening KIO to write received data
            WaitingRemote,                        // Waiting for remote host's response
            Connecting,                           // RECV: trying to connect to the server
            Sending,                              // Sending
            Receiving,                            // Receiving
            Done,                                 // Transfer done
            Failed,                               // Transfer failed
            Aborted,                              // Transfer aborted by user
            DccStatusCount
        };

        enum UnavailableStatus
        {
            Calculating = -1,
            NotInTransfer = -2,
            InfiniteValue = -3,
        };

        DccTransfer( DccType dccType, const QString& partnerNick );
        virtual ~DccTransfer();

        // info of DccTransfer can be copied with this constructor.
        DccTransfer( const DccTransfer& obj );

        DccType            getType()                  const;
        DccStatus          getStatus()                const;
        const QString&     getStatusDetail()          const;
        QDateTime          getTimeOffer()             const;
        QString            getOwnIp()                 const;
        QString            getOwnPort()               const;
        QString            getPartnerNick()           const;
        QString            getPartnerIp()             const;
        QString            getPartnerPort()           const;
        QString            getFileName()              const;
        KIO::filesize_t    getFileSize()              const;
        KIO::fileoffset_t  getTransferringPosition()  const;
        KIO::fileoffset_t  getTransferStartPosition() const;
        KURL               getFileURL()               const;
        bool               isResumed()                const;
        long               getCurrentSpeed()          const;
        int                getTimeLeft()              const;
        int                getProgress()              const;
        QDateTime          getTimeTransferStarted()   const;
        QDateTime          getTimeTransferFinished()  const;

    signals:
        void transferStarted( DccTransfer* item );
        void done( DccTransfer* item );
        void statusChanged( DccTransfer* item, int newStatus, int oldStatus );

    public slots:
        virtual void start() {};
        virtual void abort() {};

    protected:
        void setStatus( DccStatus status, const QString& statusDetail = QString::null );
        void startTransferLogger();
        void finishTransferLogger();

        static QString getNumericalIpText( const QString& ipString );
        static unsigned long intel( unsigned long value );

    protected slots:
        void logTransfer();

    protected:
        // transfer information
        DccType m_dccType;
        DccStatus m_dccStatus;
        QString m_dccStatusDetail;
        bool m_resumed;
        KIO::fileoffset_t m_transferringPosition;
        KIO::fileoffset_t m_transferStartPosition;

        /*
        QValueList<QDateTime> m_transferTimeLog;  // write per packet to calc CPS
        QValueList<KIO::fileoffset_t> m_transferPositionLog;  // write per packet to calc CPS
        */

        QString m_partnerNick;
        QString m_partnerIp;                      // null when unknown
        QString m_partnerPort;
        QString m_ownIp;
        QString m_ownPort;

        unsigned long m_bufferSize;
        char* m_buffer;

        /**
         * The filename.
         * For receiving, it holds the filename as the sender said.
         * So be careful, it can contain "../" and so on.
         */
        QString m_fileName;

        /** The file size of the complete file sending/recieving. */
        KIO::filesize_t  m_fileSize;

        /**
         * If we are sending a file, this is the url of the file we are sending.
         * If we are recieving a file, this is the url of the file we are saving
         * to in the end (Temporararily it will be filename+".part" ).
         */
        KURL m_fileURL;

    private:
        DccTransfer& operator = ( const DccTransfer& obj );

        void updateTransferMeters();

    private:
        QDateTime m_timeOffer;
        QDateTime m_timeTransferStarted;
        //QDateTime m_timeLastActive;
        QDateTime m_timeTransferFinished;

        QTimer m_loggerTimer;
        QTime m_loggerBaseTime;  // for calculating CPS
        QValueList<int> m_transferLogTime;
        QValueList<KIO::fileoffset_t> m_transferLogPosition;

        // transfer meters;
        double m_currentSpeed;
        int m_timeLeft;
};

#endif  // DCCTRANSFER_H
