/*
  receive a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "transferrecv.h" ////// header renamed
#include "dcccommon.h"
#include "channel.h"
#include "transfermanager.h" ////// header renamed
#include "application.h" ////// header renamed
#include "connectionmanager.h"
#include "server.h"

#include <QTcpServer>
#include <QTcpSocket>

#include <kdebug.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdirselectdialog.h>
#include <kuser.h>
#include <kauthorized.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>


class DccResumeDialog;

/*
 *flow chart*

 DccTransferRecv()

 start()              : called from DccTransferPanel when user pushes the accept button
  | \
  | requestResume()   : called when user chooses to resume in DccResumeDialog. it emits the signal ResumeRequest()
  |
  | startResume()     : called by "Server"
  | |
connectToSender()

connectionSuccess()  : called by recvSocket

*/

DccTransferRecv::DccTransferRecv(QObject* parent)
    : DccTransfer( DccTransfer::Receive, parent )
{
    kDebug();

    m_serverSocket = 0;
    m_recvSocket = 0;
    m_writeCacheHandler = 0;

    m_connectionTimer = new QTimer( this );
    m_connectionTimer->setSingleShot(true);
    connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
    //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted
}

DccTransferRecv::~DccTransferRecv()
{
    cleanUp();
}

QString DccTransferRecv::getTypeText() const
{
    return i18n( "Receive" );
}

QPixmap DccTransferRecv::getTypeIcon() const
{
    return KIconLoader::global()->loadIcon( "arrow-down",KIconLoader::Small );
}

void DccTransferRecv::cleanUp()
{
    kDebug();

    stopConnectionTimer();
    finishTransferLogger();
    if ( m_serverSocket )
    {
        m_serverSocket->close();
        m_serverSocket = 0;
    }
    if ( m_recvSocket )
    {
        m_recvSocket->close();
        m_recvSocket = 0;                         // the instance will be deleted automatically by its parent
    }
    if ( m_writeCacheHandler )
    {
        m_writeCacheHandler->closeNow();
        m_writeCacheHandler->deleteLater();
        m_writeCacheHandler = 0;
    }
}

void DccTransferRecv::setPartnerIp( const QString& ip )
{
    if ( getStatus() == Configuring )
        m_partnerIp = ip;
}

void DccTransferRecv::setPartnerPort( uint port )
{
    if ( getStatus() == Configuring )
        m_partnerPort = port;
}

void DccTransferRecv::setFileSize( unsigned long fileSize )
{
    if ( getStatus() == Configuring )
        m_fileSize = fileSize;
}

void DccTransferRecv::setFileName( const QString& fileName )
{
    if ( getStatus() == Configuring )
    {
        m_fileName = fileName;
        m_saveFileName = m_fileName;
    }
}

void DccTransferRecv::setFileURL( const KUrl& url )
{
    if ( getStatus() == Preparing || getStatus() == Configuring || getStatus() == Queued )
    {
        m_fileURL = url;
        m_saveFileName = url.fileName();
    }
}

void DccTransferRecv::setReverse( bool reverse, const QString& reverseToken )
{
    if ( getStatus() == Configuring )
    {
        m_reverse = reverse;
        if ( reverse )
        {
            m_partnerPort = 0;
            m_reverseToken = reverseToken;
        }
    }
}

