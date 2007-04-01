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
    m_nextPassiveSendTokenNumber = 1001;
}

DccTransferManager::~DccTransferManager()
{
    // delete SEND items
    QValueListIterator< DccTransferSend* > itSend = m_sendItems.begin();
    while ( itSend != m_sendItems.end() )
    {
        delete (*itSend);
        itSend = m_sendItems.erase( itSend );
    }
    // delete RECV items
    QValueListIterator< DccTransferRecv* > itRecv = m_recvItems.begin();
    while ( itRecv != m_recvItems.end() )
    {
        delete (*itRecv);
        itRecv = m_recvItems.erase( itRecv );
    }
}

DccTransferRecv* DccTransferManager::newDownload()
{
    DccTransferRecv* transfer = new DccTransferRecv();
    m_recvItems.push_back( transfer );
    connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeRecvItem( DccTransfer* ) ) );
    initTransfer( transfer );
    return transfer;
}

DccTransferSend* DccTransferManager::newUpload()
{
    DccTransferSend* transfer = new DccTransferSend();
    m_sendItems.push_back( transfer );
    connect( transfer, SIGNAL( done( DccTransfer* ) ), this, SLOT( removeSendItem( DccTransfer* ) ) );
    initTransfer( transfer );
    return transfer;
}

void DccTransferManager::initTransfer( DccTransfer* transfer )
{
    connect( transfer, SIGNAL( statusChanged( DccTransfer*, int, int ) ), this, SLOT( slotTransferStatusChanged( DccTransfer*, int, int ) ) );

    emit newTransferAdded( transfer );
}

DccTransferSend* DccTransferManager::findStandbySendItemByFileName( const QString& fileName, bool resumedItemOnly ) const
{
    QValueListConstIterator< DccTransferSend* > it;
    for ( it = m_sendItems.begin() ; it != m_sendItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getFileName() == fileName &&
             ( !resumedItemOnly || (*it)->isResumed() ) )
        {
            return *it;
        }
    }
    return 0;
}

DccTransferRecv* DccTransferManager::findStandbyRecvItemByFileName( const QString& fileName, bool resumedItemOnly ) const
{
    QValueListConstIterator< DccTransferRecv* > it;
    for ( it = m_recvItems.begin() ; it != m_recvItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getFileName() == fileName &&
             ( !resumedItemOnly || (*it)->isResumed() ) )
        {
            return *it;
        }
    }
    return 0;
}

DccTransferSend* DccTransferManager::findStandbySendItemByOwnPort( const QString& ownPort, bool resumedItemOnly ) const
{
    QValueListConstIterator< DccTransferSend* > it;
    for ( it = m_sendItems.begin() ; it != m_sendItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getOwnPort() == ownPort &&
             ( !resumedItemOnly || (*it)->isResumed() ) )
        {
            return *it;
        }
    }
    return 0;
}

DccTransferRecv* DccTransferManager::findStandbyRecvItemByOwnPort( const QString& ownPort, bool resumedItemOnly ) const
{
    QValueListConstIterator< DccTransferRecv* > it;
    for ( it = m_recvItems.begin() ; it != m_recvItems.end() ; ++it )
    {
        if ( ( (*it)->getStatus() == DccTransfer::Queued || (*it)->getStatus() == DccTransfer::WaitingRemote ) &&
             (*it)->getOwnPort() == ownPort &&
             ( !resumedItemOnly || (*it)->isResumed() ) )
        {
            return *it;
        }
    }
    return 0;
}

bool DccTransferManager::isLocalFileInWritingProcess( const KURL& url ) const
{
    QValueListConstIterator< DccTransferRecv* > it;
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

int DccTransferManager::generatePassiveSendTokenNumber()
{
    return m_nextPassiveSendTokenNumber++;
}

void DccTransferManager::slotTransferStatusChanged( DccTransfer* item, int newStatus, int oldStatus )
{
    kdDebug() << "DccTransferManager::slotTransferStatusChanged(): " << oldStatus << " -> " << newStatus << " " << item->getFileName() << " (" << item->getType() << ")" << endl;
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

#include "dcctransfermanager.moc"
