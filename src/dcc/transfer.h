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

typedef qreal transferspeed_t;

class DccTransfer : public QObject
{
    Q_OBJECT

    public:
        enum DccType
        {
            Receive,
            Send
        };

        enum DccStatus
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

        DccTransfer( DccType dccType, QObject* parent );
        virtual ~DccTransfer();

        // info of DccTransfer can be copied with this constructor.
        DccTransfer( const DccTransfer& obj );

        DccType            getType()                  const;
        DccStatus          getStatus()                const;
        const QString&     getStatusDetail()          const;
        QDateTime          getTimeOffer()             const;
        int                getConnectionId()          const;
        QString            getOwnIp()                 const;
        uint               getOwnPort()               const;
        QString            getPartnerNick()           const;
        QString            getPartnerIp()             const;
        uint               getPartnerPort()           const;
        QString            getFileName()              const;
        KIO::filesize_t    getFileSize()              const;
        KIO::fileoffset_t  getTransferringPosition()  const;
        KIO::fileoffset_t  getTransferStartPosition() const;
        KUrl               getFileURL()               const;
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
        void setConnectionId( int connectionId );
        // REQUIRED
        void setPartnerNick( const QString& nick );

    signals:
        void transferStarted( DccTransfer* item );
        void done( DccTransfer* item );
        void statusChanged( DccTransfer* item, int newStatus, int oldStatus );

    public slots:
        virtual bool queue();
        virtual void start() {};
        virtual void abort() {};

    protected:
        void setStatus( DccStatus status, const QString& statusDetail = QString() );
        void startTransferLogger();
        void finishTransferLogger();

        static QString transferFileName( const QString& fileName );
        static QString sanitizeFileName( const QString& fileName );
        static unsigned long intel( unsigned long value );

    protected slots:
        void logTransfer();

    protected:
        // transfer information
        DccType m_type;
        DccStatus m_status;
        QString m_statusDetail;
        bool m_resumed;
        bool m_reverse;
        QString m_reverseToken;
        KIO::fileoffset_t m_transferringPosition;
        KIO::fileoffset_t m_transferStartPosition;

        /*
        QValueList<QDateTime> m_transferTimeLog;  // write per packet to calc CPS
        QValueList<KIO::fileoffset_t> m_transferPositionLog;  // write per packet to calc CPS
        */

        // we'll communicate with the partner via this server
        int m_connectionId;
        QString m_partnerNick;
        QString m_partnerIp;                      // null when unknown
        uint m_partnerPort;
        QString m_ownIp;
        uint m_ownPort;

        unsigned long m_bufferSize;
        char* m_buffer;

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
        KUrl m_fileURL;

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
        QList<int> m_transferLogTime;
        QList<KIO::fileoffset_t> m_transferLogPosition;

        transferspeed_t m_averageSpeed;
        transferspeed_t m_currentSpeed;
        int m_timeLeft;
};

#endif  // DCCTRANSFER_H
