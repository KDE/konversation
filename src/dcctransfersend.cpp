/*
  send a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004,2005 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <qfile.h>
#include <qtimer.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kserversocket.h>
#include <ksocketaddress.h>
#include <kstreamsocket.h>
#include <kio/netaccess.h>
#include <kfileitem.h>
#include <kinputdialog.h>

#include "dccpanel.h"
#include "dcctransfersend.h"
#include "konversationapplication.h"

using namespace KNetwork;

DccTransferSend::DccTransferSend( DccPanel* panel, const QString& partnerNick, const KURL& fileURL, const QString& ownIp, const QString &altFileName, uint fileSize  )
: DccTransfer( panel, DccTransfer::Send, partnerNick )
{
    kdDebug() << "DccTransferSend::DccTransferSend()" << endl
        << "DccTransferSend::DccTransferSend(): Partner: " << partnerNick << endl
        << "DccTransferSend::DccTransferSend(): File: " << fileURL.prettyURL() << endl;

    m_fileName = fileURL.fileName();
    m_fileURL = fileURL;
    m_ownIp = ownIp;

    if(Preferences::dccIPv4Fallback())
    {
        KIpAddress ip(m_ownIp);
        if(ip.isIPv6Addr())
        {
            /* This is fucking ugly but there is no KDE way to do this yet :| -cartman */
            struct ifreq ifr;
            const char* address = Preferences::dccIPv4FallbackIface().ascii();
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            strncpy( ifr.ifr_name, address, IF_NAMESIZE );
            ifr.ifr_addr.sa_family = AF_INET;
            if( ioctl( sock, SIOCGIFADDR, &ifr ) >= 0 )
                m_ownIp =  inet_ntoa( ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr );
            kdDebug() << "Falling back to IPv4 address " << m_ownIp << endl;
        }
    }

    if ( altFileName.isEmpty() )
        m_fileName = m_fileURL.fileName();
    else
        m_fileName = altFileName;

    m_serverSocket = 0;
    m_sendSocket = 0;
    m_fastSend = Preferences::dccFastSend();
    kdDebug() << "DccTransferSend::DccTransferSend(): Fast DCC send: " << m_fastSend << endl;

    m_connectionTimer = new QTimer( this );

    if (!kapp->authorize("allow_downloading"))
    {
        //Do not have the rights to send the file.  Shouldn't have gotten this far anyway
        //Note this is after the initialisation so the view looks correct still
        failed(i18n("The admin has restricted the right to send files"));
        return;
    }

    connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
    //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted

    //Check the file exists
    if ( !KIO::NetAccess::exists( m_fileURL, true, listView() ) )
    {
        failed( i18n( "The url \"%1\" does not exist" ).arg( m_fileURL.prettyURL() ) );
        return;
    }

    //FIXME: KIO::NetAccess::download() is a synchronous function. we should use KIO::get() instead.
    //Download the file.  Does nothing if it's local (file:/)
    if ( !KIO::NetAccess::download( m_fileURL, m_tmpFile, listView() ) )
    {
        failed( i18n( "Could not retrieve \"%1\"" ).arg( m_fileURL.prettyURL() ) );
        kdDebug() << "DccTransferSend::DccTransferSend(): KIO::NetAccess::download() failed. reason: " << KIO::NetAccess::lastErrorString() << endl;
        return;
    }

    //Some protocols, like http, maybe not return a filename, and altFileName may be empty, So prompt the user for one.
    if ( m_fileName.isEmpty() )
    {
        bool pressedOk;
        m_fileName = KInputDialog::getText( i18n( "Enter Filename" ), i18n( "<qt>The file that you are sending to <i>%1</i> does not have a filename.<br>Please enter a filename to be presented to the receiver, or cancel the dcc transfer</qt>" ).arg( getPartnerNick() ), "unknown", &pressedOk, listView() );

        if ( !pressedOk )
        {
            failed( i18n( "No filename was given" ) );
            return;
        }
    }

    m_fileName = m_fileName.lower();
    m_fileName.replace( " ", "_" );

    m_file.setName( m_tmpFile );

    if ( fileSize > 0 )
        m_fileSize = fileSize;
    else
        m_fileSize = m_file.size();

    updateView();

    panel->selectMe( this );
}

DccTransferSend::~DccTransferSend()
{
    cleanUp();
}

QString DccTransferSend::getTypeText() const
{
    return i18n( "Send" );
}

QPixmap DccTransferSend::getTypeIcon() const
{
    return KGlobal::iconLoader()->loadIcon( "up", KIcon::Small );
}

void DccTransferSend::cleanUp()
{
    kdDebug() << "DccTransferSend::cleanUp()" << endl;
    stopConnectionTimer();
    finishTransferMeter();
    if ( !m_tmpFile.isEmpty() )
        KIO::NetAccess::removeTempFile( m_tmpFile );
    m_tmpFile = QString::null;
    m_file.close();
    if ( m_sendSocket )
    {
        m_sendSocket->close();
        m_sendSocket = 0;                         // the instance will be deleted automatically by its parent
    }
    if ( m_serverSocket )
    {
        m_serverSocket->close();
        m_serverSocket = 0;                       // the instance will be deleted automatically by its parent
    }
}

