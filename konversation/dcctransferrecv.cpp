/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

// dcctransferrecv.cpp : separated from dcctransfer.cpp
/*
  dcctransfer.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
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

DccTransferRecv::DccTransferRecv(KListView* _parent, const QString& _partnerNick, const KURL& _defaultFolderURL, const QString& _fileName, unsigned long _fileSize, const QString& _partnerIp, const QString& _partnerPort)
  : DccTransfer(_parent, DccTransfer::Receive, _partnerNick, _fileName)
{
  
  QString sanitised_filename = QFileInfo(_fileName).fileName();  //Just incase anyone tries to do anything nasty
  if(sanitised_filename.isEmpty()) sanitised_filename= "unnamed";
  kdDebug() << "DccTransferRecv::DccTransferRecv()" << endl
            << "DccTransferRecv::DccTransferRecv(): Partner=" << _partnerNick << endl
            << "DccTransferRecv::DccTransferRecv(): File=" << _fileName << endl
            << "DccTransferRecv::DccTransferRecv(): Sanitised Filename=" << sanitised_filename << endl
            << "DccTransferRecv::DccTransferRecv(): FileSize=" << _fileSize << endl
            << "DccTransferRecv::DccTransferRecv(): Partner Address=" << _partnerIp << ":" << _partnerPort << endl;
  
  // set default path
  // Append folder with partner's name if wanted
  KURL saveToFileURL(_defaultFolderURL);
  saveToFileURL.adjustPath(1);  // add a slash if there is none
  if(KonversationApplication::preferences.getDccCreateFolder())
    saveToFileURL.addPath(_partnerNick.lower()+"/");
  
  // determine default filename
  // Append partner's name to file name if wanted
  if(KonversationApplication::preferences.getDccAddPartner())
    saveToFileURL.addPath(_partnerNick.lower()+"."+sanitised_filename);
  else
    saveToFileURL.addPath(sanitised_filename);
  
  setSaveToFileURL(saveToFileURL);
 
  kdDebug() << "DccTransferRecv::DccTransferRecv(): saving to: '" << saveToFileURL.prettyURL() << "'" << endl;
  
  m_fileSize=_fileSize;
  
  partnerIp=_partnerIp;
  partnerPort=_partnerPort;
  
  m_writeCacheHandler = 0;
  
  m_recvSocket = 0;
  m_connectionTimer = 0;
  
  updateView();
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
  
  m_saveToFileExists = ( KIO::NetAccess::exists(m_fileURL, false, listView()));
  m_partialFileExists = ( KIO::NetAccess::exists(m_saveToTmpFileURL, false, listView()));
  
  
  if(m_saveToFileExists) {
    KIO::UDSEntry saveToFileEntry;
    KIO::NetAccess::stat(m_fileURL, saveToFileEntry, listView());
    KFileItem saveToFileInfo(saveToFileEntry, m_fileURL);
    m_saveToFileSize = saveToFileInfo.size();
  }

  if(m_partialFileExists) {
    KIO::UDSEntry partialFileEntry;
    KIO::NetAccess::stat(m_saveToTmpFileURL, partialFileEntry, listView());
    KFileItem partialFileInfo(partialFileEntry, m_saveToTmpFileURL);
    m_partialFileSize = partialFileInfo.size();
  }
  
  m_saveToFileExists = m_saveToFileExists && 
                       0 < m_saveToFileSize && 
                       m_saveToFileSize < m_fileSize;
  m_partialFileExists = m_partialFileExists && 
                        0 < m_partialFileSize;

  if(!m_saveToFileExists && m_partialFileExists && KonversationApplication::preferences.getDccAutoResume())
    requestResume();
  else if(m_saveToFileExists || m_partialFileExists)
  {
    switch(DccResumeDialog::ask(this))
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

void DccTransferRecv::startResume(unsigned long _position)  // public slot
{
  kdDebug() << "DccTransferRecv::startResume( position=" << _position << " )" << endl;
  
  stopConnectionTimer();
  
  // TODO: we'd better check if _position is equals to the requested position (transferringPosition)
  transferringPosition = _position;
  
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
  if(m_recvSocket)
  {
    m_recvSocket->close();
    m_recvSocket->deleteLater();
    m_recvSocket = 0;
  }
  if(m_writeCacheHandler)
  {
    m_writeCacheHandler->closeNow();
    m_writeCacheHandler->deleteLater();
    m_writeCacheHandler = 0;
  }
}

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  bResumed = true;
  setStatus(WaitingRemote, i18n("Requesting to accept resuming"));
  
  // Rollback for Resume
  KIO::filesize_t rb=KonversationApplication::preferences.getDccRollback();
  if(m_fileSize < rb)
    transferringPosition=0;
  else
    transferringPosition=m_fileSize-rb;
  
  updateView();
  
  startConnectionTimer(5);
  
  emit resumeRequest(partnerNick,fileName,partnerPort,transferringPosition);
  
}

void DccTransferRecv::connectToSender()
{
  kdDebug() << "DccTransferRecv::connectToSender()" << endl;
  
  // prepare local KIO
  KIO::TransferJob* transferJob = KIO::put(m_fileURL, -1, false, false, false);
  
  m_writeCacheHandler = new DccTransferRecvWriteCacheHandler(transferJob);
  
  connect( m_writeCacheHandler, SIGNAL( done() ),        this, SLOT( writeDone() ) );
  connect( m_writeCacheHandler, SIGNAL( gotError(int) ), this, SLOT( gotWriteError(int) ) );
  
  // connect to sender
  
  setStatus(Connecting);
  updateView();
  
  m_recvSocket=new KNetwork::KStreamSocket(partnerIp, partnerPort);
  
  m_recvSocket->setBlocking(false);  // asynchronous mode
  m_recvSocket->setFamily(KNetwork::KResolver::InetFamily);
  m_recvSocket->setResolutionEnabled(false);
  m_recvSocket->setTimeout(5000);
  
  m_recvSocket->enableRead(false);
  m_recvSocket->enableWrite(false);
  
  connect( m_recvSocket, SIGNAL( connected(const KResolverEntry&) ), this, SLOT( connectionSuccess() ) );
  connect( m_recvSocket, SIGNAL( gotError(int) ),                    this, SLOT( connectionFailed(int) ) );
  
  connect( m_recvSocket, SIGNAL( readyRead() ),  this, SLOT( readData() ) );
  connect( m_recvSocket, SIGNAL( readyWrite() ), this, SLOT( sendAck() ) );
  
  m_recvSocket->connect();
}

void DccTransferRecv::connectionSuccess()  // slot
{
  kdDebug() << "DccTransferRecv::connectionSuccess()" << endl;
  
  timeTransferStarted = QDateTime::currentDateTime();
  
  setStatus(Receiving);
  updateView();
  startAutoUpdateView();
  
  m_recvSocket->enableRead(true);
}

void DccTransferRecv::connectionFailed(int errorCode)  // slot
{
  kdDebug() << "DccTransferRecv::connectionFailed(): code = " << errorCode << endl;
  kdDebug() << "DccTransferRecv::connectionFailed(): string = " << m_recvSocket->errorString() << endl;
  
  setStatus(Failed);
  cleanUp();
  updateView();
}

void DccTransferRecv::readData()  // slot
{
  int actual=m_recvSocket->readBlock(buffer,bufferSize);
  if(actual>0)
  {
    transferringPosition += actual;
    QByteArray* ba = new QByteArray;
    ba->duplicate(buffer, actual);
    m_writeCacheHandler->append(ba);
    m_writeCacheHandler->write();
    m_recvSocket->enableWrite(true);
  }
}

void DccTransferRecv::sendAck()  // slot
{
  KIO::fileoffset_t pos=intel(transferringPosition);

  m_recvSocket->enableWrite(false);
  m_recvSocket->writeBlock((char*) &pos,4);

  if(transferringPosition==m_fileSize)
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
  emit done( fileName );
}

void DccTransferRecv::gotWriteError( int errorCode )  // slot
{
  KMessageBox::sorry( 0, i18n("KIO Error. code %1").arg( QString::number( errorCode ) ),i18n("DCC Error") );
  setStatus( Failed, i18n("KIO Error. code %1").arg( QString::number( errorCode ) ) );
  cleanUp();
  updateView();
}

void DccTransferRecv::setSaveToFileURL(const KURL& url)
{
  m_fileURL = url;  //Parent's class url
  m_saveToTmpFileURL = KURL(url.url() + ".part");
}

void DccTransferRecv::startConnectionTimer(int sec)
{
  stopConnectionTimer();
  m_connectionTimer = new QTimer();
  connect(m_connectionTimer, SIGNAL(timeout()), this, SLOT(connectionTimeout()));
  m_connectionTimer->start(sec*1000, TRUE);
}

void DccTransferRecv::stopConnectionTimer()
{
  if(m_connectionTimer)
  {
    m_connectionTimer->stop();
    delete m_connectionTimer;
    m_connectionTimer = 0;
  }
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
  
  connect( this,          SIGNAL( dataFinished() ),                    m_transferJob, SLOT( slotFinished() ) );
  connect( m_transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ), this,          SLOT( slotKIODataReq( KIO::Job*, QByteArray& ) ) );
  connect( m_transferJob, SIGNAL( result( KIO::Job* ) ),               this,          SLOT( slotKIOResult() ) );
  
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
  delete cache;
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
  if ( m_writeAsyncMode )
    m_writeReady = true;
  else
  {
    if ( !m_cacheList.isEmpty() )
      data = *( popCache() );
    else
    {
      kdDebug() << "DccTransferRecvWriteCacheHandler::slotKIODataReq(): flushing done." << endl;
      m_transferJob = 0;
      emit done();  // ->DccTransferRecv
    }
  }
}

void DccTransferRecvWriteCacheHandler::slotKIOResult()
{
  if(m_transferJob->error())
  {
    emit gotError(m_transferJob->error());
    m_transferJob = 0;
  }
}

#include "dcctransferrecv.moc"
