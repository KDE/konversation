// dcctransferrecv.cpp - receive a file on DCC protocol
/*
  dcctransfer.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstreamsocket.h>
#include <kdirselectdialog.h> 
#include <kuser.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

#include "dccpanel.h"
#include "dccresumedialog.h"
#include "dcctransferrecv.h"
#include "konversationapplication.h"

#include "konvidebug.h"

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
  kdDebug() << "DccTransferRecv::DccTransferRecv() [BEGIN]" << endl
            << "DccTransferRecv::DccTransferRecv(): Partner: " << partnerNick << endl
            << "DccTransferRecv::DccTransferRecv(): File: " << fileName << endl
            << "DccTransferRecv::DccTransferRecv(): Sanitised Filename: " << m_fileName << endl
            << "DccTransferRecv::DccTransferRecv(): FileSize: " << fileSize << endl
            << "DccTransferRecv::DccTransferRecv(): Partner Address: " << partnerIp << ":" << partnerPort << endl;
    
  m_fileSize = fileSize;
  m_partnerIp = partnerIp;
  m_partnerPort = partnerPort;
  
  m_writeCacheHandler = 0;
  
  m_connectionTimer = new QTimer( this );
  connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
  //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted
  
  m_recvSocket = 0;
  
  // determine default incoming file URL
  calculateSaveToFileURL( defaultFolderURL );
  
  updateView();
  
  panel->selectMe( this );
  
  kdDebug() << "DccTransferRecv::DccTransferRecv() [END]" << endl;
}

void DccTransferRecv::calculateSaveToFileURL( const KURL& defaultFolderURL )
{
  // set default folder
  if( !defaultFolderURL.isEmpty() )
    m_fileURL = defaultFolderURL;
  else
    m_fileURL.setPath( KUser( KUser::UseRealUserID ).homeDir() );  // default folder is *not* specified
  m_fileURL.adjustPath( 1 );  // add a slash if there is none
  // Append folder with partner's name if wanted
  if( KonversationApplication::preferences.getDccCreateFolder() )
    m_fileURL.addPath( m_partnerNick.lower() + "/" );
  
  // set default filename
  // Append partner's name to file name if wanted
  if( KonversationApplication::preferences.getDccAddPartner() )
    m_fileURL.addPath( m_partnerNick.lower() + "." + m_fileName );
  else
    m_fileURL.addPath( m_fileName );
  
  kdDebug() << "DccTransferRecv::calculateSaveToFileURL(): Default URL: " << m_fileURL.prettyURL() << endl;
}

DccTransferRecv::~DccTransferRecv()
{
  cleanUp();
}

void DccTransferRecv::abort()  // public slot
{
  kdDebug() << "DccTransferRecv::abort()" << endl;
  
  setStatus( Aborted );
  cleanUp();
  updateView();
}

void DccTransferRecv::cleanUp()
{
  kdDebug() << "DccTransferRecv::cleanUp()" << endl;
  
  stopConnectionTimer();
  finishTransferMeter();
  if( m_recvSocket )
  {
    m_recvSocket->close();
    m_recvSocket = 0;  // the instance will be deleted automatically by its parent
  }
  if( m_writeCacheHandler )
  {
    m_writeCacheHandler->closeNow();
    m_writeCacheHandler->deleteLater();
    m_writeCacheHandler = 0;
  }
}

void DccTransferRecv::start()  // public slot
{
  kdDebug() << "DccTransferRecv::start() [BEGIN]" << endl;
  
  if( m_dccStatus != Queued )
    return;
  
  setStatus( Preparing );
  
  prepareLocalKio( false, 0 );
  
  kdDebug() << "DccTransferRecv::start() [END]" << endl;
}

void DccTransferRecv::prepareLocalKio( bool overwrite, KIO::fileoffset_t startPosition )
{
  kdDebug() << "DccTransferRecv::prepareLocalKio()" << endl
            << "DccTransferRecv::prepareLocalKio(): URL: " << m_fileURL << endl
            << "DccTransferRecv::prepareLocalKio(): Overwrite: " << overwrite << endl
            << "DccTransferRecv::prepareLocalKio(): Resume: " << ( startPosition != 0 ) << " (Position: " << QString::number( startPosition ) << ")" << endl;
  
  m_resumed = ( startPosition != 0 );
  m_transferringPosition = startPosition;
  
  if( !createDirs( m_fileURL.upURL() ) )
  {
    switch( DccResumeDialog::ask( this,
                                  i18n("<b>Cannot create folder</b><br>Folder %1").arg( m_fileURL.upURL().prettyURL() ),
                                  DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                                  DccResumeDialog::RA_Resume ) )
    {
      case DccResumeDialog::RA_Rename:
        prepareLocalKio( false, 0 );
      case DccResumeDialog::RA_Cancel:
      default:
        setStatus( Queued );
        updateView();
    }
    return;
  }
  
  KIO::TransferJob* transferJob = KIO::put( m_fileURL, -1, overwrite, m_resumed, false );
  
  if( !transferJob )
  {
    kdDebug() << "DccTransferRecv::prepareLocalKio(): KIO::put() returned NULL. what happened?" << endl;
    setStatus( Failed, i18n("KIO Error") );
    updateView();
    cleanUp();
    return;
  }
  
  connect( transferJob, SIGNAL( canResume( KIO::Job*, KIO::filesize_t ) ), this, SLOT( slotLocalCanResume( KIO::Job*, KIO::filesize_t ) ) );
  connect( transferJob, SIGNAL( result( KIO::Job* ) ),                     this, SLOT( slotLocalGotResult( KIO::Job* ) ) );
  connect( transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ),       this, SLOT( slotLocalReady( KIO::Job* ) ) );
}

bool DccTransferRecv::createDirs( const KURL& dirURL ) const
{
  KURL kurl( dirURL );
  QString surl = kurl.url();
  
  //First we split directories until we reach to the top,
  //since we need to create directories one by one
  
  QStringList dirList;
  while ( surl != kurl.upURL().url() )
  {
    dirList.prepend( surl );
    kurl = kurl.upURL();
    surl = kurl.url();
  }       
  
  //Now we create the directories
  
  QStringList::Iterator it;
  for ( it=dirList.begin() ; it!=dirList.end() ; ++it )
    if ( !KIO::NetAccess::exists( *it, true, listView() ) )
      if( !KIO::NetAccess::mkdir( *it, listView(), -1 ) )
        return false;
  
  return true;
}

void DccTransferRecv::slotLocalCanResume( KIO::Job* job, KIO::filesize_t size )
{
  kdDebug() << "DccTransferRecv::slotLocalCanResume() [BEGIN]" << endl
            << "DccTransferRecv::slotLocalCanResume(): size: " << QString::number( size ) << endl;
  
  if( size != 0 )
  {
    KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );
    
    if( KonversationApplication::preferences.getDccAutoResume() )
    {
      disconnect( transferJob, 0, 0, 0 );
      transferJob->kill();
      prepareLocalKio( false, size );
    }
    else
    {
      switch( DccResumeDialog::ask( this,
                                    i18n("<b>A partial file exists</b><br>file %1").arg( m_fileURL.prettyURL() ),
                                    DccResumeDialog::RA_Resume | DccResumeDialog::RA_Overwrite | DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                                    DccResumeDialog::RA_Resume ) )
      {
        case DccResumeDialog::RA_Resume:
          disconnect( transferJob, 0, 0, 0 );
          transferJob->kill();
          prepareLocalKio( false, size );
          break;
        case DccResumeDialog::RA_Overwrite:
          // throw canResume()
          break;
        case DccResumeDialog::RA_Rename:
          disconnect( transferJob, 0, 0, 0 );
          transferJob->kill();
          prepareLocalKio( false, 0 );
        case DccResumeDialog::RA_Cancel:
        default:
          disconnect( transferJob, 0, 0, 0 );
          transferJob->kill();
          setStatus( Queued );
          updateView();
      }
    }
  }
  
  kdDebug() << "DccTransferRecv::slotLocalCanResume() [END]" << endl;
}

void DccTransferRecv::slotLocalGotResult( KIO::Job* job )
{
  kdDebug() << "DccTransferRecv::slotLocalGotResult() [BEGIN]" << endl;
  
  KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );
  disconnect( transferJob, 0, 0, 0 );
  
  switch( transferJob->error() )
  {
    case 0:  // no error
      kdDebug() << "DccTransferRecv::slotLocalGotResult(): job->error() returned 0." << endl
                << "DccTransferRecv::slotLocalGotResult(): Why was I called in spite of no error?" << endl;
      break;
    case KIO::ERR_FILE_ALREADY_EXIST:
      switch( DccResumeDialog::ask( this,
                                  i18n("<b>The file already exists.</b><br>file %1").arg( m_fileURL.prettyURL() ),
                                  DccResumeDialog::RA_Overwrite | DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                                  DccResumeDialog::RA_Overwrite ) )
      {
        case DccResumeDialog::RA_Overwrite:
          prepareLocalKio( true, 0 );
          break;
        case DccResumeDialog::RA_Rename:
          prepareLocalKio( false, 0 );
        case DccResumeDialog::RA_Cancel:
        default:
          setStatus( Queued );
          updateView();
      }
      break;
    default:
      switch( DccResumeDialog::ask( this,
                                    i18n("<b>Cannot open the file</m><br>file %1<br>error %2")
                                      .arg( m_fileURL.prettyURL() )
                                      .arg( transferJob->error() ),
                                    DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                                    DccResumeDialog::RA_Rename ) )
      {
        case DccResumeDialog::RA_Rename:
          prepareLocalKio( false, 0 );
        case DccResumeDialog::RA_Cancel:
        default:
          setStatus( Queued );
          updateView();
      }
      break;
  }
  
  kdDebug() << "DccTransferRecv::slotLocalGotResult() [END]" << endl;
}

void DccTransferRecv::slotLocalReady( KIO::Job* job )
{
  kdDebug() << "DccTransferRecv::slotLocalReady()" << endl;
  
  KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );
  
  disconnect( transferJob, 0, 0, 0 );  // WriteCacheHandler will control the job after this
  
  m_writeCacheHandler = new DccTransferRecvWriteCacheHandler( transferJob );
  
  connect( m_writeCacheHandler, SIGNAL( done() ),                     this, SLOT( slotLocalWriteDone() )                     );
  connect( m_writeCacheHandler, SIGNAL( gotError( const QString& ) ), this, SLOT( slotLocalGotWriteError( const QString& ) ) );
  
  if( !m_resumed )
    connectToSender();
  else
    requestResume();
}

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  setStatus( WaitingRemote, i18n("Waiting for remote host's acceptance") );
  updateView();
  
  startConnectionTimer( 30 ); //Was only 5 seconds?
  // <shin> john: because senders don't need to "accept" transfer. but, yes, it was too short.
  kdDebug() << "DccTransferRecv::requestResume(): requesting resume for " << m_partnerNick << " file " << m_fileName << " partner " << m_partnerPort << endl;

  //TODO   m_filename could have been sanitized - will this effect this?
  emit resumeRequest( m_partnerNick, m_fileName, m_partnerPort, m_transferringPosition );
}

void DccTransferRecv::startResume( unsigned long position )  // public slot
{
  kdDebug() << "DccTransferRecv::startResume( position=" << position << " )" << endl;
  
  stopConnectionTimer();
  
  // FIXME: we should check if _position is equals to the requested position (transferringPosition)
  m_transferringPosition = position;
  
  connectToSender();
}

void DccTransferRecv::connectToSender()
{
  kdDebug() << "DccTransferRecv::connectToSender()" << endl;
  
  // connect to sender
  
  setStatus( Connecting );
  updateView();
  
  m_recvSocket = new KNetwork::KStreamSocket( m_partnerIp, m_partnerPort, this);
  
  m_recvSocket->setBlocking( false );  // asynchronous mode
  m_recvSocket->setFamily( KNetwork::KResolver::InetFamily );
  m_recvSocket->setResolutionEnabled( false );
  m_recvSocket->setTimeout( 50000 ); //Was only 5 secs.  Made 50 secs
  
  m_recvSocket->enableRead( false );
  m_recvSocket->enableWrite( false );
  
  connect( m_recvSocket, SIGNAL( connected( const KResolverEntry& ) ), this, SLOT( connectionSuccess() )     );
  connect( m_recvSocket, SIGNAL( gotError( int ) ),                    this, SLOT( connectionFailed( int ) ) );
  connect( m_recvSocket, SIGNAL( closed() ),                           this, SLOT( slotSocketClosed() )      );
  connect( m_recvSocket, SIGNAL( readyRead() ),                        this, SLOT( readData() )              );
  connect( m_recvSocket, SIGNAL( readyWrite() ),                       this, SLOT( sendAck() )               );
  
  kdDebug() << "DccTransferRecv::connectToSender(): attempting to connect to " << m_partnerIp << ":" << m_partnerPort << endl;
  
  if(!m_recvSocket->connect()) {
    kdDebug() << "DccTransferRecv::connectToSender(): connect failed immediately!! - " << m_recvSocket->errorString() << endl;
    abort();
    return;
  }
}

void DccTransferRecv::connectionSuccess()  // slot
{
  kdDebug() << "DccTransferRecv::connectionSuccess()" << endl;
  
  setStatus( Receiving );
  updateView();
  
  m_transferStartPosition = m_transferringPosition;
  
  m_recvSocket->enableRead( true );
  
  initTransferMeter();  // initialize CPS counter, ETA counter, etc...
}

void DccTransferRecv::connectionFailed( int errorCode )  // slot
{
  kdDebug() << "DccTransferRecv::connectionFailed(): code = " << errorCode << ", string = " << m_recvSocket->errorString() << endl;
  
  KMessageBox::sorry( listView(), i18n("Connection failure: %1").arg( m_recvSocket->errorString() ),i18n("DCC Error") );
  setStatus( Failed, i18n("Connection failure: %1").arg( m_recvSocket->errorString() ) );
  cleanUp();
  updateView();
}

void DccTransferRecv::readData()  // slot
{
  //kdDebug() << "readData()" << endl;
  int actual = m_recvSocket->readBlock( m_buffer, m_bufferSize );
  if( actual > 0 )
  {
    //actual is the size we read in, and is guaranteed to be less than m_bufferSize
    m_transferringPosition += actual;
    QByteArray ba;
    ba.duplicate( m_buffer, actual );
    m_writeCacheHandler->append( ba );
    if(!m_writeCacheHandler->write( false )) {
      //kdDebug() << "m_writeCacheHandler->write() failed in readData()" << endl;
    }
    m_recvSocket->enableWrite( true );
  }
}

void DccTransferRecv::sendAck()  // slot
{
  //kdDebug() << "sendAck()" << endl;
  KIO::fileoffset_t pos = intel( m_transferringPosition );

  m_recvSocket->enableWrite( false );
  m_recvSocket->writeBlock( (char*)&pos, 4 );
  if( m_transferringPosition == (KIO::fileoffset_t)m_fileSize )
  {
    kdDebug() << "DccTransferRecv::sendAck(): done." << endl;
    m_writeCacheHandler->close();
  }
}

void DccTransferRecv::slotLocalWriteDone()  // slot
{
  kdDebug() << "DccTransferRecv::slotLocalWriteDone()" << endl;
  setStatus( Done );
  updateView();
  cleanUp();
  emit done( m_fileName );
}

void DccTransferRecv::slotLocalGotWriteError( const QString& errorString )  // slot
{
  kdDebug() << "DccTransferRecv::slotLocalGotWriteError()" << endl;
  
  Q_ASSERT(m_recvSocket); if(!m_recvSocket) return;
  
  setStatus( Failed, i18n("KIO Error: %1").arg( errorString ) );
  cleanUp();
  updateView();
  
  KMessageBox::sorry( listView(), i18n("KIO Error: %1").arg( errorString ), i18n("DCC Error") );
}

void DccTransferRecv::startConnectionTimer( int sec )
{
  stopConnectionTimer();
  kdDebug() << "DccTransferRecv::startConnectionTimer()" << endl;
  m_connectionTimer->start( sec*1000, TRUE );
}

void DccTransferRecv::stopConnectionTimer()
{
  if( m_connectionTimer->isActive() )
  {
    m_connectionTimer->stop();
    kdDebug() << "DccTransferRecv::stopConnectionTimer(): stop" << endl;
  }
}

void DccTransferRecv::connectionTimeout()  // slot
{
  kdDebug() << "DccTransferRecv::connectionTimeout()" << endl;
  
  setStatus(Failed, i18n("Timed out"));
  updateView();
  cleanUp();
}

void DccTransferRecv::slotSocketClosed()
{
  if( m_dccStatus == Receiving )
  {
    setStatus( Failed, i18n("Remote user disconnected") );
    updateView();
    cleanUp();
  }
}


// WriteCacheHandler

DccTransferRecvWriteCacheHandler::DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob )
  : m_transferJob( transferJob )
{
  m_writeReady = true;
  m_cacheStream = 0;
  
  connect( this,          SIGNAL( dataFinished() ),                    m_transferJob, SLOT( slotFinished() )                           );
  connect( m_transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ), this,          SLOT( slotKIODataReq( KIO::Job*, QByteArray& ) ) );
  connect( m_transferJob, SIGNAL( result( KIO::Job* ) ),               this,          SLOT( slotKIOResult( KIO::Job* ) )                          );
  
  m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = true );
}

DccTransferRecvWriteCacheHandler::~DccTransferRecvWriteCacheHandler()
{
  closeNow();
}

void DccTransferRecvWriteCacheHandler::append( const QByteArray& cache )  // public
{
  // sendAsyncData() and dataReq() cost a lot of time, so we should pack some caches.
  
  static const unsigned int maxWritePacketSize = 2 * 1024 * 1024;  // 2megs
  
  if( m_cacheList.isEmpty() || m_cacheList.back().size() + cache.size() > maxWritePacketSize )
  {
    m_cacheList.append( QByteArray() );
    delete m_cacheStream;
    m_cacheStream = new QDataStream( m_cacheList.back(), IO_WriteOnly );
  }
  
  m_cacheStream->writeRawBytes( cache.data(), cache.size() );
}

bool DccTransferRecvWriteCacheHandler::write( bool force )  // public
{
  // force == false: return without doing anything when the whole cache size is less than minWritePacketSize
  static const unsigned int minWritePacketSize = 1 * 1024 * 1024;  // 1meg
  
  if( m_cacheList.isEmpty() || !m_transferJob || !m_writeReady || !m_writeAsyncMode )
    return false;
  
  if( !force && m_cacheList.front().size() < minWritePacketSize )
    return false;
  
  // do write
  
  m_writeReady = false;
  m_transferJob->sendAsyncData( m_cacheList.front() );
  unsigned int wroteSize = m_cacheList.front().size();
  m_cacheList.pop_front();
  
  kdDebug() << "DTRWriteCacheHandler::write(): wrote " << wroteSize << " bytes." << endl;
  
  return true;
}

void DccTransferRecvWriteCacheHandler::close()  // public
{
  kdDebug() << "DTRWriteCacheHandler::close()" << endl;
  write( true );  // write once if kio is ready to write
  m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = false );
  kdDebug() << "DTRWriteCacheHandler::close(): switched to synchronized mode." << endl;
  kdDebug() << "DTRWriteCacheHandler::close(): flushing... (remaining caches: " << m_cacheList.count() << ")" << endl;
}

void DccTransferRecvWriteCacheHandler::closeNow()  // public
{
  if ( m_transferJob )
  {
    m_transferJob->kill();
    m_transferJob = 0;
  }
  m_cacheList.clear();
  delete m_cacheStream;
  m_cacheStream = 0;
}

void DccTransferRecvWriteCacheHandler::slotKIODataReq( KIO::Job*, QByteArray& data )
{
  //We are in writeAsyncMode if there is more data to be read in from dcc
  if ( m_writeAsyncMode ) {
    m_writeReady = true;  
    kdDebug()<< "DTRWriteCacheHandler::slotKIODataReq(): in async mode" << endl;
  }
  else
  {
    //No more data left to read from incomming dcctransfer
    if ( !m_cacheList.isEmpty() )
    {
      //once we write everything in cache, the file is complete.
      //This function will be called once more after this last data is written.
      data = m_cacheList.front();
      m_cacheList.pop_front();
    }
    else
    {
      //finally, no data left to write or read.
      kdDebug() << "DTRWriteCacheHandler::slotKIODataReq(): flushing done." << endl;
      m_transferJob = 0;
      emit done();  // ->DccTransferRecv
    }
  }
}

void DccTransferRecvWriteCacheHandler::slotKIOResult( KIO::Job* job )
{
  Q_ASSERT( m_transferJob );
  
  disconnect( m_transferJob, 0, 0, 0 );
  m_transferJob = 0;
    
  if( job->error() )
  {
    QString errorString = job->errorString();
    closeNow();
    emit gotError( errorString );
  }
}

#include "dcctransferrecv.moc"
