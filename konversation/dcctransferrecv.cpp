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

 start()              : called from DccPanel or DccDetailDialog when user pushes the accept button
  | \
  | requestResume()   : called when user chooses to resume in DccResumeDialog. it emits the signal ResumeRequest() 
  |
  | startResume()     : called from "Server"
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
  
  m_recvSocket = 0;
  m_writeCacheHandler = 0;
  
  m_fileSize = fileSize;
  m_partnerIp = partnerIp;
  m_partnerPort = partnerPort;
  
  m_connectionTimer = new QTimer( this );
  connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
  //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted
  
  // determine default incoming file URL
  calculateSaveToFileURL( defaultFolderURL );
  
  setStatus( Queued );
  updateView();
  
  panel->selectMe( this );
  
  // TODO: should we support it?
  if ( fileSize == 0 )
  {
    failed( i18n( "Unsupported negotiation (filesize=0)" ) );
    return;
  }
  
  kdDebug() << "DccTransferRecv::DccTransferRecv() [END]" << endl;
}

void DccTransferRecv::calculateSaveToFileURL( const KURL& defaultFolderURL )
{
  // set default folder
  if ( !defaultFolderURL.isEmpty() )
    m_fileURL = defaultFolderURL;
  else
    m_fileURL.setPath( KUser( KUser::UseRealUserID ).homeDir() );  // default folder is *not* specified
  // add a slash if there is none
  m_fileURL.adjustPath( 1 );
  // Append folder with partner's name if wanted
  if ( KonversationApplication::preferences.getDccCreateFolder() )
    m_fileURL.addPath( m_partnerNick.lower() + "/" );
  
  // set default filename
  // Append partner's name to file name if wanted
  if ( KonversationApplication::preferences.getDccAddPartner() )
    m_fileURL.addPath( m_partnerNick.lower() + "." + m_fileName );
  else
    m_fileURL.addPath( m_fileName );
  
  kdDebug() << "DccTransferRecv::calculateSaveToFileURL(): Default URL: " << m_fileURL.prettyURL() << endl;
}

DccTransferRecv::~DccTransferRecv()
{
  cleanUp();
}

void DccTransferRecv::cleanUp()
{
  kdDebug() << "DccTransferRecv::cleanUp()" << endl;
  
  stopConnectionTimer();
  finishTransferMeter();
  if ( m_recvSocket )
  {
    m_recvSocket->close();
    m_recvSocket = 0;  // the instance will be deleted automatically by its parent
  }
  if ( m_writeCacheHandler )
  {
    m_writeCacheHandler->closeNow();
    m_writeCacheHandler->deleteLater();
    m_writeCacheHandler = 0;
  }
}

// just for convenience
void DccTransferRecv::failed( const QString& errorMessage )
{
  // don't show the dialog if user didn't take action on this DCC
  bool bNeedToOpenDetail = ( m_dccStatus != Queued );
  
  setStatus( Failed, errorMessage );
  updateView();
  cleanUp();
  emit done( m_fileURL.path(), Failed, errorMessage );
  
  if ( bNeedToOpenDetail )
    openDetailDialog();
}

void DccTransferRecv::abort()  // public slot
{
  kdDebug() << "DccTransferRecv::abort()" << endl;
  
  setStatus( Aborted );
  updateView();
  cleanUp();
  emit done( m_fileURL.path(), Aborted );
}

void DccTransferRecv::start()  // public slot
{
  kdDebug() << "DccTransferRecv::start() [BEGIN]" << endl;
  
  if ( m_dccStatus != Queued )
    return;
  
  setStatus( Preparing );
  
  prepareLocalKio( false, false );
  
  kdDebug() << "DccTransferRecv::start() [END]" << endl;
}

