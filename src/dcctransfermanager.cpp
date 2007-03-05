/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include <kdebug.h>

#include "dcctransferrecv.h"
#include "dcctransfersend.h"

#include "dcctransfermanager.h"

DccTransferManager::DccTransferManager( QObject* parent )
    : QObject( parent )
{
}

DccTransferManager::~DccTransferManager()
{
    // delete all items
    QValueListIterator< DccTransfer* > it = m_transfers.begin();
    while ( it != m_transfers.end() )
    {
        delete (*it);
        it = m_transfers.erase( it );
    }
}

DccTransferRecv* DccTransferManager::newDownload()
{
    DccTransferRecv* transfer = new DccTransferRecv();
    initTransfer( transfer );
    return transfer;
}

DccTransferSend* DccTransferManager::newUpload()
{
    DccTransferSend* transfer = new DccTransferSend();
    initTransfer( transfer );
    return transfer;
}

void DccTransferManager::initTransfer( DccTransfer* transfer )
{
    m_transfers.push_back( transfer );

    connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotTransferStatusChanged( DccTransfer*, int, int ) ) );
    connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeItem( DccTransfer* ) ) );

    emit newTransferAdded( transfer );
}

DccTransfer* DccTransferManager::searchStandbyTransferByFileName( const QString& fileName, DccTransfer::DccType type, bool resumed )
{
    QValueListConstIterator< DccTransfer* > it;
    for ( it = m_transfers.begin() ; it != m_transfers.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getFileName() == fileName &&
             (*it)->getType() == type &&
             !(resumed && !(*it)->isResumed() ) )  // <- what the hell does it mean? (strm)
        {
            return *it;
        }
    }
    return 0;
}

DccTransfer* DccTransferManager::searchStandbyTransferByOwnPort( const QString& ownPort, DccTransfer::DccType type, bool resumed )
{
    QValueListConstIterator< DccTransfer* > it;
    for ( it = m_transfers.begin() ; it != m_transfers.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getOwnPort() == ownPort &&
             (*it)->getType() == type &&
             !(resumed && !(*it)->isResumed() ) )  // <- what the hell does it mean? (strm)
        {
            return *it;
        }
    }
    return 0;
}

bool DccTransferManager::isLocalFileInWritingProcess( const KURL& url )
{
    QValueListConstIterator< DccTransfer* > it;
    for ( it = m_transfers.begin() ; it != m_transfers.end() ; ++it )
    {
        if ( (*it)->getType() == DccTransfer::Receive &&
             ( (*it)->getStatus() == DccTransfer::Connecting ||
               (*it)->getStatus() == DccTransfer::Transferring ) &&
             (*it)->getFileURL() == url )
        {
            return true;
        }
    }
    return false;
}

void DccTransferManager::slotTransferStatusChanged( DccTransfer* item, int newStatus, int oldStatus )
{
    kdDebug() << "DccTransferManager::slotTransferStatusChanged(): " << oldStatus << " -> " << newStatus << " " << item->getFileName() << " (" << item->getType() << ")" << endl;

    if ( newStatus == DccTransfer::Queued )
        emit newTransferQueued( item );
}

void DccTransferManager::removeItem( DccTransfer* item )
{
    item->deleteLater();
    m_transfers.remove( item );
}

#include "dcctransfermanager.moc"
