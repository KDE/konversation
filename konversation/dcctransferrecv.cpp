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
  m_connectionTimer = new QTimer(this);
  connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
  //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted
 
  kdDebug() << "DccTransferRecv::DccTransferRecv()" << endl
            << "DccTransferRecv::DccTransferRecv(): Partner=" << partnerNick << endl
            << "DccTransferRecv::DccTransferRecv(): File=" << fileName << endl
            << "DccTransferRecv::DccTransferRecv(): Sanitised Filename=" << m_fileName << endl
            << "DccTransferRecv::DccTransferRecv(): FileSize=" << fileSize << endl
            << "DccTransferRecv::DccTransferRecv(): Partner Address=" << partnerIp << ":" << partnerPort << endl;
    
  m_fileSize = fileSize;
  m_defaultFolderURL = defaultFolderURL;
  m_partnerIp = partnerIp;
  m_partnerPort = partnerPort;
  
  m_writeCacheHandler = 0;
  
  m_recvSocket = 0;
  //The below function may not be valid, and may be empty.  Not checked until this user selects accept, and start() is called
  calculateSaveToFileURL(m_defaultFolderURL);
  updateView();
  panel->selectMe( this );

}

DccTransferRecv::~DccTransferRecv()
{
  cleanUp();
}

void DccTransferRecv::calculateSaveToFileURL( const KURL& folderURL )
{
  if( folderURL.isEmpty() || !folderURL.isValid() )
    return;  // don't do anything - we'll prompt the user about this in start()
  
  KURL saveToFileURL = folderURL;
  // set default path
  // Append folder with partner's name if wanted
  saveToFileURL.adjustPath( 1 );  // add a slash if there is none
  if( KonversationApplication::preferences.getDccCreateFolder() )
    saveToFileURL.addPath( m_partnerNick.lower() + "/" );
  
  // determine default filename
  // Append partner's name to file name if wanted
  if( KonversationApplication::preferences.getDccAddPartner() )
    saveToFileURL.addPath( m_partnerNick.lower() + "." + m_fileName );
  else
    saveToFileURL.addPath( m_fileName );
  
  setSaveToFileURL( saveToFileURL );
 
  kdDebug() << "DccTransferRecv::calculateSaveToFileURL(): saving to: '" << saveToFileURL.prettyURL() << "'" << endl;
}

bool DccTransferRecv::validateSaveToFileURL() {
  KURL saveToFileURL;
  if(m_defaultFolderURL.isEmpty() || !m_defaultFolderURL.isValid()) {
    saveToFileURL = KDirSelectDialog::selectDirectory(QString::null, false, listView(), "Select directory to save to");
    if(saveToFileURL.isEmpty())
      return false;
    calculateSaveToFileURL( saveToFileURL );
  }
  return true;
}

bool DccTransferRecv::createDirs( const KURL& dirURL ) const
{
  KURL kurl(dirURL);
  QString surl = kurl.url();
        
  //First we split directories until we reach to the top,
  //since we need to create directories one by one
        
  QStringList dirList;
  while (surl!=kurl.upURL().url())
  {
    dirList.prepend(surl);
    kurl=kurl.upURL();
    surl=kurl.url();
  }       

  //Now we create the directories

  QStringList::Iterator it;
  for ( it=dirList.begin() ; it!=dirList.end() ; ++it )
    if ( !KIO::NetAccess::exists( *it, true, listView() ) )
      if( !KIO::NetAccess::mkdir( *it, listView(), -1 ) )
      {
        KMessageBox::error(listView(), i18n("To save the sent file, the folder %1 needed to be created.  This failed, probably because you do not have permission to write there.  You can set where sent files are saved to by default in the Settings menu, under Configure Konversation, and choosing the Behavior->dcc tab.").arg(dirURL.prettyURL()), i18n("Failed to create folder"));
        return false;
      }
  return true;
}