void DccTransferRecv::prepareLocalKio( bool overwrite, bool resume, KIO::fileoffset_t startPosition /* = 0 */ )
{
  kdDebug() << "DccTransferRecv::prepareLocalKio()" << endl
            << "DccTransferRecv::prepareLocalKio(): URL: " << m_fileURL << endl
            << "DccTransferRecv::prepareLocalKio(): Overwrite: " << overwrite << endl
            << "DccTransferRecv::prepareLocalKio(): Resume: " << resume << " (Position: " << QString::number( startPosition ) << ")" << endl;
  
  m_resumed = resume;
  m_transferringPosition = startPosition;
  
  if ( !createDirs( m_fileURL.upURL() ) )
  {
    askAndPrepareLocalKio( i18n( "<b>Cannot create the folder.</b><br>"
                                 "Folder: %1<br>" )
                             .arg( m_fileURL.upURL().prettyURL() ),
                           DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                           DccResumeDialog::RA_Rename );
    return;
  }
  
  KIO::TransferJob* transferJob = KIO::put( m_fileURL, -1, overwrite, m_resumed, false );
  
  if ( !transferJob )
  {
    kdDebug() << "DccTransferRecv::prepareLocalKio(): KIO::put() returned NULL. what happened?" << endl;
    failed( i18n( "Could not create a KIO instance" ) );
    return;
  }
  
  connect( transferJob, SIGNAL( canResume( KIO::Job*, KIO::filesize_t ) ), this, SLOT( slotLocalCanResume( KIO::Job*, KIO::filesize_t ) ) );
  connect( transferJob, SIGNAL( result( KIO::Job* ) ),                     this, SLOT( slotLocalGotResult( KIO::Job* ) ) );
  connect( transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ),       this, SLOT( slotLocalReady( KIO::Job* ) ) );
}

void DccTransferRecv::askAndPrepareLocalKio( const QString& message, int enabledActions, DccResumeDialog::ReceiveAction defaultAction, KIO::fileoffset_t startPosition )
{
  switch ( DccResumeDialog::ask( this, message, enabledActions, defaultAction ) )
  {
    case DccResumeDialog::RA_Resume:
      prepareLocalKio( false, true, startPosition );
      break;
    case DccResumeDialog::RA_Overwrite:
      prepareLocalKio( true, false );
      break;
    case DccResumeDialog::RA_Rename:
      prepareLocalKio( false, false );
      break;
    case DccResumeDialog::RA_Cancel:
    default:
      setStatus( Queued );
      updateView();
  }
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
      if ( !KIO::NetAccess::mkdir( *it, listView(), -1 ) )
        return false;
  
  return true;
}

void DccTransferRecv::slotLocalCanResume( KIO::Job* job, KIO::filesize_t size )
{
  kdDebug() << "DccTransferRecv::slotLocalCanResume() [BEGIN]" << endl
            << "DccTransferRecv::slotLocalCanResume(): size: " << QString::number( size ) << endl;
  
  if ( size != 0 )
  {
    KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );
    
    disconnect( transferJob, 0, 0, 0 );
    transferJob->kill();
    
    if ( KonversationApplication::preferences.getDccAutoResume() )
      prepareLocalKio( false, true, size );
    else
      askAndPrepareLocalKio( i18n( "<b>A partial file is existing.</b><br>"
                                   "%1<br>"
                                   "Size of the partial file: %2 bytes<br>" )
                               .arg( m_fileURL.prettyURL() )
                               .arg( DccTransfer::getPrettyNumberText( QString::number( size ) ) ),
                             DccResumeDialog::RA_Resume | DccResumeDialog::RA_Overwrite | DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                             DccResumeDialog::RA_Resume,
                             size );
  }
  
  kdDebug() << "DccTransferRecv::slotLocalCanResume() [END]" << endl;
}

void DccTransferRecv::slotLocalGotResult( KIO::Job* job )
{
  kdDebug() << "DccTransferRecv::slotLocalGotResult() [BEGIN]" << endl;
  
  KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );
  disconnect( transferJob, 0, 0, 0 );
  
  switch ( transferJob->error() )
  {
    case 0:  // no error
      kdDebug() << "DccTransferRecv::slotLocalGotResult(): job->error() returned 0." << endl
                << "DccTransferRecv::slotLocalGotResult(): Why was I called in spite of no error?" << endl;
      break;
    case KIO::ERR_FILE_ALREADY_EXIST:
      askAndPrepareLocalKio( i18n( "<b>The file is already existing.</b><br>"
                                   "%1<br>" )
                               .arg( m_fileURL.prettyURL() ),
                             DccResumeDialog::RA_Overwrite | DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                             DccResumeDialog::RA_Overwrite );
      break;
    default:
      askAndPrepareLocalKio( i18n( "<b>Could not open the file.<br>"
                                   "Error: %1</b><br>"
                                   "%2<br>" )
                               .arg( transferJob->error() )
                               .arg( m_fileURL.prettyURL() ),
                              DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                              DccResumeDialog::RA_Rename );
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
  
  if ( !m_resumed )
    connectToSender();
  else
    requestResume();
}

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  setStatus( WaitingRemote, i18n( "Waiting for remote host's acceptance" ) );
  updateView();
  
  startConnectionTimer( 30 );
  
  kdDebug() << "DccTransferRecv::requestResume(): requesting resume for " << m_partnerNick << " file " << m_fileName << " partner " << m_partnerPort << endl;

  //TODO   m_filename could have been sanitized - will this effect this?
  emit resumeRequest( m_partnerNick, m_fileName, m_partnerPort, m_transferringPosition );
}