// just for convenience
void DccTransferSend::failed( const QString& errorMessage )
{
    setStatus( Failed, errorMessage );
    updateView();
    cleanUp();
    emit done( m_fileURL.path(), Failed, errorMessage );
    openDetailDialog();
}

void DccTransferSend::abort()                     // public slot
{
    kdDebug() << "DccTransferSend::abort()" << endl;

    setStatus( Aborted );
    updateView();
    cleanUp();
    emit done( m_fileURL.path(), Aborted );
}

void DccTransferSend::start()                     // public slot
{
    kdDebug() << "DccTransferSend::start()" << endl;

    if ( getStatus() != Queued )
        return;

    // Set up server socket
    m_serverSocket = new KNetwork::KServerSocket( this );
    m_serverSocket->setFamily( KNetwork::KResolver::InetFamily );

                                                  // user is specifing ports
    if ( Preferences::dccSpecificSendPorts() )
    {
        // set port
        bool found = false;                       // whether succeeded to set port
        unsigned long port = Preferences::dccSendPortsFirst();
        for ( ; port <= Preferences::dccSendPortsLast() ; ++port )
        {
            kdDebug() << "DccTransferSend::start(): trying port " << port << endl;
            m_serverSocket->setAddress( QString::number( port ) );
            bool success = m_serverSocket->listen();
            if ( found = ( success && m_serverSocket->error() == KNetwork::KSocketBase::NoError ) )
                break;
            m_serverSocket->close();
        }
        if ( !found )
        {
            failed( i18n( "No vacant port" ) );
            return;
        }
    }
    else                                          // user isn't specifing ports
    {
        // Let the operating system choose a port
        m_serverSocket->setAddress( "0" );

        if ( !m_serverSocket->listen() )
        {
            kdDebug() << "DccTransferSend::start(): listen() failed!" << endl;
            failed( i18n( "Could not open a socket" ) );
            return;
        }
    }

    connect( m_serverSocket, SIGNAL( readyAccept() ),   this, SLOT( heard() )            );
    connect( m_serverSocket, SIGNAL( gotError( int ) ), this, SLOT( socketError( int ) ) );
    connect( m_serverSocket, SIGNAL( closed() ),        this, SLOT( slotServerSocketClosed() ) );

    // Get our own port number
    KNetwork::KSocketAddress ipAddr = m_serverSocket->localAddress();
    const struct sockaddr_in* socketAddress = (sockaddr_in*)ipAddr.address();
    m_ownPort = QString::number( ntohs( socketAddress->sin_port ) );

    kdDebug() << "DccTransferSend::start(): own Address=" << m_ownIp << ":" << m_ownPort << endl;

    setStatus( WaitingRemote, i18n( "Waiting remote user's acceptance" ) );
    updateView();

    startConnectionTimer( Preferences::dccSendTimeout() );

    emit sendReady( m_partnerNick, m_fileName, getNumericalIpText( m_ownIp ), m_ownPort, m_fileSize );
}

                                                  // public
bool DccTransferSend::setResume( unsigned long position )
{
    kdDebug() << "DccTransferSend::setResume(): position=" << position << endl;

    if ( position < m_fileSize )
    {
        m_resumed = true;
        m_transferringPosition = position;
        updateView();
        return true;
    }
    else
    {
        kdDebug() << "DccTransferSend::setResume(): Invalid position. (greater than filesize=" << QString::number( m_fileSize ) << ")" << endl;
        updateView();
        return false;
    }
}

void DccTransferSend::heard()                     // slot
{
    kdDebug() << "DccTransferSend::heard()" << endl;

    stopConnectionTimer();

    m_sendSocket = static_cast<KNetwork::KStreamSocket*>( m_serverSocket->accept() );
    if ( !m_sendSocket )
    {
        failed( i18n( "Could not accept the connection. (Socket Error)" ) );
        return;
    }

    connect( m_sendSocket, SIGNAL( readyWrite() ), this, SLOT( writeData() )            );
    connect( m_sendSocket, SIGNAL( readyRead() ),  this, SLOT( getAck() )               );
    connect( m_sendSocket, SIGNAL( closed() ),     this, SLOT( slotSendSocketClosed() ) );

    if ( m_sendSocket->peerAddress().asInet().ipAddress().isV4Mapped() )
        m_partnerIp = KNetwork::KIpAddress( m_sendSocket->peerAddress().asInet().ipAddress().IPv4Addr() ).toString();
    else
        m_partnerIp = m_sendSocket->peerAddress().asInet().ipAddress().toString();
    m_partnerPort = m_sendSocket->peerAddress().serviceName();

    // we don't need ServerSocket anymore
    m_serverSocket->close();

    if ( m_file.open( IO_ReadOnly ) )
    {
        // seek to file position to make resume work
        m_file.at( m_transferringPosition );
        m_transferStartPosition = m_transferringPosition;

        setStatus( Sending );
        m_sendSocket->enableWrite( true );
        m_sendSocket->enableRead( m_fastSend );
        initTransferMeter();                      // initialize CPS counter, ETA counter, etc...
        updateView();
    }
    else
        failed( i18n( "Could not open the file: %1" ).arg( getQFileErrorString( m_file.status() ) ) );
}