void DccTransferRecv::start()  // public slot
{
  kdDebug() << "DccTransferRecv::start()" << endl;
  if( m_dccStatus != Queued )
    return;
  if( !validateSaveToFileURL() )  //Check that we are saving it somewhere valid, and set up the directories.
    return;
  
  // check whether the file exists
  // if exists, ask user to rename/overwrite/abort
  
  // check whether the temporary file exists
  // if exists, ask user to resume/rename/overwrite/abort
  
  setStatus( Queued, i18n("Checking local files...") );
  
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
  
  if( !m_saveToFileExists && !m_partialFileExists )
    if( !createDirs( m_fileURL.upURL() ) )
    {
      setStatus( Queued );  // just for removing the detail message
      return;
    }
  
  m_partialFileExists = m_partialFileExists && 
                        0 < m_partialFileSize &&
                        m_partialFileSize < m_fileSize;
  
  kdDebug() << "DccTransferRecv::start(): the completed file " << ( m_saveToFileExists ? QString("exists.") : QString("does NOT exist.") ) << endl;
  kdDebug() << "DccTransferRecv::start(): the partial file " << ( m_partialFileExists ? QString("exists.") : QString("does NOT exist.") ) << endl;
                        
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
        abort();
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

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  m_resumed = true;
  
  setStatus( WaitingRemote, i18n("waiting") );
  
  // Rollback for Resume
  // disabled temporarily
  // hey, can we rollback a file via KIO?
  /*
  KIO::filesize_t rb = KonversationApplication::preferences.getDccRollback();
  if( m_fileSize < rb )
    m_transferringPosition = 0;
  else
    m_transferringPosition = m_partialFileSize - rb;
  */
  m_transferringPosition = m_partialFileSize;
  
  updateView();
  
  startConnectionTimer( 30 ); //Was only 5 seconds?
  // <shin> john: because senders don't need to "accept" transfer. but, yes, it was too short.
  kdDebug() <<" requesting resume for " << m_partnerNick << " file " << m_fileName << " partner " << m_partnerPort << endl;

  //TODO   m_filename could have been sanitized - will this effect this?
  emit resumeRequest( m_partnerNick, m_fileName, m_partnerPort, m_transferringPosition );
}

void DccTransferRecv::connectToSender()
{
  kdDebug() << "DccTransferRecv::connectToSender()" << endl;
  
  // prepare local KIO
  KIO::TransferJob* transferJob = KIO::put( m_fileURL, -1, !m_resumed ? m_saveToFileExists : false, m_resumed, false );
  if(!transferJob) {
    kdDebug() << "KIO::put failed!" << endl;
    setStatus( Failed, i18n("KIO error") );
    updateView();
    cleanUp();
    return;
  }
  connect( transferJob, SIGNAL( canResume( KIO::Job*, KIO::filesize_t ) ), this, SLOT( slotCanResume( KIO::Job*, KIO::filesize_t ) ) );
  
  m_writeCacheHandler = new DccTransferRecvWriteCacheHandler( transferJob );
  
  connect( m_writeCacheHandler, SIGNAL( done() ),                     this, SLOT( writeDone() )                     );
  connect( m_writeCacheHandler, SIGNAL( gotError( const QString& ) ), this, SLOT( gotWriteError( const QString& ) ) );
  
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
  kdDebug() << "In connectToSender - attempting to connect to " << m_partnerIp << ":" << m_partnerPort << endl;
  if(!m_recvSocket->connect()) {
    kdDebug() << "connect failed immediately!! - " << m_recvSocket->errorString() << endl;
    abort();
    return;
  }
}

void DccTransferRecv::connectionSuccess()  // slot
{
  kdDebug() << "DccTransferRecv::connectionSuccess()" << endl;
  
  setStatus( Receiving );
  updateView();
  
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
    if(!m_writeCacheHandler->write()) {
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

void DccTransferRecv::writeDone()  // slot
{
  kdDebug() << "DccTransferRecv::writeDone()" << endl;
  setStatus( Done );
  updateView();
  cleanUp();
  emit done( m_fileName );
}

void DccTransferRecv::gotWriteError( const QString& errorString )  // slot
{
  Q_ASSERT(m_recvSocket); if(!m_recvSocket) return;
  
  KMessageBox::sorry( listView(), i18n("KIO Error: %1").arg( errorString ), i18n("DCC Error") );
  setStatus( Failed, i18n("KIO Error: %1").arg( errorString ) );
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
  SHOW;
  m_connectionTimer->start( sec*1000, TRUE );
}

void DccTransferRecv::stopConnectionTimer()
{
  SHOW;
  Q_ASSERT( m_connectionTimer );
  m_connectionTimer->stop();
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

void DccTransferRecv::slotCanResume( KIO::Job* /* job */, KIO::filesize_t size )
{
  kdDebug() << "DccTransferRecv::slotCanResume(): size = " << QString::number(size) << endl;
}

// WriteCacheHandler

DccTransferRecvWriteCacheHandler::DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob )
  : m_transferJob( transferJob )
{
  m_writeReady = false;
  m_wholeCacheSize = 0;
  
  connect( this,          SIGNAL( dataFinished() ),                    m_transferJob, SLOT( slotFinished() )                           );
  connect( m_transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ), this,          SLOT( slotKIODataReq( KIO::Job*, QByteArray& ) ) );
  connect( m_transferJob, SIGNAL( result( KIO::Job* ) ),               this,          SLOT( slotKIOResult() )                          );
  
  m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = true );
}

