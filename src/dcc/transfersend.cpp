/*
  send a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
/*
  Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/
/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "transfersend.h"
#include "dcccommon.h"
#include "transfermanager.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "upnprouter.h"

#include <QFile>
#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>

#include <KFileItem>
#include <KIO/NetAccess>

// TODO: remove the dependence
#include <KInputDialog>
#include <KAuthorized>

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#ifndef Q_CC_MSVC
#   include <net/if.h>
#   include <sys/ioctl.h>
#   ifdef HAVE_STROPTS_H
#       include <stropts.h>
#   endif
#endif
#include <arpa/inet.h>

using namespace Konversation::UPnP;

namespace Konversation
{
    namespace DCC
    {
        TransferSend::TransferSend(QObject* parent)
            : Transfer( Transfer::Send, parent )
        {
            kDebug();

            m_serverSocket = 0;
            m_sendSocket = 0;

            m_connectionTimer = new QTimer( this );
            m_connectionTimer->setSingleShot(true);
            connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( slotConnectionTimeout() ) );

            // set defualt values
            m_reverse = Preferences::self()->dccPassiveSend();
        }

        TransferSend::~TransferSend()
        {
            cleanUp();
        }

        void TransferSend::cleanUp()
        {
            kDebug();

            stopConnectionTimer();
            disconnect(m_connectionTimer, 0, 0, 0);

            finishTransferLogger();
            if ( !m_tmpFile.isEmpty() )
            {
                KIO::NetAccess::removeTempFile( m_tmpFile );
            }

            m_tmpFile.clear();
            m_file.close();
            if ( m_sendSocket )
            {
                disconnect(m_sendSocket, 0, 0, 0);
                m_sendSocket->close();
                m_sendSocket = 0;                         // the instance will be deleted automatically by its parent
            }
            if ( m_serverSocket )
            {
                m_serverSocket->close();
                m_serverSocket = 0;                       // the instance will be deleted automatically by its parent

                if (Preferences::self()->dccUPnP())
                {
                    UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                    if (router) router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                }
            }
            Transfer::cleanUp();
        }

        void TransferSend::setFileURL( const KUrl& url )
        {
            if ( getStatus() == Configuring )
                m_fileURL = url;
        }

        void TransferSend::setFileName( const QString& fileName )
        {
            if ( getStatus() == Configuring )
                m_fileName = fileName;
        }

        void TransferSend::setOwnIp( const QString& ownIp )
        {
            if ( getStatus() == Configuring )
                m_ownIp = ownIp;
        }

        void TransferSend::setFileSize( KIO::filesize_t fileSize )
        {
            if ( getStatus() == Configuring )
                m_fileSize = fileSize;
        }

        void TransferSend::setReverse( bool reverse )
        {
            if ( getStatus() == Configuring )
                m_reverse = reverse;
        }

        bool TransferSend::queue()
        {
            kDebug();

            if ( getStatus() != Configuring )
                return false;

            if ( m_ownIp.isEmpty() )
            {
                m_ownIp = DccCommon::getOwnIp( Application::instance()->getConnectionManager()->getServerByConnectionId( m_connectionId ) );
            }

            if ( !KAuthorized::authorizeKAction( "allow_downloading" ) )
            {
                //Do not have the rights to send the file.  Shouldn't have gotten this far anyway
                //Note this is after the initialisation so the view looks correct still
                failed(i18n("The admin has restricted the right to send files"));
                return false;
            }

            if ( m_fileName.isEmpty() )
                m_fileName = sanitizeFileName( m_fileURL.fileName() );

            if ( Preferences::self()->dccIPv4Fallback() )
            {
                QHostAddress ip(m_ownIp);
                if (ip.protocol() == QAbstractSocket::IPv6Protocol)
                {
        #ifndef Q_WS_WIN
                    /* This is fucking ugly but there is no KDE way to do this yet :| -cartman */
                    struct ifreq ifr;
                    const QByteArray addressBa = Preferences::self()->dccIPv4FallbackIface().toAscii();
                    const char* address = addressBa.constData();
                    int sock = socket(AF_INET, SOCK_DGRAM, 0);
                    strncpy( ifr.ifr_name, address, IF_NAMESIZE );
                    ifr.ifr_addr.sa_family = AF_INET;
                    if ( ioctl( sock, SIOCGIFADDR, &ifr ) >= 0 )
                        m_ownIp =  inet_ntoa( ( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr );
                    kDebug() << "Falling back to IPv4 address " << m_ownIp;
        #endif
                }
            }

            m_fastSend = Preferences::self()->dccFastSend();
            kDebug() << "Fast DCC send: " << m_fastSend;

            //Check the file exists
            if ( !KIO::NetAccess::exists( m_fileURL, KIO::NetAccess::SourceSide, NULL ) )
            {
                failed( i18n( "The url \"%1\" does not exist", m_fileURL.prettyUrl() ) );
                return false;
            }

            //FIXME: KIO::NetAccess::download() is a synchronous function. we should use KIO::get() instead.
            //Download the file.  Does nothing if it's local (file:/)
            if ( !KIO::NetAccess::download( m_fileURL, m_tmpFile, NULL ) )
            {
                failed( i18n( "Could not retrieve \"%1\"", m_fileURL.prettyUrl() ) );
                kDebug() << "KIO::NetAccess::download() failed. reason: " << KIO::NetAccess::lastErrorString();
                return false;
            }

            //Some protocols, like http, maybe not return a filename, and altFileName may be empty, So prompt the user for one.
            if ( m_fileName.isEmpty() )
            {
                bool pressedOk;
                m_fileName = KInputDialog::getText( i18n( "Enter Filename" ), i18n( "<qt>The file that you are sending to <i>%1</i> does not have a filename.<br>Please enter a filename to be presented to the receiver, or cancel the dcc transfer</qt>", getPartnerNick() ), "unknown", &pressedOk, NULL );

                if ( !pressedOk )
                {
                    failed( i18n( "No filename was given" ) );
                    return false;
                }
            }

            //FIXME: if "\\\"" works well on other IRC clients, replace "\"" with "\\\""
            m_fileName.replace( '\"', '_' );
            if (Preferences::self()->dccSpaceToUnderscore())
                m_fileName.replace( ' ', '_' );

            kDebug() << "m_tmpFile: " << m_tmpFile;
            m_file.setFileName( m_tmpFile );

            if ( m_fileSize == 0 )
            {
                m_fileSize = m_file.size();
                kDebug() << "filesize 0, new filesize: " << m_fileSize;
                if ( m_fileSize == 0 )
                    failed( i18n( "Unable to send a 0 byte file." ) );
            }

            return Transfer::queue();
        }

        void TransferSend::reject()
        {
            kDebug();

            failed( i18n( "DCC SEND request was rejected" ) );
        }

        void TransferSend::abort()                     // public slot
        {
            kDebug();

            cleanUp();
            setStatus( Aborted );
            emit done( this );
        }

        void TransferSend::start()                     // public slot
        {
            kDebug();

            if ( getStatus() != Queued )
                return;

            // common procedure

            Server* server = Application::instance()->getConnectionManager()->getServerByConnectionId( m_connectionId );
            if ( !server )
            {
                kDebug() << "could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed( i18n( "Could not send a DCC SEND request to the partner via the IRC server." ) );
                return;
            }

            if ( !m_reverse )
            {
                // Normal DCC SEND
                kDebug() << "normal DCC SEND";

                // Set up server socket
                QString failedReason;
                if ( Preferences::self()->dccSpecificSendPorts() )
                    m_serverSocket = DccCommon::createServerSocketAndListen( this, &failedReason, Preferences::self()->dccSendPortsFirst(), Preferences::self()->dccSendPortsLast() );
                else
                    m_serverSocket = DccCommon::createServerSocketAndListen( this, &failedReason );

                if ( !m_serverSocket )
                {
                    failed( failedReason );
                    return;
                }

                connect( m_serverSocket, SIGNAL( newConnection() ),   this, SLOT( acceptClient() ) );

                // Get own port number
                m_ownPort = m_serverSocket->serverPort();

                kDebug() << "Own Address=" << m_ownIp << ":" << m_ownPort;

                if (Preferences::self()->dccUPnP())
                {
                    UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();

                    if (router && router->forward(QHostAddress(server->getOwnIpByNetworkInterface()), m_ownPort, QAbstractSocket::TcpSocket))
                        connect(router, SIGNAL( forwardComplete(bool, quint16 ) ), this, SLOT ( sendRequest(bool, quint16) ) );
                    else
                        sendRequest(true, 0); // Just try w/o UPnP on failure
                }
                else
                {
                    sendRequest(false, 0);
                }
            }
            else
            {
                // Passive DCC SEND
                kDebug() << "Passive DCC SEND";

                int tokenNumber = Application::instance()->getDccTransferManager()->generateReverseTokenNumber();
                // TODO: should we append a letter "T" to this token?
                m_reverseToken = QString::number( tokenNumber );

                kDebug() << "Passive DCC key(token): " << m_reverseToken;

                startConnectionTimer( Preferences::self()->dccSendTimeout() );

                server->dccPassiveSendRequest( m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp( m_ownIp ), m_fileSize, m_reverseToken );
            }

            setStatus( WaitingRemote, i18n( "Awaiting remote user's acceptance" ) );
        }

        void TransferSend::sendRequest(bool /* error */, quint16 port)
        {
            Server* server = Application::instance()->getConnectionManager()->getServerByConnectionId( m_connectionId );
            if ( !server )
            {
                kDebug() << "could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed( i18n( "Could not send a DCC SEND request to the partner via the IRC server." ) );
                return;
            }

            if (Preferences::self()->dccUPnP() && this->sender())
            {
                if (port != m_ownPort) return; // Somebody elses forward succeeded

                disconnect (this->sender(), SIGNAL( forwardComplete(bool, quint16 ) ), this, SLOT ( sendRequest(bool, quint16) ) );
            }

            startConnectionTimer( Preferences::self()->dccSendTimeout() );

            server->dccSendRequest( m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp( m_ownIp ), m_ownPort, m_fileSize );
        }

        void TransferSend::connectToReceiver( const QString& partnerHost, uint partnerPort )
        {
            kDebug();
            // Reverse DCC

            startConnectionTimer( Preferences::self()->dccSendTimeout() );

            m_partnerIp = partnerHost;
            m_partnerPort = partnerPort;

            m_sendSocket = new QTcpSocket( this );

            connect( m_sendSocket, SIGNAL( connected( ) ), this, SLOT( startSending() ) );
            connect( m_sendSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( slotGotSocketError( QAbstractSocket::SocketError ) ) );

            setStatus( Connecting );

            m_sendSocket->connectToHost( partnerHost, partnerPort );
        }
                                                          // public
        bool TransferSend::setResume( quint64 position )
        {
            kDebug() << "Position=" << position;

            if ( getStatus() > WaitingRemote )
                return false;

            if ( position >= m_fileSize )
                return false;

            m_resumed = true;
            m_transferringPosition = position;
            return true;
        }

        void TransferSend::acceptClient()                     // slot
        {
            // Normal DCC

            kDebug();

            stopConnectionTimer();

            m_sendSocket = m_serverSocket->nextPendingConnection();
            if ( !m_sendSocket )
            {
                failed( i18n( "Could not accept the connection (socket error.)" ) );
                return;
            }
            connect( m_sendSocket, SIGNAL( error ( QAbstractSocket::SocketError ) ), this, SLOT( slotGotSocketError( QAbstractSocket::SocketError ) ));

            // we don't need ServerSocket anymore
            m_serverSocket->close();
            m_serverSocket = 0; // the instance will be deleted automatically by its parent

            if (Preferences::self()->dccUPnP())
            {
                UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                if (router) router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
            }

            startSending();
        }

        void TransferSend::startSending()
        {
            stopConnectionTimer();

            if ( m_fastSend )
                connect( m_sendSocket, SIGNAL( bytesWritten( qint64 ) ), this, SLOT( bytesWritten( qint64 ) ) );
            connect( m_sendSocket, SIGNAL( readyRead() ),  this, SLOT( getAck() ) );

            m_partnerIp = m_sendSocket->peerAddress().toString();
            m_partnerPort = m_sendSocket->peerPort();
            m_ownPort = m_sendSocket->localPort();

            if ( m_file.open( QIODevice::ReadOnly ) )
            {
                // seek to file position to make resume work
                m_file.seek( m_transferringPosition );
                m_transferStartPosition = m_transferringPosition;

                writeData();
                startTransferLogger();                      // initialize CPS counter, ETA counter, etc...
                setStatus( Transferring );
            }
            else
            {
                failed( getQFileErrorString( m_file.error() ) );
            }
        }

        void TransferSend::bytesWritten(qint64 /*bytes*/)
        {
            //wait for all remaining bytes to be written
            if (m_sendSocket && m_sendSocket->bytesToWrite() <= (qint64)m_bufferSize)
            {
                writeData();
            }
        }

        void TransferSend::writeData()                 // slot
        {
            //kDebug();

            qint64 actual = m_file.read( m_buffer, m_bufferSize );
            if ( actual > 0 )
            {
                qint64 byteWritten = m_sendSocket->write( m_buffer, actual );

                if (byteWritten > 0)
                {
                    m_transferringPosition += byteWritten;
                    //m_transferringPosition += actual;
                    if ( (KIO::fileoffset_t)m_fileSize <= m_transferringPosition )
                    {
                        Q_ASSERT( (KIO::fileoffset_t)m_fileSize == m_transferringPosition );
                        kDebug() << "Done.";
                    }
                }
            }
        }

        void TransferSend::getAck()                    // slot
        {
            //kDebug();
            if ( m_transferringPosition < (KIO::fileoffset_t)m_fileSize )
            {
                writeData();
            }

            quint32 pos;
            while ( m_sendSocket->bytesAvailable() >= 4 )
            {
                m_sendSocket->read( (char*)&pos, 4 );
                pos = intel( pos );

                //kDebug() << pos  << "/" << m_fileSize;
                if ( pos == m_fileSize )
                {
                    kDebug() << "Received final ACK.";
                    cleanUp();
                    setStatus( Done );
                    emit done( this );
                    break;                                // for safe
                }
            }
        }

        void TransferSend::slotGotSocketError( QAbstractSocket::SocketError errorCode )
        {
            stopConnectionTimer();
            kDebug() << "code =  " << errorCode << " string = " << m_sendSocket->errorString();
            failed( i18n( "Socket error: %1", m_sendSocket->errorString() ) );
        }

        void TransferSend::startConnectionTimer( int sec )
        {
            kDebug();
            //start also restarts, no need for us to double check it
            m_connectionTimer->start(sec*1000);
        }

        void TransferSend::stopConnectionTimer()
        {
            if ( m_connectionTimer->isActive() )
            {
                kDebug() << "stop";
                m_connectionTimer->stop();
            }
        }

        void TransferSend::slotConnectionTimeout()         // slot
        {
            kDebug();
            failed( i18n( "Timed out" ) );
        }
                                                          // protected, static
        QString TransferSend::getQFileErrorString( int code )
        {
            QString errorString;

            switch(code)
            {
                case QFile::NoError:
                    errorString=i18n("The operation was successful. Should never happen in an error dialog.");
                    break;
                case QFile::ReadError:
                    errorString=i18n("Could not read from file \"%1\".",m_fileName );
                    break;
                case QFile::WriteError:
                    errorString=i18n("Could not write to file \"%1\".", m_fileName );
                    break;
                case QFile::FatalError:
                    errorString=i18n("A fatal unrecoverable error occurred.");
                    break;
                case QFile::OpenError:
                    errorString=i18n("Could not open file \"%1\".", m_fileName );
                    break;

                    // Same case value? Damn!
                    //        case IO_ConnectError:
                    //          errorString="Could not connect to the device.";
                    //        break;

                case QFile::AbortError:
                    errorString=i18n("The operation was unexpectedly aborted.");
                    break;
                case QFile::TimeOutError:
                    errorString=i18n("The operation timed out.");
                    break;
                case QFile::UnspecifiedError:
                    errorString=i18n("An unspecified error happened on close.");
                    break;
                default:
                    errorString=i18n("Unknown error. Code %1",code);
                    break;
            }

            return errorString;
        }

    }
}

#include "transfersend.moc"