bool DccTransferRecv::queue()
{
    kDebug();

    if ( getStatus() != Configuring )
    {
        return false;
    }

    if ( m_partnerIp.isEmpty())
    {
        return false;
    }

    if (!KAuthorized::authorizeKAction("allow_downloading"))
    {
        //note we have this after the initialisations so that item looks okay
        //Do not have the rights to send the file.  Shouldn't have gotten this far anyway
        failed(i18n("The admin has restricted the right to receive files"));
        return false;
    }

    // check if the sender IP is valid
    if ( m_partnerIp == "0.0.0.0" )
    {
        failed( i18n( "Invalid sender address (%1)", m_partnerIp ) );
        return false;
    }

    // TODO: should we support it?
    if ( m_fileSize == 0 )
    {
        failed( i18n( "Unsupported negotiation (filesize=0)" ) );
        return false;
    }

    if ( m_fileName.isEmpty() )
    {
        m_fileName = "unnamed_file";
        m_saveFileName = m_fileName;
    }

    if ( m_fileURL.isEmpty() )
    {
        // determine default incoming file URL

        // set default folder
        if ( !Preferences::self()->dccPath().isEmpty() )
        {
            m_fileURL = KUrl( Preferences::self()->dccPath() );
        }
        else
        {
            m_fileURL.setPath( KUser( KUser::UseRealUserID ).homeDir() );  // default folder is *not* specified
        }

        // add a slash if there is none
        m_fileURL.adjustPath( KUrl::AddTrailingSlash );

        // Append folder with partner's name if wanted
        if ( Preferences::self()->dccCreateFolder() )
        {
            m_fileURL.addPath( m_partnerNick + '/' );
        }

        // Just incase anyone tries to do anything nasty
        QString fileNameSanitized = sanitizeFileName( m_saveFileName );

        // Append partner's name to file name if wanted
        if ( Preferences::self()->dccAddPartner() )
        {
            m_fileURL.addPath( m_partnerNick + '.' + fileNameSanitized );
        }
        else
        {
            m_fileURL.addPath( fileNameSanitized );
        }
    }

    return DccTransfer::queue();
}

void DccTransferRecv::abort()                     // public slot
{
    kDebug();

    if (getStatus() == DccTransfer::Queued)
    {
        Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByConnectionId( m_connectionId );
        if ( server )
        {
            server->dccRejectSend( m_partnerNick, transferFileName(m_fileName) );
        }
    }

    if(m_writeCacheHandler)
    {
        m_writeCacheHandler->write( true );       // flush
    }

    cleanUp();
    setStatus( Aborted );
    emit done( this );
}

void DccTransferRecv::start()                     // public slot
{
    kDebug() << "[BEGIN]";

    if ( getStatus() != Queued )
        return;

    setStatus( Preparing );

    prepareLocalKio( false, false );

    kDebug() << "[END]";
}

void DccTransferRecv::prepareLocalKio( bool overwrite, bool resume, KIO::fileoffset_t startPosition /* = 0 */ )
{
    kDebug()
        << "URL: " << m_fileURL << endl
        << "Overwrite: " << overwrite << endl
        << "Resume: " << resume << " (Position: " << QString::number( startPosition ) << ")";

    m_resumed = resume;
    m_transferringPosition = startPosition;

    if ( !createDirs( m_fileURL.upUrl() ) )
    {
        askAndPrepareLocalKio( i18n( "<b>Cannot create the folder.</b><br>"
            "Folder: %1<br>",
            m_fileURL.upUrl().prettyUrl() ),
            DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
            DccResumeDialog::RA_Rename );
        return;
    }

    KIO::JobFlags flags;
    if(overwrite)
        flags |= KIO::Overwrite;
    if(m_resumed)
        flags |= KIO::Resume;
    //for now, maybe later
    flags |= KIO::HideProgressInfo;
    KIO::TransferJob* transferJob = KIO::put( m_fileURL, -1, flags );

    if ( !transferJob )
    {
        kDebug() << "KIO::put() returned NULL. what happened?";
        failed( i18n( "Could not create a KIO instance" ) );
        return;
    }

    connect( transferJob, SIGNAL( canResume( KIO::Job*, KIO::filesize_t ) ), this, SLOT( slotLocalCanResume( KIO::Job*, KIO::filesize_t ) ) );
    connect( transferJob, SIGNAL( result( KJob* ) ),                         this, SLOT( slotLocalGotResult( KJob* ) ) );
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
    }
}

