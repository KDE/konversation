// dcctransfersend.cpp - send a file on DCC protocol
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

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <qfile.h>
#include <qtimer.h>

#include <kdebug.h>
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

DccTransferSend::DccTransferSend( DccPanel* panel, const QString& partnerNick, const KURL& fileURL, const QString& ownIp )
  : DccTransfer( panel, DccTransfer::Send, partnerNick, fileURL.filename() )
{
  kdDebug() << "DccTransferSend::DccTransferSend()" << endl
            << "DccTransferSend::DccTransferSend(): Partner=" << partnerNick << endl
            << "DccTransferSend::DccTransferSend(): File=" << fileURL.prettyURL() << endl;
  
  m_fileURL = fileURL;
  m_ownIp = ownIp;
  
  m_fileName = m_fileURL.filename();
 
  m_serverSocket = 0;
  m_sendSocket = 0;
  m_connectionTimer = new QTimer( this );
  connect( m_connectionTimer, SIGNAL( timeout() ), this, SLOT( connectionTimeout() ) );
  //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted

  //Check the file exists 
  if( !KIO::NetAccess::exists( m_fileURL, true, listView() ) ) {
    KMessageBox::sorry(listView(), i18n("The url \"%1\" does not exist").arg( m_fileURL.prettyURL() ) );
    setStatus( Failed, i18n("The url \"%1\" does not exist").arg( m_fileURL.prettyURL() ) );
    updateView();
    cleanUp();
    return;
  }
  //Download the file.  Does nothing if it's local (file:/)
  if( !KIO::NetAccess::download( m_fileURL, m_tmpFile, listView() ) ) {
    KMessageBox::sorry(listView(), i18n("Could not retrieve \"%1\".").arg( m_fileURL.prettyURL() ) );
    setStatus( Failed, i18n("Could not retrieve \"%1\"").arg( m_fileURL.prettyURL() ) );
    updateView();
    cleanUp();
    return;
  }

  //Some protocols, like http, maybe not return a filename.  So prompt the user for one.
  if( m_fileName.isEmpty() ) {
    bool pressedOk;
    m_fileName = KInputDialog::getText( i18n("Enter filename"), i18n("<qt>The file that you are sending to <i>%1</i> does not have a filename.<br>Please enter a filename to be presented to the receiver, or cancel the dcc transfer</qt>").arg( getPartnerNick() ), "unknown", &pressedOk, listView() );
    if( !pressedOk ) {
      setStatus( Failed, i18n("Any filename weren't given") );
      updateView();
      cleanUp();
      return;    
    }
  }
  m_file.setName( m_tmpFile );
  m_fileSize = m_file.size();

  updateView();
  panel->selectMe( this );
}

DccTransferSend::~DccTransferSend()
{
  cleanUp();
}

void DccTransferSend::start()  // public slot
{
  kdDebug() << "DccTransferSend::start()" << endl;
  if( getStatus() != Queued )
    return; //setStatus(Failed) or something has been called.
  
  // Set up server socket
  m_serverSocket = new KNetwork::KServerSocket( this );
  m_serverSocket->setFamily( KNetwork::KResolver::InetFamily );
  
  if( KonversationApplication::preferences.getDccSpecificSendPorts() )  // user specifies ports
  {
    // set port
    bool found = false;  // wheter succeeded to set port
    unsigned long port = KonversationApplication::preferences.getDccSendPortsFirst();
    for( ; port <= KonversationApplication::preferences.getDccSendPortsLast(); ++port )
    {
      kdDebug() << "DccTransferSend::start(): trying port " << port << endl;
      m_serverSocket->setAddress( QString::number( port ) );
      bool success = m_serverSocket->listen();
      if( found = ( success && m_serverSocket->error() == KNetwork::KSocketBase::NoError ) )
        break;
      m_serverSocket->close();
    }
    if( !found )
    {
      KMessageBox::sorry( listView(), i18n("There is no vacant port for DCC sending.") );
      setStatus( Failed, i18n("No vacant port") );
      updateView();
      cleanUp();
      return;
    }
  }
  else  // user doesn't specify ports
  {
    // Let the operating system choose a port
    m_serverSocket->setAddress("0");
    
    if( !m_serverSocket->listen() )
    {
      kdDebug() << this << "DccTransferSend::start(): listen() failed!" << endl;
      setStatus( Failed );
      updateView();
      cleanUp();
      return;
    }
  }
    
  connect( m_serverSocket, SIGNAL( readyAccept() ),   this, SLOT( heard() )            );
  connect( m_serverSocket, SIGNAL( gotError( int ) ), this, SLOT( socketError( int ) ) );
  
  // Get our own port number
  KNetwork::KSocketAddress ipAddr = m_serverSocket->localAddress();
  const struct sockaddr_in* socketAddress = (sockaddr_in*)ipAddr.address();
  m_ownPort = QString::number( ntohs( socketAddress->sin_port ) );
  
  kdDebug() << "DccTransferSend::start(): own Address=" << m_ownIp << ":" << m_ownPort << endl;
  
  setStatus( WaitingRemote, i18n("Waiting remote user's acceptance") );
  updateView();
  
  startConnectionTimer( 90 );  // wait for 90 sec
  
  emit sendReady( m_partnerNick, m_fileName, getNumericalIpText( m_ownIp ), m_ownPort, m_fileSize );
}

