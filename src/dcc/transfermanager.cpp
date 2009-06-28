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

#include "transfermanager.h"
#include "transferrecv.h"
#include "transfersend.h"
#include "application.h"
#include "preferences.h"
#include "upnpmcastsocket.h"
#include "upnprouter.h"
#include "transfer.h"

using namespace Konversation::UPnP;

namespace Konversation
{
    namespace DCC
    {
        TransferManager::TransferManager( QObject* parent )
            : QObject( parent )
        {
            // initial number
            m_nextReverseTokenNumber = 1001;

            m_defaultIncomingFolder = Preferences::self()->dccPath();

            connect( Application::instance(), SIGNAL( appearanceChanged() ),
                     this, SLOT( slotSettingsChanged() ) );

            m_upnpRouter = NULL;
            m_upnpSocket = NULL;

            if (Preferences::self()->dccUPnP())
                startupUPnP();
        }

        TransferManager::~TransferManager()
        {
            m_sendItems.clear();
            m_recvItems.clear();

            shutdownUPnP();
        }

        void TransferManager::startupUPnP(void)
        {
            m_upnpSocket = new UPnPMCastSocket();

            connect(m_upnpSocket, SIGNAL( discovered(Konversation::UPnP::UPnPRouter *) ),
                    this, SLOT( upnpRouterDiscovered(Konversation::UPnP::UPnPRouter *) ) );

            m_upnpSocket->discover();
        }

        void TransferManager::shutdownUPnP(void)
        {
            // This deletes the router too.
            if (m_upnpSocket) delete m_upnpSocket;
            m_upnpSocket = NULL;
            m_upnpRouter = NULL;
        }

        TransferRecv* TransferManager::newDownload()
        {
            TransferRecv* transfer = new TransferRecv(this);
            m_recvItems.push_back( transfer );
            connect( transfer, SIGNAL( done( Konversation::DCC::Transfer* ) ), this, SLOT( removeRecvItem( Konversation::DCC::Transfer* ) ) );
            initTransfer( transfer );
            return transfer;
        }

        TransferSend* TransferManager::newUpload()
        {
            TransferSend* transfer = new TransferSend(this);
            m_sendItems.push_back( transfer );
            connect( transfer, SIGNAL( done( Konversation::DCC::Transfer* ) ), this, SLOT( removeSendItem( Konversation::DCC::Transfer* ) ) );
            initTransfer( transfer );
            return transfer;
        }

        TransferSend* TransferManager::rejectSend(int connectionId, const QString& partnerNick, const QString& fileName)
        {
            TransferSend* transfer = 0;

            // find applicable one
            foreach (TransferSend* it, m_sendItems )
            {
                if ( ( it->getStatus() == Transfer::Queued || it->getStatus() == Transfer::WaitingRemote ) &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName )
                {
                    transfer = it;
                    kDebug() << "Filename match: " << fileName;
                    break;
                }
            }

            if ( transfer )
                transfer->reject();

            return transfer;
        }

        TransferRecv* TransferManager::resumeDownload( int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position )
        {
            TransferRecv* transfer = 0;

            // find applicable one
            foreach (TransferRecv* it, m_recvItems )
            {
                if ( ( it->getStatus() == Transfer::Queued || it->getStatus() == Transfer::WaitingRemote ) &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName &&
                    it->isResumed() )
                {
                    transfer = it;
                    kDebug() << "Filename match: " << fileName << ", claimed port: " << ownPort << ", item port: " << transfer->getOwnPort();
                    // the port number can be changed behind NAT, so we pick an item which only the filename is correspondent in that case.
                    if ( transfer->getOwnPort() == ownPort )
                    {
                        break;
                    }
                }
            }

            if ( transfer )
                transfer->startResume( position );

            return transfer;
        }

        TransferSend* TransferManager::resumeUpload( int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position )
        {
            TransferSend* transfer = 0;

            // find applicable one
            foreach ( TransferSend* it, m_sendItems )
            {
                if ( ( it->getStatus() == Transfer::Queued || it->getStatus() == Transfer::WaitingRemote ) &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName &&
                    !it->isResumed() )
                {
                    transfer = it;
                    kDebug() << "Filename match: " << fileName << ", claimed port: " << ownPort << ", item port: " << transfer->getOwnPort();
                    // the port number can be changed behind NAT, so we pick an item which only the filename is correspondent in that case.
                    if ( transfer->getOwnPort() == ownPort )
                    {
                        break;
                    }
                }
            }

            if ( transfer )
                transfer->setResume( position );

            return transfer;
        }