void DccTransferSend::writeData()                 // slot
{
    //kdDebug() << "DccTransferSend::writeData()" << endl;
    if ( !m_fastSend )
    {
        m_sendSocket->enableWrite( false );
        m_sendSocket->enableRead( true );
    }
    int actual = m_file.readBlock( m_buffer, m_bufferSize );
    if ( actual > 0 )
    {
        m_sendSocket->writeBlock( m_buffer, actual );
        m_transferringPosition += actual;
        if ( (KIO::fileoffset_t)m_fileSize <= m_transferringPosition )
        {
            Q_ASSERT( (KIO::fileoffset_t)m_fileSize == m_transferringPosition );
            kdDebug() << "DccTransferSend::writeData(): Done." << endl;
            m_sendSocket->enableWrite( false );   // there is no need to call this function anymore
        }
    }
}

void DccTransferSend::getAck()                    // slot
{
    //kdDebug() << "DccTransferSend::getAck()" << endl;
    if ( !m_fastSend && m_transferringPosition < (KIO::fileoffset_t)m_fileSize )
    {
        m_sendSocket->enableWrite( true );
        m_sendSocket->enableRead( false );
    }
    unsigned long pos;
    while ( m_sendSocket->bytesAvailable() >= 4 )
    {
        m_sendSocket->readBlock( (char*)&pos, 4 );
        pos = intel( pos );
        if ( pos == m_fileSize )
        {
            kdDebug() << "DccTransferSend::getAck(): Received final ACK." << endl;
            finishTransferMeter();
            setStatus( Done );
            updateView();
            cleanUp();
            emit done( m_fileURL.path() );
            break;                                // for safe
        }
    }
}

void DccTransferSend::socketError( int errorCode )
{
    kdDebug() << "DccTransferSend::socketError(): code =  " << errorCode << " string = " << m_serverSocket->errorString() << endl;
    failed( i18n( "Socket error: %1" ).arg( m_serverSocket->errorString() ) );
}

void DccTransferSend::startConnectionTimer( int sec )
{
    kdDebug() << "DccTransferSend::startConnectionTimer()"<< endl;
    stopConnectionTimer();
    m_connectionTimer->start( sec*1000, true );
}

void DccTransferSend::stopConnectionTimer()
{
    if ( m_connectionTimer->isActive() )
    {
        kdDebug() << "DccTransferSend::stopConnectionTimer(): stop" << endl;
        m_connectionTimer->stop();
    }
}

void DccTransferSend::connectionTimeout()         // slot
{
    kdDebug() << "DccTransferSend::connectionTimeout()" << endl;
    failed( i18n( "Timed out" ) );
}

void DccTransferSend::slotServerSocketClosed()
{
    kdDebug() << "DccTransferSend::slotServerSocketClosed()" << endl;
}

void DccTransferSend::slotSendSocketClosed()
{
    kdDebug() << "DccTransferSend::slotSendSocketClosed()" << endl;
    finishTransferMeter();
    if ( m_dccStatus == Sending && m_transferringPosition < (KIO::fileoffset_t)m_fileSize )
        failed( i18n( "Remote user disconnected" ) );
}

                                                  // protected, static
QString DccTransferSend::getQFileErrorString( int code )
{
    QString errorString;

    switch(code)
    {
        case IO_Ok:
            errorString=i18n("The operation was successful. Should never happen in an error dialog.");
            break;
        case IO_ReadError:
            errorString=i18n("Could not read from file \"%1\".");
            break;
        case IO_WriteError:
            errorString=i18n("Could not write to file \"%1\".");
            break;
        case IO_FatalError:
            errorString=i18n("A fatal unrecoverable error occurred.");
            break;
        case IO_OpenError:
            errorString=i18n("Could not open file \"%1\".");
            break;

            // Same case value? Damn!
            //        case IO_ConnectError:
            //          errorString="Could not connect to the device.";
            //        break;

        case IO_AbortError:
            errorString=i18n("The operation was unexpectedly aborted.");
            break;
        case IO_TimeOutError:
            errorString=i18n("The operation timed out.");
            break;
        case IO_UnspecifiedError:
            errorString=i18n("An unspecified error happened on close.");
            break;
        default:
            errorString=i18n("Unknown error. Code %1").arg(code);
            break;
    }

    return errorString;
}

#include "dcctransfersend.moc"