void DccTransferSend::abort()  // public slot
{
  kdDebug() << "DccTransferSend::abort()" << endl;
  
  setStatus( Aborted );
  updateView();
  cleanUp();
  m_file.close();
}

void DccTransferSend::setResume( unsigned long position )  // public
{
  kdDebug() << "DccTransferSend::setResume( position=" << position << " )" << endl;
  
  m_resumed = true;
  m_transferringPosition = position;
  
  updateView();
}

void DccTransferSend::cleanUp()
{
  kdDebug() << "DccTransferSend::cleanUp()" << endl;
  stopConnectionTimer();
  finishTransferMeter();
  if( !m_tmpFile.isEmpty() )
    KIO::NetAccess::removeTempFile( m_tmpFile );
  m_tmpFile = QString::null;
  if( m_sendSocket )
  {
    m_sendSocket->close();
    m_sendSocket = 0;  // the instance will be deleted automatically by its parent
  }
  if( m_serverSocket )
  {
    m_serverSocket->close();
    m_serverSocket = 0;  // the instance will be deleted automatically by its parent
  }
}

void DccTransferSend::heard()  // slot
{
  kdDebug() << "DccTransferSend::heard()" << endl;
  
  stopConnectionTimer();
  
  m_sendSocket = static_cast<KNetwork::KStreamSocket*>( m_serverSocket->accept() );
  if(!m_sendSocket) {
    KMessageBox::sorry( listView(), QString( getErrorString( m_file.status() ) ).arg( m_file.name() ), i18n("DCC Send Error") );
    setStatus( Failed );
    cleanUp();
    return;
  }
  connect( m_sendSocket, SIGNAL( readyRead() ),  this, SLOT( getAck() )               );
  connect( m_sendSocket, SIGNAL( readyWrite() ), this, SLOT( writeData() )            );
  connect( m_sendSocket, SIGNAL( closed() ),     this, SLOT( slotSendSocketClosed() ) );
  
  // we don't need ServerSocket anymore
  m_serverSocket->close();
  
  if( m_file.open( IO_ReadOnly ) )
  {
    // seek to file position to make resume work
    m_file.at( m_transferringPosition );
    m_transferStartPosition = m_transferringPosition;
    
    setStatus( Sending );
    m_sendSocket->enableRead( true );
    m_sendSocket->enableWrite( true );
    initTransferMeter();  // initialize CPS counter, ETA counter, etc...
  }
  else
  {
    QString errorString = getErrorString( m_file.status() );
    KMessageBox::sorry( listView(), QString( errorString ).arg( m_file.name() ), i18n("DCC Send Error") );
    setStatus( Failed, i18n("File open failure") );
    cleanUp();
  }
  updateView();
}

void DccTransferSend::writeData()  // slot
{
  //kdDebug() << "writeData()" << endl;
  int actual = m_file.readBlock( m_buffer, m_bufferSize );
  if( actual > 0 )
  {
    m_sendSocket->writeBlock( m_buffer, actual );
    m_transferringPosition += actual;
  }
}

void DccTransferSend::getAck()  // slot
{
  //kdDebug() << "getAck()" << endl;
  unsigned long pos;
  while( m_sendSocket->bytesAvailable() >= 4 )
  {
    m_sendSocket->readBlock( (char*)&pos, 4 );
    pos = intel( pos );
    if( pos == m_fileSize )
    {
      kdDebug() << "DccTransferSend::getAck(): Done." << endl;
      
      setStatus( Done );
      cleanUp();
      updateView();
      emit done( m_fileURL.path() );
      break;  // for safe
    }
  }
}

void DccTransferSend::socketError( int errorCode )
{
  kdDebug() << "DccTransferSend::socketError(): code =  " << errorCode << " string = " << m_serverSocket->errorString() << endl;

  setStatus( Failed, i18n("Socket error: %1").arg( m_serverSocket->errorString() ));
  updateView();
  cleanUp();
}

void DccTransferSend::startConnectionTimer( int sec )
{
  kdDebug() << "DccTransferSend::startConnectionTimer"<< endl;
  Q_ASSERT(m_connectionTimer);  // mmm? what is it for?
  stopConnectionTimer();
  m_connectionTimer->start( sec*1000, TRUE );
}

void DccTransferSend::stopConnectionTimer()
{
  kdDebug() << "DccTransferSend::stopConnectionTimer" << endl;
  Q_ASSERT(m_connectionTimer);
  m_connectionTimer->stop();
}

void DccTransferSend::connectionTimeout()  // slot
{
  kdDebug() << "DccTransferSend::connectionTimeout()" << endl;
  
  setStatus( Failed, i18n("Timed out") );
  updateView();
  cleanUp();
}

void DccTransferSend::slotSendSocketClosed()
{
  if( m_dccStatus == Sending )
  {
    setStatus( Failed, i18n("Remote user disconnected") );
    updateView();
    cleanUp();
  }
}

#include "dcctransfersend.moc"
