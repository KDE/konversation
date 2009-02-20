/*
  DccTransferManager controls DccTransfer instances.
  All DccTransferRecv/DccTransferSend instances are created and deleted by this class.
  Each DccTransfer instance is deleted immediately after its transfer done.
*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#ifndef DCCTRANSFERMANAGER_H
#define DCCTRANSFERMANAGER_H

#include "transfer.h"

#include <qobject.h>


class KUrl;

class DccTransferRecv;
class DccTransferSend;

class DccTransferManager : public QObject
{
    Q_OBJECT

    public:
        DccTransferManager( QObject* parent = 0 );
        ~DccTransferManager();

    signals:
        /*
         * The status of the item is DccTransfer::Configuring when this signal is emitted.
         */
        void newTransferAdded( DccTransfer* transfer );
        /*
         * The status of the item is DccTransfer::Queued when this signal is emitted.
         */
        void newTransferQueued( DccTransfer* transfer );

        void fileURLChanged( DccTransferRecv* transfer );

    public:
        DccTransferRecv* newDownload();
        DccTransferSend* newUpload();

        /**
         * @return a DccTransferRecv item if applicable one found, otherwise 0.
         */
        DccTransferRecv* resumeDownload(int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position );

        /**
         * @return a DccTransferSend item if applicable one found, otherwise 0.
         */
        DccTransferSend* resumeUpload(int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position );

        DccTransferSend* startReverseSending(int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, uint partnerPort, unsigned long fileSize, const QString& token );

        bool isLocalFileInWritingProcess( const KUrl& localUrl ) const;

        int generateReverseTokenNumber();

        bool hasActiveTransfers();

    private:
        /*
         * initTransfer() does the common jobs for newDownload() and newUpload()
         */
        void initTransfer( DccTransfer* transfer );

    private slots:
        void slotTransferStatusChanged( DccTransfer* item, int newStatus, int oldStatus );
        void removeSendItem( DccTransfer* item );
        void removeRecvItem( DccTransfer* item );

        void slotSettingsChanged();

    private:
        QList< DccTransferSend* > m_sendItems;
        QList< DccTransferRecv* > m_recvItems;

        int m_nextReverseTokenNumber;
        KUrl m_defaultIncomingFolder;  // store here to know if this settings is changed
};

#endif  // DCCTRANSFERMANAGER_H