bool DccTransferRecv::createDirs( const KUrl& dirURL ) const
{
    KUrl kurl( dirURL );
    QString surl = kurl.url();

    //First we split directories until we reach to the top,
    //since we need to create directories one by one

    QStringList dirList;
    while ( surl != kurl.upUrl().url() )
    {
        dirList.prepend( surl );
        kurl = kurl.upUrl();
        surl = kurl.url();
    }

    //Now we create the directories

    QStringList::ConstIterator it;
    for ( it=dirList.constBegin() ; it!=dirList.constEnd() ; ++it )
        if ( !KIO::NetAccess::exists( *it, KIO::NetAccess::SourceSide, NULL ) )
            if ( !KIO::NetAccess::mkdir( *it, NULL, -1 ) )
                return false;

    return true;
}

void DccTransferRecv::slotLocalCanResume( KIO::Job* job, KIO::filesize_t size )
{
    kDebug() << "[BEGIN]" << endl
        << "size: " << QString::number( size ); 

    if ( size != 0 )
    {
        KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );

        disconnect( transferJob, 0, 0, 0 );
        transferJob->kill();

        if ( KonversationApplication::instance()->getDccTransferManager()->isLocalFileInWritingProcess( m_fileURL ) )
        {
            askAndPrepareLocalKio( i18n( "<b>The file is used by another transfer.</b><br>"
                "%1<br>",
                m_fileURL.prettyUrl() ),
                DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                DccResumeDialog::RA_Rename );
        }
        else if ( Preferences::self()->dccAutoResume() )
        {
            prepareLocalKio( false, true, size );
        }
        else
        {
            askAndPrepareLocalKio( i18np(
                "<b>A partial file exists:</b><br/>"
                "%2<br/>"
                "Size of the partial file: 1 byte.<br/>",
                "<b>A partial file exists:</b><br/>"
                "%2<br/>"
                "Size of the partial file: %1 bytes.<br/>",
                size,
                m_fileURL.prettyUrl()),
                DccResumeDialog::RA_Resume | DccResumeDialog::RA_Overwrite | DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                DccResumeDialog::RA_Resume,
                size );
        }
    }

    kDebug() << "[END]";
}

void DccTransferRecv::slotLocalGotResult( KJob* job )
{
    kDebug() << "[BEGIN]";

    KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );
    disconnect( transferJob, 0, 0, 0 );

    switch ( transferJob->error() )
    {
        case 0:                                   // no error
            kDebug() << "DccTransferRecv::slotLocalGotResult(): job->error() returned 0." << endl
                << "DccTransferRecv::slotLocalGotResult(): Why was I called in spite of no error?";
            break;
        case KIO::ERR_FILE_ALREADY_EXIST:
            askAndPrepareLocalKio( i18n( "<b>The file already exists.</b><br>"
                "%1<br>",
                m_fileURL.prettyUrl() ),
                DccResumeDialog::RA_Overwrite | DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                DccResumeDialog::RA_Overwrite );
            break;
        default:
            askAndPrepareLocalKio( i18n( "<b>Could not open the file.<br>"
                "Error: %1</b><br>"
                "%2<br>",
                transferJob->error(),
                m_fileURL.prettyUrl() ),
                DccResumeDialog::RA_Rename | DccResumeDialog::RA_Cancel,
                DccResumeDialog::RA_Rename );
    }

    kDebug() << "[END]";
}

void DccTransferRecv::slotLocalReady( KIO::Job* job )
{
    kDebug();

    KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>( job );

    disconnect( transferJob, 0, 0, 0 );           // WriteCacheHandler will control the job after this

    m_writeCacheHandler = new DccTransferRecvWriteCacheHandler( transferJob );

    connect( m_writeCacheHandler, SIGNAL( done() ),                     this, SLOT( slotLocalWriteDone() )                     );
    connect( m_writeCacheHandler, SIGNAL( gotError( const QString& ) ), this, SLOT( slotLocalGotWriteError( const QString& ) ) );

    if ( !m_resumed )
        connectWithSender();
    else
        requestResume();
}

