// dcctransferrecv.cpp - receive a file on DCC protocol
// Copyright (C) 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qdir.h>

#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstreamsocket.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

#include "dccpanel.h"
#include "dccresumedialog.h"
#include "dcctransferrecv.h"
#include "konversationapplication.h"

/*
 *flow chart*

 DccTransferRecv()

 start()              : called from DccPanel or DccDetailDialog when user push the accept button
  | \
  | requestResume()   : called when user chooses to resume in DccResumeDialog. it emits the signal ResumeRequest() 
  |
  | startResume()     : called from Server[classname]
  | |
 connectToSender()
 
 connectionSuccess()  : called from recvSocket
 
*/

DccTransferRecv::DccTransferRecv( DccPanel* panel, const QString& partnerNick, const KURL& defaultFolderURL, const QString& fileName, unsigned long fileSize, const QString& partnerIp, const QString& partnerPort )
  : DccTransfer( panel, DccTransfer::Receive, partnerNick, fileName )
{
  
  QString sanitised_filename = QFileInfo(fileName).fileName();  //Just incase anyone tries to do anything nasty
  if(sanitised_filename.isEmpty()) sanitised_filename= "unnamed";
  kdDebug() << "DccTransferRecv::DccTransferRecv()" << endl
            << "DccTransferRecv::DccTransferRecv(): Partner=" << partnerNick << endl
            << "DccTransferRecv::DccTransferRecv(): File=" << fileName << endl
            << "DccTransferRecv::DccTransferRecv(): Sanitised Filename=" << sanitised_filename << endl
            << "DccTransferRecv::DccTransferRecv(): FileSize=" << fileSize << endl
            << "DccTransferRecv::DccTransferRecv(): Partner Address=" << partnerIp << ":" << partnerPort << endl;
  
  // set default path
  // Append folder with partner's name if wanted
  KURL saveToFileURL( defaultFolderURL );
  saveToFileURL.adjustPath( 1 );  // add a slash if there is none
  if( KonversationApplication::preferences.getDccCreateFolder() )
    saveToFileURL.addPath( partnerNick.lower() + "/" );
  
  // determine default filename
  // Append partner's name to file name if wanted
  if( KonversationApplication::preferences.getDccAddPartner() )
    saveToFileURL.addPath( partnerNick.lower() + "." + sanitised_filename );
  else
    saveToFileURL.addPath( sanitised_filename );
  
  setSaveToFileURL( saveToFileURL );
 
  kdDebug() << "DccTransferRecv::DccTransferRecv(): saving to: '" << saveToFileURL.prettyURL() << "'" << endl;
  
  m_fileSize = fileSize;
  
  m_partnerIp = partnerIp;
  m_partnerPort = partnerPort;
  
  m_writeCacheHandler = 0;
  
  m_recvSocket = 0;
  
  updateView();
  panel->selectMe( this );
}

DccTransferRecv::~DccTransferRecv()
{
  cleanUp();
}

void DccTransferRecv::start()  // public slot
{
  kdDebug() << "DccTransferRecv::start()" << endl;
  
  // check whether the file exists
  // if exists, ask user to rename/overwrite/abort
  
  // check whether the temporary file exists
  // if exists, ask user to resume/rename/overwrite/abort
  
  m_saveToFileExists = ( KIO::NetAccess::exists( m_fileURL, false, listView() ) );
  m_partialFileExists = ( KIO::NetAccess::exists( m_saveToTmpFileURL, false, listView() ) );
  
  
  if( m_saveToFileExists ) {
    KIO::UDSEntry saveToFileEntry;
    KIO::NetAccess::stat( m_fileURL, saveToFileEntry, listView() );
    KFileItem saveToFileInfo( saveToFileEntry, m_fileURL );
    m_saveToFileSize = saveToFileInfo.size();
  }

  if( m_partialFileExists ) {
    KIO::UDSEntry partialFileEntry;
    KIO::NetAccess::stat( m_saveToTmpFileURL, partialFileEntry, listView() );
    KFileItem partialFileInfo( partialFileEntry, m_saveToTmpFileURL );
    m_partialFileSize = partialFileInfo.size();
  }
  
  m_partialFileExists = m_partialFileExists && 
                        0 < m_partialFileSize &&
                        m_partialFileSize < m_fileSize;

  if( !m_saveToFileExists && m_partialFileExists && KonversationApplication::preferences.getDccAutoResume() )
    requestResume();
  else if( m_saveToFileExists || m_partialFileExists )
  {
    switch( DccResumeDialog::ask( this ) )
    {
      case DccResumeDialog::Overwrite:
      case DccResumeDialog::Rename:
        connectToSender();
        break;
      case DccResumeDialog::Resume:
        requestResume();
        break;
      case DccResumeDialog::Cancel:
      default:
        return;
    }
  }
  else
    connectToSender();
}