DccTransferRecvWriteCacheHandler::~DccTransferRecvWriteCacheHandler()
{
  closeNow();
}

void DccTransferRecvWriteCacheHandler::append( QByteArray cache )  // public
{
  m_cacheList.append( cache );
  m_wholeCacheSize += cache.size();
}

bool DccTransferRecvWriteCacheHandler::write( bool force )  // public
{
  // force == false: return without doing anything when the whole cache size is less than minWritePacketSize
  static const unsigned int minWritePacketSize = 1 * 1024 * 1024;  // 1meg
  
  if ( m_cacheList.isEmpty() || !m_transferJob || !m_writeReady || !m_writeAsyncMode )
    return false;
  
  if ( !force && m_wholeCacheSize < minWritePacketSize )
    return false;
  
  // do write

  kdDebug() << "cachehandler::write(" << force << ")" << endl;
  m_writeReady = false;
  QByteArray cache = popCache();
  m_transferJob->sendAsyncData( cache );
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
}

QByteArray DccTransferRecvWriteCacheHandler::popCache()
{
  // sendAsyncData() and dataReq() cost a lot of time, so we should pack some caches.
  
  static const unsigned int maxWritePacketSize = 2 * 1024 * 1024;  // 2megs
  
  QByteArray buffer;
  int number_written = 0; //purely for debug info
  int sizeSum = 0;
  if ( !m_cacheList.isEmpty() )
  {
    QDataStream out( buffer, IO_WriteOnly );
    QValueList<QByteArray>::iterator it = m_cacheList.begin();
    do { //It is guaranteed that at least one bytearray is written since m_cacheList is not empty
      Q_ASSERT((*it).size() > 0);
      out.writeRawBytes( (*it).data(), (*it).size() );
      sizeSum += (*it).size();
      it = m_cacheList.remove( it );
      number_written++; //for debug info
    } while( it != m_cacheList.end() && maxWritePacketSize >= sizeSum + (*it).size() );
    m_wholeCacheSize -= sizeSum;
  }
  kdDebug() << "DTRWriteCacheHandler::popCache(): caches in the packet: " << number_written << ", remaining caches: " << m_cacheList.count() << ".  Size just written now: " << sizeSum << endl;
  static unsigned long allPoppedSize = 0;
  allPoppedSize += buffer.size();
  kdDebug() << "DTRWriteCacheHandler::popCache(): all popped size: " << allPoppedSize << endl;
  return buffer;
}

void DccTransferRecvWriteCacheHandler::slotKIODataReq( KIO::Job*, QByteArray& data )
{
  //We are in writeAsyncMode if there is more data to be read in from dcc
  if ( m_writeAsyncMode ) {
    m_writeReady = true;  
    kdDebug()<< "slotKIODataReq - in async mode" << endl;
  }
  else
  {
    //No more data left to read from incomming dcctransfer
    if ( !m_cacheList.isEmpty() )
    {
      //once we write everything in cache, the file is complete.
      //This function will be called once more after this last data is written.
      kdDebug() << "slotKIODataReq - sending data." << endl;
      data = popCache();
    }
    else
    {
      //finally, no data left to write or read.
      kdDebug() << "DTRWriteCacheHandler::slotKIODataReq(): flushing done." << endl;
      Q_ASSERT( m_wholeCacheSize == 0 );
      m_transferJob = 0;
      emit done();  // ->DccTransferRecv
    }
  }
}

void DccTransferRecvWriteCacheHandler::slotKIOResult()
{
  Q_ASSERT(m_transferJob);
  if( m_transferJob->error() )
  {
    QString errorString = m_transferJob->errorString();
    closeNow();
    emit gotError( errorString );
  }
}

#include "dcctransferrecv.moc"

