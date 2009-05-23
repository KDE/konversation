/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include "transfermanager.h" ////// header renamed
#include "transferrecv.h" ////// header renamed
#include "transfersend.h" ////// header renamed
#include "application.h"
#include "preferences.h"

#include <kdebug.h>


DccTransferManager::DccTransferManager( QObject* parent )
    : QObject( parent )
{
    // initial number
    m_nextReverseTokenNumber = 1001;

    m_defaultIncomingFolder = Preferences::self()->dccPath();

    connect( KonversationApplication::instance(), SIGNAL( appearanceChanged() ),
             this, SLOT( slotSettingsChanged() ) );
}

DccTransferManager::~DccTransferManager()
{
    m_sendItems.clear();
    m_recvItems.clear();
}

DccTransferRecv* DccTransferManager::newDownload()
{
    DccTransferRecv* transfer = new DccTransferRecv(this);
    m_recvItems.push_back( transfer );
    connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeRecvItem( DccTransfer* ) ) );
    initTransfer( transfer );
    return transfer;
}

DccTransferSend* DccTransferManager::newUpload()
{
    DccTransferSend* transfer = new DccTransferSend(this);
    m_sendItems.push_back( transfer );
    connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeSendItem( DccTransfer* ) ) );
    initTransfer( transfer );
    return transfer;
}

DccTransferSend* DccTransferManager::rejectSend(int connectionId, const QString& partnerNick, const QString& fileName)
{
    DccTransferSend* transfer = 0;

    // find applicable one
    foreach (DccTransferSend* it, m_sendItems )
    {
        if ( ( it->getStatus() == DccTransfer::Queued || it->getStatus() == DccTransfer::WaitingRemote ) &&
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

DccTransferRecv* DccTransferManager::resumeDownload( int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position )
{
    DccTransferRecv* transfer = 0;

    // find applicable one
    foreach (DccTransferRecv* it, m_recvItems )
    {
        if ( ( it->getStatus() == DccTransfer::Queued || it->getStatus() == DccTransfer::WaitingRemote ) &&
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

DccTransferSend* DccTransferManager::resumeUpload( int connectionId, const QString& partnerNick, const QString& fileName, uint ownPort, unsigned long position )
{
    DccTransferSend* transfer = 0;

    // find applicable one
    foreach ( DccTransferSend* it, m_sendItems )
    {
        if ( ( it->getStatus() == DccTransfer::Queued || it->getStatus() == DccTransfer::WaitingRemote ) &&
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

DccTransferSend* DccTransferManager::startReverseSending( int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, uint partnerPort, unsigned long fileSize, const QString& token )
{
    kDebug() << "Server group ID: " << connectionId << ", partner: " << partnerNick << ", filename: " << fileName << ", partner IP: " << partnerHost << ", parnter port: " << partnerPort << ", filesize: " << fileSize << ", token: " << token;
    DccTransferSend* transfer = 0;

    // find applicable one
    foreach ( DccTransferSend* it, m_sendItems )
    {
        if (
            it->getStatus() == DccTransfer::WaitingRemote &&
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

void DccTransferManager::initTransfer( DccTransfer* transfer )
{
    connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotTransferStatusChanged( DccTransfer*, int, int ) ) );

    emit newTransferAdded( transfer );
}

bool DccTransferManager::isLocalFileInWritingProcess( const KUrl& url ) const
{
    foreach ( DccTransferRecv* it, m_recvItems )
    {
        if ( ( it->getStatus() == DccTransfer::Connecting ||
               it->getStatus() == DccTransfer::Transferring ) &&
            it->getFileURL() == url )
        {
            return true;
        }
    }
    return false;
}

int DccTransferManager::generateReverseTokenNumber()
{
    return m_nextReverseTokenNumber++;
}

bool DccTransferManager::hasActiveTransfers()
{
    foreach ( DccTransferSend* it, m_sendItems )
    {
        if (it->getStatus() == DccTransfer::Transferring)
            return true;
    }

    foreach ( DccTransferRecv* it, m_recvItems )
    {
        if (it->getStatus() == DccTransfer::Transferring)
            return true;
    }

    return false;
}

void DccTransferManager::slotTransferStatusChanged( DccTransfer* item, int newStatus, int oldStatus )
{
    kDebug() << oldStatus << " -> " << newStatus << " " << item->getFileName() << " (" << item->getType() << ")";

    if ( newStatus == DccTransfer::Queued )
        emit newTransferQueued( item );
}

void DccTransferManager::slotSettingsChanged()
{
    // update the default incoming directory for already existed DCCRECV items
    if ( Preferences::self()->dccPath() != m_defaultIncomingFolder )
    {
        foreach ( DccTransferRecv* it, m_recvItems )
        {
            if ( it->getStatus() == DccTransfer::Queued &&
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

void DccTransferManager::removeSendItem( DccTransfer* item_ )
{
    DccTransferSend* item = static_cast< DccTransferSend* > ( item_ );
    m_sendItems.removeOne( item );
    item->deleteLater();
}

void DccTransferManager::removeRecvItem( DccTransfer* item_ )
{
    DccTransferRecv* item = static_cast< DccTransferRecv* > ( item_ );
    m_recvItems.removeOne( item );
    item->deleteLater();
}

#include "transfermanager.moc"