void DccTransferRecv::startResume( unsigned long position )  // public slot
{
  kdDebug() << "DccTransferRecv::startResume( position=" << position << " )" << endl;
  
  stopConnectionTimer();
  
  // TODO: we'd better check if _position is equals to the requested position (transferringPosition)
  m_transferringPosition = position;
  
  connectToSender();
}

void DccTransferRecv::abort()  // public slot
{
  kdDebug() << "DccTransferRecv::abort()" << endl;
  
  setStatus(Aborted);
  cleanUp();
  updateView();
}

void DccTransferRecv::cleanUp()
{
  kdDebug() << "DccTransferRecv::cleanUp()" << endl;
  
  stopConnectionTimer();
  stopAutoUpdateView();
  if( m_recvSocket )
  {
    m_recvSocket->close();
    m_recvSocket->deleteLater();
    m_recvSocket = 0;
  }
  if( m_writeCacheHandler )
  {
    m_writeCacheHandler->closeNow();
    m_writeCacheHandler->deleteLater();
    m_writeCacheHandler = 0;
  }
}

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  m_resumed = true;
  setStatus( WaitingRemote, i18n("Requesting to accept resuming") );
  
  // Rollback for Resume
  // disabled temporarily
  // hey, can we rallback a file via KIO?
  /*
  KIO::filesize_t rb = KonversationApplication::preferences.getDccRollback();
  if( m_fileSize < rb )
    m_transferringPosition = 0;
  else
    m_transferringPosition = m_partialFileSize - rb;
  */
  m_transferringPosition = m_partialFileSize;
  
  updateView();
  
  startConnectionTimer( 5 );
  
  emit resumeRequest( m_partnerNick, m_fileName, m_partnerPort, m_transferringPosition );
}

void DccTransferRecv::connectToSender()
{
  kdDebug() << "DccTransferRecv::connectToSender()" << endl;
  
  // prepare local KIO
  KIO::TransferJob* transferJob = KIO::put( m_fileURL, -1, !m_resumed ? m_saveToFileExists : false, m_resumed, false );
  
  m_writeCacheHandler = new DccTransferRecvWriteCacheHandler( transferJob );
  
  connect( m_writeCacheHandler, SIGNAL( done() ),          this, SLOT( writeDone() )          );
  connect( m_writeCacheHandler, SIGNAL( gotError( int ) ), this, SLOT( gotWriteError( int ) ) );
  
  // connect to sender
  
  setStatus( Connecting );
  updateView();
  
  m_recvSocket = new KNetwork::KStreamSocket( m_partnerIp, m_partnerPort );
  
  m_recvSocket->setBlocking( false );  // asynchronous mode
  m_recvSocket->setFamily( KNetwork::KResolver::InetFamily );
  m_recvSocket->setResolutionEnabled( false );
  m_recvSocket->setTimeout( 5000 );
  
  m_recvSocket->enableRead( false );
  m_recvSocket->enableWrite( false );
  
  connect( m_recvSocket, SIGNAL( connected(const KResolverEntry&) ), this, SLOT( connectionSuccess() )   );
  connect( m_recvSocket, SIGNAL( gotError(int) ),                    this, SLOT( connectionFailed(int) ) );
  
  connect( m_recvSocket, SIGNAL( readyRead() ),  this, SLOT( readData() ) );
  connect( m_recvSocket, SIGNAL( readyWrite() ), this, SLOT( sendAck() )  );
  
  m_recvSocket->connect();
}