        TransferSend* TransferManager::startReverseSending( int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, uint partnerPort, unsigned long fileSize, const QString& token )
        {
            kDebug() << "Server group ID: " << connectionId << ", partner: " << partnerNick << ", filename: " << fileName << ", partner IP: " << partnerHost << ", parnter port: " << partnerPort << ", filesize: " << fileSize << ", token: " << token;
            TransferSend* transfer = 0;

            // find applicable one
            foreach ( TransferSend* it, m_sendItems )
            {
                if (
                    it->getStatus() == Transfer::WaitingRemote &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName &&
                    it->getFileSize() == fileSize &&
                    it->getReverseToken() == token
                )
                {
                    transfer = it;
                    break;
                }
            }

            if ( transfer )
                transfer->connectToReceiver( partnerHost, partnerPort );

            return transfer;
        }

        void TransferManager::initTransfer( Transfer* transfer )
        {
            connect( transfer, SIGNAL( statusChanged( Konversation::DCC::Transfer*, int, int ) ), this, SLOT( slotTransferStatusChanged( Konversation::DCC::Transfer*, int, int ) ) );

            emit newTransferAdded( transfer );
        }

        bool TransferManager::isLocalFileInWritingProcess( const KUrl& url ) const
        {
            foreach ( TransferRecv* it, m_recvItems )
            {
                if ( ( it->getStatus() == Transfer::Connecting ||
                       it->getStatus() == Transfer::Transferring ) &&
                    it->getFileURL() == url )
                {
                    return true;
                }
            }
            return false;
        }

        int TransferManager::generateReverseTokenNumber()
        {
            return m_nextReverseTokenNumber++;
        }

        bool TransferManager::hasActiveTransfers()
        {
            foreach ( TransferSend* it, m_sendItems )
            {
                if (it->getStatus() == Transfer::Transferring)
                    return true;
            }

            foreach ( TransferRecv* it, m_recvItems )
            {
                if (it->getStatus() == Transfer::Transferring)
                    return true;
            }

            return false;
        }

        void TransferManager::slotTransferStatusChanged( Transfer* item, int newStatus, int oldStatus )
        {
            kDebug() << oldStatus << " -> " << newStatus << " " << item->getFileName() << " (" << item->getType() << ")";

            if ( newStatus == Transfer::Queued )
                emit newDccTransferQueued( item );
        }

        void TransferManager::slotSettingsChanged()
        {
            // update the default incoming directory for already existed DCCRECV items
            if ( Preferences::self()->dccPath() != m_defaultIncomingFolder )
            {
                foreach ( TransferRecv* it, m_recvItems )
                {
                    if ( it->getStatus() == Transfer::Queued &&
                         it->getFileURL().directory() == m_defaultIncomingFolder.pathOrUrl() )
                    {
                        KUrl url;
                        url.setDirectory( Preferences::self()->dccPath().pathOrUrl() );
                        url.setFileName( it->getFileURL().fileName() );
                        it->setFileURL( url );

                        emit fileURLChanged( it );
                    }
                }

                m_defaultIncomingFolder = Preferences::self()->dccPath();
            }
        }

        void TransferManager::removeSendItem( Transfer* item_ )
        {
            TransferSend* item = static_cast< TransferSend* > ( item_ );
            m_sendItems.removeOne( item );
            item->deleteLater();
        }

        void TransferManager::removeRecvItem( Transfer* item_ )
        {
            TransferRecv* item = static_cast< TransferRecv* > ( item_ );
            m_recvItems.removeOne( item );
            item->deleteLater();
        }

        void TransferManager::upnpRouterDiscovered(UPnPRouter *router)
        {
            kDebug() << "Router discovered!" << endl;

            // Assuming only 1 router for now
            m_upnpRouter = router;
        }

    }
}

#include "transfermanager.moc"