void DccTransferRecv::connectWithSender()
{
    if ( m_reverse )
    {
        if ( !startListeningForSender() )
            return;

        Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByConnectionId( m_connectionId );
        if ( !server )
        {
            failed( i18n( "Could not send Reverse DCC SEND acknowledgement to the partner via the IRC server." ) );
            return;
        }

        m_ownIp = DccCommon::getOwnIp( server );
        m_ownPort = m_serverSocket->serverPort();

        setStatus( WaitingRemote, i18n( "Waiting for connection" ) );

        server->dccReverseSendAck( m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp( m_ownIp ), m_ownPort, m_fileSize, m_reverseToken );
    }
    else
    {
        connectToSendServer();
    }
}

void DccTransferRecv::requestResume()
{
    kDebug();

    setStatus( WaitingRemote, i18n( "Waiting for remote host's acceptance" ) );

    startConnectionTimer( 30 );

    kDebug() << "Requesting resume for " << m_partnerNick << " file " << m_fileName << " partner " << m_partnerPort;

    Server* server = KonversationApplication::instance()->getConnectionManager()->getServerByConnectionId( m_connectionId );
    if ( !server )
    {
        kDebug() << "Could not retrieve the instance of Server. Connection id: " << m_connectionId;
        failed( i18n( "Could not send DCC RECV resume request to the partner via the IRC server." ) );
        return;
    }

    if (m_reverse)
        server->dccPassiveResumeGetRequest( m_partnerNick, transferFileName(m_fileName), m_partnerPort, m_transferringPosition, m_reverseToken );
    else
        server->dccResumeGetRequest( m_partnerNick, transferFileName(m_fileName), m_partnerPort, m_transferringPosition );
}

                                                  // public slot
void DccTransferRecv::startResume( unsigned long position )
{
    kDebug() << "Position:" << position;

    stopConnectionTimer();

    if ( (unsigned long)m_transferringPosition != position )
    {
        kDebug() << "DccTransferRecv::startResume(): remote responsed an unexpected position"<< endl
            << "DccTransferRecv::startResume(): expected: " << QString::number( m_transferringPosition ) << endl
            << "DccTransferRecv::startResume(): remote response: " << position;
        failed( i18n( "Unexpected response from remote host" ) );
        return;
    }

    connectWithSender();
}

void DccTransferRecv::connectToSendServer()
{
    kDebug();

    // connect to sender

    setStatus( Connecting );

    startConnectionTimer( 30 );

    m_recvSocket = new QTcpSocket( this);

    connect( m_recvSocket, SIGNAL( connected( ) ), this, SLOT( startReceiving() )     );
    connect( m_recvSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( connectionFailed( QAbstractSocket::SocketError ) ) );

    kDebug() << "Attempting to connect to " << m_partnerIp << ":" << m_partnerPort;

    m_recvSocket->connectToHost(m_partnerIp, m_partnerPort);
}

bool DccTransferRecv::startListeningForSender()
{
    // Set up server socket
    QString failedReason;
    if ( Preferences::self()->dccSpecificSendPorts() )
        m_serverSocket = DccCommon::createServerSocketAndListen( this, &failedReason, Preferences::self()->dccSendPortsFirst(), Preferences::self()->dccSendPortsLast() );
    else
        m_serverSocket = DccCommon::createServerSocketAndListen( this, &failedReason );
    if ( !m_serverSocket )
    {
        failed( failedReason );
        return false;
    }

    connect( m_serverSocket, SIGNAL( newConnection() ),   this, SLOT( slotServerSocketReadyAccept() ) );

    startConnectionTimer( 30 );

    return true;
}