void DccTransferRecv::startResume( unsigned long position )  // public slot
{
  kdDebug() << "DccTransferRecv::startResume():  position: " << position << endl;
  
  stopConnectionTimer();
  
  if ( m_transferringPosition != position )
  {
    kdDebug() << "DccTransferRecv::startResume(): remote responsed an unexpected position" << endl
              << "DccTransferRecv::startResume(): expected: " << QString::number( m_transferringPosition ) << endl
              << "DccTransferRecv::startResume(): remote response: " << position << endl;
    failed( i18n( "Unexpected response from remote host" ) );
    return;
  }
  
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
  m_recvSocket->setTimeout( 30000 );
  
  m_recvSocket->enableRead( false );
  m_recvSocket->enableWrite( false );
  
  connect( m_recvSocket, SIGNAL( connected( const KResolverEntry& ) ), this, SLOT( connectionSuccess() )     );
  connect( m_recvSocket, SIGNAL( gotError( int ) ),                    this, SLOT( connectionFailed( int ) ) );
  connect( m_recvSocket, SIGNAL( closed() ),                           this, SLOT( slotSocketClosed() )      );
  connect( m_recvSocket, SIGNAL( readyRead() ),                        this, SLOT( readData() )              );
  connect( m_recvSocket, SIGNAL( readyWrite() ),                       this, SLOT( sendAck() )               );
  
  kdDebug() << "DccTransferRecv::connectToSender(): attempting to connect to " << m_partnerIp << ":" << m_partnerPort << endl;
  
  m_recvSocket->connect();
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
  failed( i18n( "Connection failure: %1" ).arg( m_recvSocket->errorString() ) );
}

void DccTransferRecv::readData()  // slot
{
  //kdDebug() << "readData()" << endl;
  int actual = m_recvSocket->readBlock( m_buffer, m_bufferSize );
  if ( actual > 0 )
  {
    //actual is the size we read in, and is guaranteed to be less than m_bufferSize
    m_transferringPosition += actual;
    m_writeCacheHandler->append( m_buffer, actual );
    m_writeCacheHandler->write( false );
    m_recvSocket->enableWrite( true );
  }
}

void DccTransferRecv::sendAck()  // slot
{
  //kdDebug() << "sendAck()" << endl;
  KIO::fileoffset_t pos = intel( m_transferringPosition );

  m_recvSocket->enableWrite( false );
  m_recvSocket->writeBlock( (char*)&pos, 4 );
  if ( m_transferringPosition == (KIO::fileoffset_t)m_fileSize )
  {
    kdDebug() << "DccTransferRecv::sendAck(): Sent final ACK." << endl;
    m_recvSocket->enableRead( false );
    m_writeCacheHandler->close();  // WriteCacheHandler will send the signal done()
  }
  else if ( m_transferringPosition > (KIO::fileoffset_t)m_fileSize )
  {
    kdDebug() << "DccTransferRecv::sendAck(): the remote host sent larger data than expected: " << QString::number( m_transferringPosition ) << endl;
    failed( i18n( "Transferring error" ) );
  }
}

void DccTransferRecv::slotLocalWriteDone()  // <-WriteCacheHandler::done()
{
  kdDebug() << "DccTransferRecv::slotLocalWriteDone()" << endl;
  setStatus( Done );
  updateView();
  cleanUp();
  emit done( m_fileName );
}

void DccTransferRecv::slotLocalGotWriteError( const QString& errorString )  // <- WriteCacheHandler::gotError()
{
  kdDebug() << "DccTransferRecv::slotLocalGotWriteError()" << endl;
  failed( i18n( "KIO error: %1" ).arg( errorString ) );
}

