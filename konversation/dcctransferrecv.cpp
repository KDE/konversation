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
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstreamsocket.h>

#include "dccresumedialog.h"
#include "dcctransferrecv.h"
#include "konversationapplication.h"

DccTransferRecv::DccTransferRecv(KListView* _parent, const QString& _partnerNick, const KURL& _defaultFolderURL, const QString& _fileName, unsigned long _fileSize, const QString& _partnerIp, const QString& _partnerPort)
  : DccTransfer(_parent, DccTransfer::Receive, _partnerNick, _fileName)
{
  kdDebug() << "DccTransferRecv::DccTransferRecv()" << endl
            << "DccTransferRecv::DccTransferRecv(): Partner=" << _partnerNick << endl
            << "DccTransferRecv::DccTransferRecv(): File=" << _fileName << endl
            << "DccTransferRecv::DccTransferRecv(): FileSize=" << _fileSize << endl
            << "DccTransferRecv::DccTransferRecv(): Partner Address=" << _partnerIp << ":" << _partnerPort << endl;
  
  // set default path
  // Append folder with partner's name if wanted
  KURL localFileURLTmp(_defaultFolderURL);
  localFileURLTmp.adjustPath(1);  // add a slash if there is none
  if(KonversationApplication::preferences.getDccCreateFolder())
    localFileURLTmp.addPath(_partnerNick.lower()+"/");
  
  // determine default filename
  // Append partner's name to file name if wanted
  if(KonversationApplication::preferences.getDccAddPartner())
    localFileURLTmp.addPath(_partnerNick.lower()+"."+_fileName);
  else
    localFileURLTmp.addPath(_fileName);
  
  setLocalFileURL(localFileURLTmp);
  
  fileSize=_fileSize;
  
  partnerIp=_partnerIp;
  partnerPort=_partnerPort;
  
  connectionTimer=0;
  
  recvSocket=0;
  
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
  bCompletedFileExists = QFile::exists(localFileURL.path());
  
  // check whether the temporary file exists
  // if exists, ask user to resume/rename/overwrite/abort
  bTemporaryFileExists = file.exists() && (0 < file.size());
  
  if(!bCompletedFileExists && bTemporaryFileExists && KonversationApplication::preferences.getDccAutoResume())
    requestResume();
  else if(bCompletedFileExists || bTemporaryFileExists)
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
  if(recvSocket)
  {
    recvSocket->close();
    recvSocket->deleteLater();
    recvSocket = 0;
  }
  file.close();
}

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  bResumed = true;
  setStatus(WaitingRemote, i18n("Requesting to accept resuming"));
  
  // Rollback for Resume
  unsigned long rb=KonversationApplication::preferences.getDccRollback();
  if(file.size()<rb)
    transferringPosition=0;
  else
    transferringPosition=file.size()-rb;
  
  updateView();
  
  startConnectionTimer(5);
  
  // FIXME: we should use ULong for position in file
  emit resumeRequest(partnerNick,fileName,partnerPort,(int)transferringPosition);
}

void DccTransferRecv::connectToSender()
{
  kdDebug() << "DccTransferRecv::connectToSender()" << endl;
  
  setStatus(Connecting);
  updateView();
  
  recvSocket=new KNetwork::KStreamSocket(partnerIp, partnerPort);
  
  recvSocket->setBlocking(false);  // asynchronous mode
  recvSocket->setFamily(KNetwork::KResolver::InetFamily);
  recvSocket->setResolutionEnabled(false);
  recvSocket->setTimeout(5000);
  
  recvSocket->enableRead(false);
  recvSocket->enableWrite(false);
  
  connect( recvSocket, SIGNAL( connected(const KResolverEntry&) ), this, SLOT( connectionSuccess() ) );
  connect( recvSocket, SIGNAL( gotError(int) ),                    this, SLOT( connectionFailed(int) ) );
  
  connect( recvSocket, SIGNAL( readyRead() ),  this, SLOT( readData() ) );
  connect( recvSocket, SIGNAL( readyWrite() ), this, SLOT( sendAck() ) );
  
  recvSocket->connect();
}

void DccTransferRecv::connectionSuccess()  // slot
{
  kdDebug() << "DccTransferRecv::connectionSuccess()" << endl;
  
  timeTransferStarted = QDateTime::currentDateTime();
  
  if(!QDir(localFileURL.directory()).exists())
    if(!KStandardDirs::makeDir(localFileURL.directory()))
    {
      KMessageBox::sorry(static_cast<QWidget*>(0),i18n("Cannot create received files directory '%1'.").arg(localFileURL.directory()),i18n("DCC Error"));
      setStatus(Failed);
      cleanUp();
      updateView();
    }
  if(file.open(IO_ReadWrite))
  {
    // Set position for DCC Resume Get
    file.at(transferringPosition);
    transferStartPosition = transferringPosition;
    
    setStatus(Receiving);
    recvSocket->enableRead(true);
    startAutoUpdateView();
  }
  else
  {
    QString errorString=getErrorString(file.status());

    KMessageBox::sorry (0,"<qt>"+QString(errorString).arg(file.name())+"</qt>",i18n("DCC Get Error"));
    setStatus(Failed);
    cleanUp();
  }
  updateView();
}

void DccTransferRecv::connectionFailed(int errorCode)  // slot
{
  kdDebug() << "DccTransferRecv::connectionFailed(): code = " << errorCode << endl;
  kdDebug() << "DccTransferRecv::connectionFailed(): string = " << recvSocket->errorString() << endl;
  
  setStatus(Failed);
  cleanUp();
  updateView();
}

void DccTransferRecv::readData()  // slot
{
  int actual=recvSocket->readBlock(buffer,bufferSize);
  if(actual>0)
  {
    transferringPosition += actual;
    recvSocket->enableWrite(true);
    file.writeBlock(buffer,actual);
  }
}

void DccTransferRecv::sendAck()  // slot
{
  unsigned long pos=intel(transferringPosition);

  recvSocket->enableWrite(false);
  recvSocket->writeBlock((char*) &pos,4);

  if(transferringPosition==fileSize)
  {
    kdDebug() << "DccTransferRecv::sendAck(): Done." << endl;
    
    cleanUp();
    
    // rename the file
    if(QFile::exists(localFileURL.path()))
      QFile::remove(localFileURL.path());  // already confirmed to overwrite
    QDir().rename(localTmpFileURL.path(), localFileURL.path());
    
    setStatus(Done);
    updateView();
    emit done(fileName);
  }
}

void DccTransferRecv::setLocalFileURL(const KURL& url)
{
  localFileURL = url;
  localTmpFileURL = KURL(url.url() + ".part");
  file.setName(localTmpFileURL.path());
}

void DccTransferRecv::startConnectionTimer(int sec)
{
  stopConnectionTimer();
  connectionTimer = new QTimer();
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(connectionTimeout()));
  connectionTimer->start(sec*1000, TRUE);
}

void DccTransferRecv::stopConnectionTimer()
{
  if(connectionTimer)
  {
    connectionTimer->stop();
    delete connectionTimer;
    connectionTimer = 0;
  }
}

void DccTransferRecv::connectionTimeout()  // slot
{
  kdDebug() << "DccTransferRecv::connectionTimeout()" << endl;
  
  setStatus(Failed, i18n("Timed out"));
  updateView();
  cleanUp();
}

#include "dcctransferrecv.moc"
