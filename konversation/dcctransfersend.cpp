/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

// dcctransfersend.cpp : separated from dcctransfer.cpp
/*
  dcctransfer.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kserversocket.h>
#include <ksocketaddress.h>
#include <kstreamsocket.h>

#include "dcctransfersend.h"
#include "konversationapplication.h"

DccTransferSend::DccTransferSend(KListView* _parent, const QString& _partnerNick, const QString& _filePath, const QString& _ownIp)
  : DccTransfer(_parent, DccTransfer::Send, _partnerNick)
{
  kdDebug() << "DccTransferSend::DccTransferSend()" << endl
            << "DccTransferSend::DccTransferSend(): Partner=" << _partnerNick << endl
            << "DccTransferSend::DccTransferSend(): File=" << _filePath << endl;
  
  filePath=_filePath;
  ownIp=_ownIp;
  
  file.setName(filePath);
  fileName=filePath.section("/",-1);
  fileSize=file.size();
  
  connectionTimer=0;
  
  serverSocket=0;
  sendSocket=0;
  
  updateView();
}

DccTransferSend::~DccTransferSend()
{
  cleanUp();
}

void DccTransferSend::start()  // public slot
{
  kdDebug() << "DccTransferSend::start()" << endl;

  // Set up server socket
  serverSocket = new KNetwork::KServerSocket();
  serverSocket->setFamily(KNetwork::KResolver::InetFamily);
  
  if(KonversationApplication::preferences.getDccSpecificSendPorts())  // user specifies ports
  {
    // set port
    bool found = false;  // wheter succeeded to set port
    unsigned long port = KonversationApplication::preferences.getDccSendPortsFirst();
    for( ; port <= KonversationApplication::preferences.getDccSendPortsLast(); ++port )
    {
      kdDebug() << "DccTransferSend::start(): trying port " << port << endl;
      serverSocket->setAddress("0.0.0.0", QString::number(port));
      bool success = serverSocket->listen();
      if( found = ( success && serverSocket->error() == KNetwork::KSocketBase::NoError ) )
        break;
      serverSocket->close();
    }
    if(!found)
    {
      KMessageBox::sorry(0, i18n("There is no vacant port for DCC sending."));
      setStatus(Failed);
      updateView();
      cleanUp();
      return;
    }
  }
  else  // user doesn't specify ports
  {
    if(!serverSocket->listen())
    {
      kdDebug() << this << "DccTransferSend::start(): listen() failed!" << endl;
      setStatus(Failed);
      updateView();
      cleanUp();
      return;
    }
  }
    
  connect( serverSocket, SIGNAL(readyAccept()), this, SLOT(heard()) );
  connect( serverSocket, SIGNAL(gotError(int)), this, SLOT(socketError(int)) );
  
  // Get our own port number
  KNetwork::KSocketAddress ipAddr=serverSocket->localAddress();
  const struct sockaddr_in* socketAddress=(sockaddr_in*)ipAddr.address();
  ownPort = QString::number(ntohs(socketAddress->sin_port));
  
  kdDebug() << "DccTransferSend::start(): own Address=" << ownIp << ":" << ownPort << endl;
  
  setStatus(WaitingRemote, i18n("Waiting remote user's acceptance"));
  updateView();
  
  startConnectionTimer(90);  // wait for 90 sec
  
  emit sendReady(partnerNick,fileName,getNumericalIpText(ownIp),ownPort,fileSize);
}

void DccTransferSend::abort()  // public slot
{
  kdDebug() << "DccTransferSend::abort()" << endl;
  
  setStatus(Aborted);
  cleanUp();
  updateView();
}

void DccTransferSend::setResume(unsigned long _position)  // public
{
  kdDebug() << "DccTransferSend::setResume( position=" << _position << " )" << endl;
  
  bResumed = true;
  transferringPosition = _position;
  
  updateView();
}

void DccTransferSend::cleanUp()
{
  kdDebug() << "DccTransferSend::cleanUp()" << endl;
  
  stopConnectionTimer();
  stopAutoUpdateView();
  if(sendSocket)
  {
    sendSocket->close();
    sendSocket->deleteLater();
    sendSocket = 0;
  }
  if(serverSocket)
  {
    serverSocket->close();
    serverSocket->deleteLater();
    serverSocket = 0;
  }
  file.close();
}

void DccTransferSend::heard()  // slot
{
  kdDebug() << "DccTransferSend::heard()" << endl;
  
  stopConnectionTimer();
  
  sendSocket = static_cast<KNetwork::KStreamSocket*>(serverSocket->accept());
  
  connect( sendSocket, SIGNAL(readyRead()),  this, SLOT(getAck()) );
  connect( sendSocket, SIGNAL(readyWrite()), this, SLOT(writeData()) );
  
  timeTransferStarted = QDateTime::currentDateTime();
  
  if(file.open(IO_ReadOnly))
  {
    // seek to file position to make resume work
    file.at(transferringPosition);
    transferStartPosition = transferringPosition;
    
    setStatus(Sending);
    sendSocket->enableRead(true);
    sendSocket->enableWrite(true);
    startAutoUpdateView();
  }
  else
  {
    QString errorString=getErrorString(file.status());
    KMessageBox::sorry(0, QString(errorString).arg(file.name()), i18n("DCC Send Error"));
    setStatus(Failed);
    cleanUp();
  }
  updateView();
}

void DccTransferSend::writeData()  // slot
{
  int actual=file.readBlock(buffer,bufferSize);
  if(actual>0)
  {
    sendSocket->writeBlock(buffer,actual);
    transferringPosition += actual;
  }
}

void DccTransferSend::getAck()  // slot
{
  unsigned long pos;
  while(sendSocket->bytesAvailable() >= 4)
  {
    sendSocket->readBlock((char *) &pos,4);
    pos=intel(pos);
    if(pos == fileSize)
    {
      kdDebug() << "DccTransferSend::getAck(): Done." << endl;
      
      setStatus(Done);
      cleanUp();
      updateView();
      emit done(filePath);
      break;  // for safe
    }
  }
}

void DccTransferSend::socketError(int errorCode)
{
  kdDebug() << "DccTransferSend::socketError(): code =  " << errorCode << endl;
  kdDebug() << "DccTransferSend::socketError(): string = " << serverSocket->errorString() << endl;

  setStatus(Failed);
  updateView();
  cleanUp();
}

void DccTransferSend::startConnectionTimer(int sec)
{
  stopConnectionTimer();
  connectionTimer = new QTimer();
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(connectionTimeout()));
  connectionTimer->start(sec*1000, TRUE);
}

void DccTransferSend::stopConnectionTimer()
{
  if(connectionTimer)
  {
    connectionTimer->stop();
    delete connectionTimer;
    connectionTimer = 0;
  }
}

void DccTransferSend::connectionTimeout()  // slot
{
  kdDebug() << "DccTransferSend::connectionTimeout()" << endl;
  
  setStatus(Failed, i18n("Timed out"));
  updateView();
  cleanUp();
}

#include "dcctransfersend.moc"