void DccTransferRecv::startConnectionTimer( int sec )
{
  stopConnectionTimer();
  kdDebug() << "DccTransferRecv::startConnectionTimer()" << endl;
  m_connectionTimer->start( sec*1000, TRUE );
}

void DccTransferRecv::stopConnectionTimer()
{
  if ( m_connectionTimer->isActive() )
  {
    m_connectionTimer->stop();
    kdDebug() << "DccTransferRecv::stopConnectionTimer(): stop" << endl;
  }
}

void DccTransferRecv::connectionTimeout()  // slot
{
  kdDebug() << "DccTransferRecv::connectionTimeout()" << endl;
  failed( i18n( "Timed out" ) );
}

void DccTransferRecv::slotSocketClosed()
{
  if ( m_dccStatus == Receiving )
    failed( i18n( "Remote user disconnected" ) );
}


// WriteCacheHandler

DccTransferRecvWriteCacheHandler::DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob )
  : m_transferJob( transferJob )
{
  m_writeReady = true;
  m_cacheStream = 0;
  
  connect( this,          SIGNAL( dataFinished() ),                    m_transferJob, SLOT( slotFinished() )                           );
  connect( m_transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ), this,          SLOT( slotKIODataReq( KIO::Job*, QByteArray& ) ) );
  connect( m_transferJob, SIGNAL( result( KIO::Job* ) ),               this,          SLOT( slotKIOResult( KIO::Job* ) )               );
  
  m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = true );
}

DccTransferRecvWriteCacheHandler::~DccTransferRecvWriteCacheHandler()
{
  closeNow();
}

void DccTransferRecvWriteCacheHandler::append( char* data, int size )  // public
{
  // sendAsyncData() and dataReq() cost a lot of time, so we should pack some caches.
  
  static const unsigned int maxWritePacketSize = 1 * 1024 * 1024;  // 1meg
  
  if ( m_cacheList.isEmpty() || m_cacheList.back().size() + size > maxWritePacketSize )
  {
    m_cacheList.append( QByteArray() );
    delete m_cacheStream;
    m_cacheStream = new QDataStream( m_cacheList.back(), IO_WriteOnly );
  }
  
  m_cacheStream->writeRawBytes( data, size );
}

bool DccTransferRecvWriteCacheHandler::write( bool force )  // public
{
  // force == false: return without doing anything when the whole cache size is less than maxWritePacketSize
  
  if ( m_cacheList.isEmpty() || !m_transferJob || !m_writeReady || !m_writeAsyncMode )
    return false;
  
  if ( !force && m_cacheList.count() < 2 )
    return false;
  
  // do write
  
  m_writeReady = false;
  m_transferJob->sendAsyncData( m_cacheList.front() );
  kdDebug() << "DTRWriteCacheHandler::write(): wrote " << m_cacheList.front().size() << " bytes." << endl;
  m_cacheList.pop_front();
  
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
  if ( m_writeAsyncMode )
    m_writeReady = true;  
  else
  {
    //No more data left to read from incomming dcctransfer
    if ( !m_cacheList.isEmpty() )
    {
      //once we write everything in cache, the file is complete.
      //This function will be called once more after this last data is written.
      data = m_cacheList.front();
      kdDebug() << "DccTransferRecvWriteCacheHandler::slotKIODataReq(): will write " << m_cacheList.front().size() << " bytes." << endl;
      m_cacheList.pop_front();
    }
    else
    {
      //finally, no data left to write or read.
      kdDebug() << "DTRWriteCacheHandler::slotKIODataReq(): flushing done." << endl;
      m_transferJob = 0;
      emit done();  // -> DccTransferRecv::slotLocalWriteDone()
    }
  }
}

void DccTransferRecvWriteCacheHandler::slotKIOResult( KIO::Job* job )
{
  Q_ASSERT( m_transferJob );
  
  disconnect( m_transferJob, 0, 0, 0 );
  m_transferJob = 0;
    
  if ( job->error() )
  {
    QString errorString = job->errorString();
    closeNow();
    emit gotError( errorString );  // -> DccTransferRecv::slotLocalGotWriteError()
  }
}

#include "dcctransferrecv.moc"