void DccTransferRecv::connectionSuccess()  // slot
{
  kdDebug() << "DccTransferRecv::connectionSuccess()" << endl;
  
  m_timeTransferStarted = QDateTime::currentDateTime();
  
  setStatus( Receiving );
  updateView();
  startAutoUpdateView();
  
  m_recvSocket->enableRead( true );
}

void DccTransferRecv::connectionFailed( int errorCode )  // slot
{
  kdDebug() << "DccTransferRecv::connectionFailed(): code = " << errorCode << endl;
  kdDebug() << "DccTransferRecv::connectionFailed(): string = " << m_recvSocket->errorString() << endl;
  
  setStatus( Failed );
  cleanUp();
  updateView();
}

void DccTransferRecv::readData()  // slot
{
  int actual = m_recvSocket->readBlock( m_buffer, m_bufferSize );
  if( actual > 0 )
  {
    m_transferringPosition += actual;
    QByteArray* ba = new QByteArray;
    ba->duplicate( m_buffer, actual );
    m_writeCacheHandler->append( ba );
    m_writeCacheHandler->write();
    m_recvSocket->enableWrite( true );
  }
}

void DccTransferRecv::sendAck()  // slot
{
  KIO::fileoffset_t pos = intel( m_transferringPosition );

  m_recvSocket->enableWrite( false );
  m_recvSocket->writeBlock( (char*)&pos, 4 );

  if( m_transferringPosition == m_fileSize )
  {
    kdDebug() << "DccTransferRecv::sendAck(): done." << endl;
    m_writeCacheHandler->close();
  }
}

void DccTransferRecv::writeDone()  // slot
{
  cleanUp();
  setStatus( Done );
  updateView();
  emit done( m_fileName );
}

void DccTransferRecv::gotWriteError( int errorCode )  // slot
{
  KMessageBox::sorry( 0, i18n("KIO Error. code %1").arg( QString::number( errorCode ) ),i18n("DCC Error") );
  setStatus( Failed, i18n("KIO Error. code %1").arg( QString::number( errorCode ) ) );
  cleanUp();
  updateView();
}

void DccTransferRecv::setSaveToFileURL( const KURL& url )
{
  m_fileURL = url;  //Parent's class url
  m_saveToTmpFileURL = KURL( url.url() + ".part" );
}

void DccTransferRecv::startConnectionTimer( int sec )
{
  stopConnectionTimer();
  connect( &m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
  m_connectionTimer.start( sec*1000, TRUE );
}

void DccTransferRecv::stopConnectionTimer()
{
  m_connectionTimer.stop();
}

void DccTransferRecv::connectionTimeout()  // slot
{
  kdDebug() << "DccTransferRecv::connectionTimeout()" << endl;
  
  setStatus(Failed, i18n("Timed out"));
  updateView();
  cleanUp();
}

// WriteCacheHandler

DccTransferRecvWriteCacheHandler::DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob )
  : m_transferJob( transferJob )
{
  m_writeReady = false;
  
  connect( this,          SIGNAL( dataFinished() ),                    m_transferJob, SLOT( slotFinished() )                           );
  connect( m_transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ), this,          SLOT( slotKIODataReq( KIO::Job*, QByteArray& ) ) );
  connect( m_transferJob, SIGNAL( result( KIO::Job* ) ),               this,          SLOT( slotKIOResult() )                          );
  
  m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = true );
}

DccTransferRecvWriteCacheHandler::~DccTransferRecvWriteCacheHandler()
{
  closeNow();
}

void DccTransferRecvWriteCacheHandler::append( QByteArray* cache )  // public
{
  m_cacheList.append( cache );
}

