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

#include <qobject.h>
#include <qvaluelist.h>

#include "dcctransfer.h"

class KURL;

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
         * This signal is provided for the convenience for frontend code.
         */
        void newTransferQueued( DccTransfer* transfer );

    public:
        DccTransferRecv* newDownload();
        DccTransferSend* newUpload();

        DccTransfer* searchStandbyTransferByFileName( const QString& fileName, DccTransfer::DccType type, bool resumed = false );
        DccTransfer* searchStandbyTransferByOwnPort( const QString& ownPort, DccTransfer::DccType type, bool resumed = false );

        bool isLocalFileInWritingProcess( const KURL& localUrl );

    private:
        /*
         * initTransfer() does the common jobs for newDownload() and newUpload()
         */
        void initTransfer( DccTransfer* transfer );

    private slots:
        void slotTransferStatusChanged( DccTransfer* item, int newStatus, int oldStatus );
        void removeItem( DccTransfer* item );

    private:
        QValueList< DccTransfer* > m_transfers;
};

#endif  // DCCTRANSFERMANAGER_H
