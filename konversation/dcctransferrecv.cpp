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
#include <kextsock.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include "dccresumedialog.h"
#include "dcctransferrecv.h"
#include "konversationapplication.h"

DccTransferRecv::DccTransferRecv(KListView* _parent, const QString& _partnerNick, const QString& _fileFolder, const QString& _fileName, unsigned long _fileSize, const QString& _partnerIp, const QString& _partnerPort)
  : DccTransfer(_parent, DccTransfer::Receive, _partnerNick)
{
  kdDebug() << "DccTransferRecv::DccTransferRecv()" << endl
            << "DccTransferRecv::DccTransferRecv(): Partner=" << _partnerNick << endl
            << "DccTransferRecv::DccTransferRecv(): File=" << _fileName << endl
            << "DccTransferRecv::DccTransferRecv(): FileSize=" << _fileSize << endl
            << "DccTransferRecv::DccTransferRecv(): Partner Address=" << _partnerIp << ":" << _partnerPort << endl;
  
  fileName=_fileName;
  
  // set default filename
  // Append partner's name to file name if wanted
  QString localFileName;
  if(KonversationApplication::preferences.getDccAddPartner())
    localFileName=_partnerNick.lower().lower()+"."+_fileName;
  else
    localFileName=_fileName;
  
  // set default path
  // Append folder with partner's name if wanted
  if(KonversationApplication::preferences.getDccCreateFolder())
    filePath=_fileFolder+"/"+_partnerNick.lower()+"/"+localFileName;
  else
    filePath=_fileFolder+"/"+localFileName;
  
  fileTmpPath=filePath+".part";
  file.setName(fileTmpPath);
  
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
  
  if(!QDir(filePath.section("/",0,-2)).exists())
    if(!KStandardDirs::makeDir(filePath.section("/",0,-2)))
    {
      KMessageBox::sorry(static_cast<QWidget*>(0),i18n("Cannot create received files directory '%1'.").arg(filePath.section("/",0,-2)),i18n("DCC Error"));
      setStatus(Failed);
      cleanUp();
      updateView();
    }
        
  // check whether the file exists
  // if exists, ask user to rename/overwrite/abort
  if(QFile::exists(filePath))
  {
    kdDebug() << "DccTransferRecv::start(): File " << fileTmpPath << " exists." << endl;
    
    
      
  }
  // check whether the temporary file exists
  // if exists, ask user to rename/resume/overwrite/abort
  else if(file.exists() && file.size()>0)
  {
    kdDebug() << "DccTransferRecv::start(): Temporary File " << fileTmpPath << " exists." << endl;
    if(KonversationApplication::preferences.getDccAutoResume())
      requestResume();
    else
    {
      QString newName(filePath.section("/",-1));
      int userRes = DccResumeDialog::ask(0,newName,filePath.section("/",0,-2));
      switch(userRes)
      {
        case KDialogBase::User1:
          kdDebug() << 1 << endl;
          filePath=filePath.section("/",0,-2)+"/"+newName;
          fileTmpPath=filePath+".part";
          file.setName(fileTmpPath);
          // fall through ...

        case KDialogBase::Ok:
          kdDebug() << 2 << endl;
          requestResume();
          break;
          
        case KDialogBase::User2:
          kdDebug() << 3 << endl;
          // just overwrite the old file
          connectToSender();
          break;
        // finally this can only mean that the user has clicked "cancel"
        default:
          abort();
      }
    }
  }
  // not having completed file, nor part file
  else
    connectToSender();
}

void DccTransferRecv::startResume(unsigned long _position)  // public slot
{
  kdDebug() << "DccTransferRecv::startResume( position=" << _position << " )" << endl;
  
  stopConnectionTimer();
  
  // TODO: we'd better check if _position is equals to the requested position (transferringPosition)
  transferringPosition = _position;
  
  updateView();
  
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
    disconnect(recvSocket, 0, 0, 0);
    recvSocket->close();
    delete recvSocket;
    recvSocket = 0;
  }
  file.close();
}

void DccTransferRecv::requestResume()
{
  kdDebug() << "DccTransferRecv::requestResume()" << endl;
  
  bResumed = true;
  setStatus(WaitingRemote);
  
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
  
  setStatus(LookingUp);
  
  recvSocket=new KExtendedSocket(partnerIp,partnerPort.toUInt(),KExtendedSocket::inetSocket);
  
  recvSocket->enableRead(false);
  recvSocket->enableWrite(false);
  recvSocket->setTimeout(5);
  
  connect(recvSocket,SIGNAL (lookupFinished(int))  ,this,SLOT (lookupFinished(int)) );
  connect(recvSocket,SIGNAL (connectionSuccess())  ,this,SLOT (connectionSuccess()) );
  connect(recvSocket,SIGNAL (connectionFailed(int)),this,SLOT (connectionFailed(int)));
  
  connect(recvSocket,SIGNAL (readyRead()),this,SLOT (readData()) );
  connect(recvSocket,SIGNAL (readyWrite()),this,SLOT (sendAck()) );
  
  recvSocket->startAsyncConnect();
}

void DccTransferRecv::lookupFinished(int /* numOfResults */)  // slot
{
  setStatus(Connecting);
}

void DccTransferRecv::connectionSuccess()  // slot
{
  kdDebug() << "DccTransferRecv::connectionSuccess()" << endl;
  
  timeTransferStarted = QDateTime::currentDateTime();
  
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
  kdDebug() << "DccTransferRecv::connectionFailed(): Error " << errorCode << endl;

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
    cleanUp();
    
    // replace the file
    if(QFile::exists(filePath))
      QFile::remove(filePath);  // already confirmed to overwrite
    
    QDir().rename(fileTmpPath, filePath);
    
    setStatus(Done);
    updateView();
    emit done(fileName);
  }
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
  
  setStatus(Failed);
  updateView();
  cleanUp();
}

#include "dcctransferrecv.moc"