bool DccTransferRecvWriteCacheHandler::write( bool force )  // public
{
  // force == false: return without doing anything when the whole cache size is less than minWritePacketSize
  static const unsigned int minWritePacketSize = 1 * 1024 * 1024;  // 1meg
  
  if ( m_cacheList.isEmpty() || !m_transferJob || !m_writeReady || !m_writeAsyncMode )
    return false;
  
  if ( !force && allCacheSize() < minWritePacketSize )
    return false;
  
  // do write
  m_writeReady = false;
  QByteArray* cache = popCache();
  m_transferJob->sendAsyncData( *cache );
  delete cache;  //FIXME double check that it's okay to do this
  return true;
}

void DccTransferRecvWriteCacheHandler::close()  // public
{
  kdDebug() << "DccTransferRecvWriteCacheHandler::close()" << endl;
  write( true );  // write once if kio is ready to write
  m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = false );
  kdDebug() << "DccTransferRecvWriteCacheHandler::close(): switched to synchronized mode." << endl;
  kdDebug() << "DccTransferRecvWriteCacheHandler::close(): flushing... (remaining caches: " << m_cacheList.count() << ")" << endl;
}

void DccTransferRecvWriteCacheHandler::closeNow()  // public
{
  if ( m_transferJob )
  {
    emit dataFinished();  // tell KIO to close file
    m_transferJob = 0;
  }
  m_cacheList.setAutoDelete( true );
  while ( m_cacheList.removeFirst() );
}

unsigned long DccTransferRecvWriteCacheHandler::allCacheSize()
{
  QPtrListIterator<QByteArray> it( m_cacheList );
  unsigned long sizeSum = 0;
  while ( it.current() )
  {
    sizeSum += it.current()->size();
    ++it;
  }
  return sizeSum;
}

QByteArray* DccTransferRecvWriteCacheHandler::popCache()
{
  // sendAsyncData() and dataReq() cost a lot of time, so we should pack some caches.
  
  static const unsigned int maxWritePacketSize = 2 * 1024 * 1024;  // 2megs
  
  if ( !m_cacheList.isEmpty() )
  {
    // determine how many caches will be included in the packet to write
    QPtrListIterator<QByteArray> it( m_cacheList );
    unsigned int sizeSum = 0;
    int numCacheToWrite = 0;
    QByteArray* one;
    while ( it.current() )
    {
      one = it.current();
      if ( maxWritePacketSize < sizeSum + one->size() )
        break;
      sizeSum += one->size();
      ++numCacheToWrite;
      ++it;
    }
    
    // generate a packet to write
    QByteArray* cache = new QByteArray( sizeSum );
    it.toFirst();
    char* data = cache->data();
    for ( int i=0 ; i < numCacheToWrite ; ++i )
    {
      one = it.current();
      ++it;
      memcpy( data, one->data(), one->size() );
      data += one->size();
      m_cacheList.removeFirst();
      delete one; 
    }
    
    kdDebug() << "DccTransferRecvWriteCacheHandler::popCache(): caches in the packet: " << numCacheToWrite << ", remaining caches: " << m_cacheList.count() << endl;
    
    return cache;
  }
  return 0;
}

void DccTransferRecvWriteCacheHandler::slotKIODataReq( KIO::Job*, QByteArray& data )
{
  //We are in writeAsyncMode if there is more data to be read in from dcc
  if ( m_writeAsyncMode )
    m_writeReady = true;  
  else
  {
    //No more data left to read from incomming dcctransfer
    if ( !m_cacheList.isEmpty() )
      data = *( popCache() );  //once we write everything in cache, the file is complete.
                               //This function will be called once more after this last data is written.
                               //FIXME: should I delete the instance from popCache() here?
    else
    {
      //finally, no data left to write or read.
      kdDebug() << "DccTransferRecvWriteCacheHandler::slotKIODataReq(): flushing done." << endl;
      m_transferJob = 0;
      emit done();  // ->DccTransferRecv
    }
  }
}

void DccTransferRecvWriteCacheHandler::slotKIOResult()
{
  if( m_transferJob->error() )
  {
    m_transferJob->showErrorDialog(0);
    emit gotError( m_transferJob->error() );
    m_transferJob = 0;
  }
}

#include "dcctransferrecv.moc"
