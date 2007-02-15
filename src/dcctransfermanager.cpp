/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

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

DccTransferRecv* DccTransferManager::newDownload( const QString& partnerNick, const KURL& defaultFolderURL, const QString& fileName, unsigned long fileSize, const QString& partnerIp, const QString& partnerPort )
{
    DccTransferRecv* transfer = new DccTransferRecv( partnerNick, defaultFolderURL, fileName, fileSize, partnerIp, partnerPort );
    m_transfers.push_back( transfer );
    // FIXME: workaround. it causes memory leak
    //connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeItem( DccTransfer* ) ) );
    emit newTransferAdded( transfer );
    return transfer;
}

DccTransferSend* DccTransferManager::newUpload( const QString& partnerNick, const KURL& fileURL, const QString& ownIp, const QString &altFileName, uint fileSize )
{
    DccTransferSend* transfer = new DccTransferSend( partnerNick, fileURL, ownIp, altFileName, fileSize );
    m_transfers.push_back( transfer );
    // FIXME: workaround. it causes memory leak.
    //connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeItem( DccTransfer* ) ) );
    emit newTransferAdded( transfer );
    return transfer;
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
               (*it)->getStatus() == DccTransfer::Receiving ) &&
             (*it)->getFileURL() == url )
        {
            return true;
        }
    }
    return false;
}

void DccTransferManager::removeItem( DccTransfer* item )
{
    item->deleteLater();
    m_transfers.remove( item );
}

#include "dcctransfermanager.moc"
