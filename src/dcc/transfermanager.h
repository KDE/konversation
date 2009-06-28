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
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include <QObject>

#include <KUrl>

namespace Konversation
{
    namespace UPnP
    {
        class UPnPMCastSocket;
        class UPnPRouter;
    }

    namespace DCC
    {
        class Transfer;
        class TransferRecv;
        class TransferSend;

        class TransferManager : public QObject
        {
            Q_OBJECT

            public:
                TransferManager( QObject* parent = 0 );
                ~TransferManager();

            signals:
                /*
                 * The status of the item is DccTransfer::Configuring when this signal is emitted.
                 */
                void newTransferAdded( Konversation::DCC::Transfer* transfer );
                /*
                 * The status of the item is DccTransfer::Queued when this signal is emitted.
                 */
                void newDccTransferQueued( Konversation::DCC::Transfer* transfer );

                void fileURLChanged( Konversation::DCC::TransferRecv* transfer );

            public:
                TransferRecv* newDownload();
                TransferSend* newUpload();

                TransferSend* rejectSend(int connectionId, const QString& partnerNick, const QString& fileName);

                /**
                 * @return a DccTransferRecv item if applicable one found, otherwise 0.
                 */
                TransferRecv* resumeDownload(int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position );

                /**
                 * @return a DccTransferSend item if applicable one found, otherwise 0.
                 */
                TransferSend* resumeUpload(int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position );

                TransferSend* startReverseSending(int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, uint partnerPort, unsigned long fileSize, const QString& token );

                bool isLocalFileInWritingProcess( const KUrl& localUrl ) const;

                int generateReverseTokenNumber();

                bool hasActiveTransfers();

                UPnP::UPnPRouter *getUPnPRouter() { return m_upnpRouter; }
                void startupUPnP(void);
                void shutdownUPnP(void);

            private:
                /*
                 * initTransfer() does the common jobs for newDownload() and newUpload()
                 */
                void initTransfer( Transfer* transfer );

            private slots:
                void slotTransferStatusChanged( Konversation::DCC::Transfer* item, int newStatus, int oldStatus );
                void removeSendItem( Konversation::DCC::Transfer* item );
                void removeRecvItem( Konversation::DCC::Transfer* item );

                void slotSettingsChanged();

                void upnpRouterDiscovered(Konversation::UPnP::UPnPRouter *router);

            private:
                QList< TransferSend* > m_sendItems;
                QList< TransferRecv* > m_recvItems;

                UPnP::UPnPMCastSocket *m_upnpSocket;
                UPnP::UPnPRouter *m_upnpRouter;

                int m_nextReverseTokenNumber;
                KUrl m_defaultIncomingFolder;  // store here to know if this settings is changed
        };
    }
}

#endif  // TRANSFERMANAGER_H