void DccTransferRecv::slotServerSocketReadyAccept()
{
    //reverse dcc

    m_recvSocket = m_serverSocket->nextPendingConnection();
    if ( !m_recvSocket )
    {
        failed( i18n( "Could not accept the connection (socket error.)" ) );
        return;
    }

    connect( m_recvSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( connectionFailed( QAbstractSocket::SocketError ) ) );

    // we don't need ServerSocket anymore
    m_serverSocket->close();

    startReceiving();
}

void DccTransferRecv::slotServerSocketGotError( int /* errorCode*/ )
{
    failed( i18n( "Socket error: %1", m_serverSocket->errorString() ) );
}

void DccTransferRecv::startReceiving()
{
    kDebug();
    stopConnectionTimer();

    connect( m_recvSocket, SIGNAL( readyRead() ),                        this, SLOT( readData() )              );
    //connect( m_recvSocket, SIGNAL( readyWrite() ),                       this, SLOT( sendAck() )               );

    //Not needed, error is also emitted when this happens + covers more cases
    //connect( m_recvSocket, SIGNAL( disconnected() ),                           this, SLOT( slotSocketClosed() )      );

    m_transferStartPosition = m_transferringPosition;

    //we don't need the original filename anymore, overwrite it to display the correct one in transfermanager/panel
    m_fileName = m_saveFileName;

    startTransferLogger();                          // initialize CPS counter, ETA counter, etc...

    setStatus( Transferring );
}

                                                  // slot
void DccTransferRecv::connectionFailed( QAbstractSocket::SocketError errorCode )
{
    finishTransferLogger();
    kDebug() << "Code = " << errorCode << ", string = " << m_recvSocket->errorString();
    failed( m_recvSocket->errorString() );
    setStatus( Failed );
    emit done( this );
}

void DccTransferRecv::readData()                  // slot
{
    //kDebug();
    int actual = m_recvSocket->read( m_buffer, m_bufferSize );
    if ( actual > 0 )
    {
        //actual is the size we read in, and is guaranteed to be less than m_bufferSize
        m_transferringPosition += actual;
        m_writeCacheHandler->append( m_buffer, actual );
        m_writeCacheHandler->write( false );
        //in case we could not read all the data, leftover data could get lost
        if (m_recvSocket->bytesAvailable() > 0)
            readData();
        else
            sendAck();
    }
}

void DccTransferRecv::sendAck()                   // slot
{
    //kDebug() << m_transferringPosition << "/" << (KIO::fileoffset_t)m_fileSize;
    KIO::fileoffset_t pos = intel( m_transferringPosition );

    m_recvSocket->write( (char*)&pos, 4 );
    if ( m_transferringPosition == (KIO::fileoffset_t)m_fileSize )
    {
        kDebug() << "Sent final ACK.";
        disconnect( m_recvSocket, 0, 0, 0 );
        finishTransferLogger();
        m_writeCacheHandler->close();             // WriteCacheHandler will send the signal done()
    }
    else if ( m_transferringPosition > (KIO::fileoffset_t)m_fileSize )
    {
        kDebug() << "The remote host sent larger data than expected: " << QString::number( m_transferringPosition );
        failed( i18n( "Transfer error" ) );
    }
}

void DccTransferRecv::slotLocalWriteDone()        // <-WriteCacheHandler::done()
{
    kDebug();
    cleanUp();
    setStatus( Done );
    emit done( this );
}

                                                  // <- WriteCacheHandler::gotError()
void DccTransferRecv::slotLocalGotWriteError( const QString& errorString )
{
    kDebug();
    failed( i18n( "KIO error: %1", errorString ) );
}

void DccTransferRecv::startConnectionTimer( int sec )
{
    kDebug();
    m_connectionTimer->start( sec*1000 );
}

void DccTransferRecv::stopConnectionTimer()
{
    if ( m_connectionTimer->isActive() )
    {
        m_connectionTimer->stop();
        kDebug();
    }
}

void DccTransferRecv::connectionTimeout()         // slot
{
    kDebug();
    failed( i18n( "Timed out" ) );
}

