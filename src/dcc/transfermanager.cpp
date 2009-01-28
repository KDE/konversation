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
#include "konversationapplication.h"
#include "preferences.h"

#include <kdebug.h>


DccTransferManager::DccTransferManager( QObject* parent )
    : QObject( parent )
{
    // initial number
    m_nextReverseTokenNumber = 1001;

    m_defaultIncomingFolder = Preferences::dccPath();

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

DccTransferRecv* DccTransferManager::resumeDownload( int connectionId, const QString& partnerNick, const QString& fileName, const QString& ownPort, unsigned long position )
{
    DccTransferRecv* transfer = 0;

    // find applicable one
    Q3ValueListConstIterator< DccTransferRecv* > it;
    for ( it = m_recvItems.begin() ; it != m_recvItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getConnectionId() == connectionId &&
             (*it)->getPartnerNick() == partnerNick &&
             (*it)->getFileName() == fileName &&
             (*it)->isResumed() )
        {
            transfer = (*it);
            kDebug() << "DccTransferManager::resumeDownload(): filename match: " << fileName << ", claimed port: " << ownPort << ", item port: " << transfer->getOwnPort() << endl;
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

DccTransferSend* DccTransferManager::resumeUpload( int connectionId, const QString& partnerNick, const QString& fileName, const QString& ownPort, unsigned long position )
{
    DccTransferSend* transfer = 0;

    // find applicable one
    Q3ValueListConstIterator< DccTransferSend* > it;
    for ( it = m_sendItems.begin() ; it != m_sendItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getConnectionId() == connectionId &&
             (*it)->getPartnerNick() == partnerNick &&
             (*it)->getFileName() == fileName &&
             !(*it)->isResumed() )
        {
            transfer = (*it);
            kDebug() << "DccTransferManager::resumeUpload(): filename match: " << fileName << ", claimed port: " << ownPort << ", item port: " << transfer->getOwnPort() << endl;
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

DccTransferSend* DccTransferManager::startReverseSending( int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, const QString& partnerPort, unsigned long fileSize, const QString& token )
{
    kDebug() << "DccTransferManager::startReverseSending(): server group ID: " << connectionId << ", partner: " << partnerNick << ", filename: " << fileName << ", partner IP: " << partnerHost << ", parnter port: " << partnerPort << ", filesize: " << fileSize << ", token: " << token << endl;
    DccTransferSend* transfer = 0;

    // find applicable one
    Q3ValueListConstIterator< DccTransferSend* > it;
    for ( it = m_sendItems.begin() ; it != m_sendItems.end() ; ++it )
    {
        if (
            (*it)->getStatus() == DccTransfer::WaitingRemote &&
            (*it)->getConnectionId() == connectionId &&
            (*it)->getPartnerNick() == partnerNick &&
            (*it)->getFileName() == fileName &&
            (*it)->getFileSize() == fileSize &&
            (*it)->getReverseToken() == token
        )
        {
            transfer = (*it);
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
    Q3ValueListConstIterator< DccTransferRecv* > it;
    for ( it = m_recvItems.begin() ; it != m_recvItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Connecting ||
               (*it)->getStatus() == DccTransfer::Transferring ) &&
             (*it)->getFileURL() == url )
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
    Q3ValueListConstIterator< DccTransferSend* > it;
    for ( it = m_sendItems.begin() ; it != m_sendItems.end() ; ++it )
    {
        if ((*it)->getStatus() == DccTransfer::Transferring)
            return true;
    }

    Q3ValueListConstIterator< DccTransferRecv* > it2;
    for ( it2 = m_recvItems.begin() ; it2 != m_recvItems.end() ; ++it2 )
    {
        if ((*it2)->getStatus() == DccTransfer::Transferring)
            return true;
    }

    return false;
}

void DccTransferManager::slotTransferStatusChanged( DccTransfer* item, int newStatus, int oldStatus )
{
    kDebug() << "DccTransferManager::slotTransferStatusChanged(): " << oldStatus << " -> " << newStatus << " " << item->getFileName() << " (" << item->getType() << ")" << endl;

    if ( newStatus == DccTransfer::Queued )
        emit newTransferQueued( item );
}

void DccTransferManager::slotSettingsChanged()
{
    // update the default incoming directory for already existed DCCRECV items
    if ( Preferences::dccPath() != m_defaultIncomingFolder )
    {
        QValueListConstIterator< DccTransferRecv* > it;
        for ( it = m_recvItems.begin() ; it != m_recvItems.end() ; ++it )
        {
            if ( (*it)->getStatus() == DccTransfer::Queued &&
                 (*it)->getFileURL().directory() == m_defaultIncomingFolder )
            {
                KURL url;
                url.setDirectory( Preferences::dccPath() );
                url.setFileName( (*it)->getFileURL().fileName() );
                (*it)->setFileURL( url );

                emit fileURLChanged( *it );
            }
        }

        m_defaultIncomingFolder = Preferences::dccPath();
    }
}

void DccTransferManager::removeSendItem( DccTransfer* item_ )
{
    DccTransferSend* item = static_cast< DccTransferSend* > ( item_ );
    m_sendItems.remove( item );
    item->deleteLater();
}

void DccTransferManager::removeRecvItem( DccTransfer* item_ )
{
    DccTransferRecv* item = static_cast< DccTransferRecv* > ( item_ );
    m_recvItems.remove( item );
    item->deleteLater();
}

// #include "./dcc/transfermanager.moc"