/*
void DccTransferRecv::slotSocketClosed()
{
    finishTransferLogger();
    if ( getStatus() == Transferring )
        failed( i18n( "Remote user disconnected" ) );
}
*/

// WriteCacheHandler

DccTransferRecvWriteCacheHandler::DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob )
: m_transferJob( transferJob )
{
    m_writeReady = true;
    m_cacheStream = 0;

    //never emitted?
    //connect( this,          SIGNAL( dataFinished() ),                    m_transferJob, SLOT( slotFinished() )                           );
    connect( m_transferJob, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ), this,          SLOT( slotKIODataReq( KIO::Job*, QByteArray& ) ) );
    connect( m_transferJob, SIGNAL( result(KJob* ) ),                    this,          SLOT( slotKIOResult( KJob* ) )                   );

    m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = true );
}

DccTransferRecvWriteCacheHandler::~DccTransferRecvWriteCacheHandler()
{
    closeNow();
}

                                                  // public
void DccTransferRecvWriteCacheHandler::append( char* data, int size )
{
    // sendAsyncData() and dataReq() cost a lot of time, so we should pack some caches.

    static const int maxWritePacketSize = 1 * 1024 * 1024; // 1meg

    if ( m_cacheList.isEmpty() || m_cacheList.back().size() + size > maxWritePacketSize )
    {
        m_cacheList.append( QByteArray() );
        delete m_cacheStream;
        m_cacheStream = new QDataStream( &m_cacheList.back(), QIODevice::WriteOnly );
    }

    m_cacheStream->writeRawData( data, size );
}

                                                  // public
bool DccTransferRecvWriteCacheHandler::write( bool force )
{
    // force == false: return without doing anything when the whole cache size is smaller than maxWritePacketSize

    if ( m_cacheList.isEmpty() || !m_transferJob || !m_writeReady || !m_writeAsyncMode )
        return false;

    if ( !force && m_cacheList.count() < 2 )
        return false;

    // do write
    m_writeReady = false;

    m_transferJob->sendAsyncData( m_cacheList.front() );
    //kDebug() << "wrote " << m_cacheList.front().size() << " bytes.";
    m_cacheList.pop_front();

    return true;
}

void DccTransferRecvWriteCacheHandler::close()    // public
{
    kDebug();
    write( true );                                // write once if kio is ready to write
    m_transferJob->setAsyncDataEnabled( m_writeAsyncMode = false );
    kDebug() << "switched to synchronized mode.";
    kDebug() << "flushing... (remaining caches: " << m_cacheList.count() << ")";
}

void DccTransferRecvWriteCacheHandler::closeNow() // public
{
    write( true );                                // flush
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
    // We are in writeAsyncMode if there is more data to be read in from dcc
    if ( m_writeAsyncMode )
        m_writeReady = true;
    else
    {
        // No more data left to read from incoming dcctransfer
        if ( !m_cacheList.isEmpty() )
        {
            // once we write everything in cache, the file is complete.
            // This function will be called once more after this last data is written.
            data = m_cacheList.front();
            kDebug() << "will write " << m_cacheList.front().size() << " bytes.";
            m_cacheList.pop_front();
        }
        else
        {
            // finally, no data left to write or read.
            kDebug() << "flushing done.";
            m_transferJob = 0;
            emit done();                          // -> DccTransferRecv::slotLocalWriteDone()
        }
    }
}

void DccTransferRecvWriteCacheHandler::slotKIOResult( KJob* job )
{
    Q_ASSERT( m_transferJob );

    disconnect( m_transferJob, 0, 0, 0 );
    m_transferJob = 0;

    if ( job->error() )
    {
        QString errorString = job->errorString();
        closeNow();
        emit gotError( errorString );             // -> DccTransferRecv::slotLocalGotWriteError()
    }
}

#include "transferrecv.moc"
