/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dcctransfer.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <stdlib.h>
#include <netinet/in.h>

#include <qtimer.h>
#include <qhostaddress.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdebug.h>
#include <kextsock.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include "dcctransfer.h"
#include "konversationapplication.h"

DccTransfer::DccTransfer(KListView* parent,DccType type,QString folder,QString partner,QString name,QString size,QString ipString,QString portString) :
             KListViewItem(parent)
{
  kdDebug() << "DccTransfer::DccTransfer()" << endl;

  dccSocket=0;
  sendSocket=0;
  
  setType(type);
  setPartner(partner);
  setFile(name);
  setSize(size.toUInt());
  setPosition(0);
  setIp(ipString);
  setPort(portString);
  setStatus(Queued);
  setFolder(folder);

  setBufferSize(KonversationApplication::preferences.getDccBufferSize());
  buffer=new char[bufferSize];

  statusText.append(i18n("Queued"));
  statusText.append(i18n("Negotiating Resume ..."));
  statusText.append(i18n("Lookup ..."));
  statusText.append(i18n("Connecting ..."));
  statusText.append(i18n("Offering ..."));
  statusText.append(i18n("Sending"));
  statusText.append(i18n("Receiving"));
  statusText.append(i18n("Stalled"));
  statusText.append(i18n("Failed"));
  statusText.append(i18n("Aborted"));
  statusText.append(i18n("Done"));

  transferStarted=QDateTime::currentDateTime();

  updateCPS();

  if(getType()==Send)
  {
    setText(6,i18n("unknown"));
    setText(7,i18n("unknown"));
    setStatus(Offering);
  }
}

DccTransfer::~DccTransfer()
{
  kdDebug() << "DccTransfer::~DccTransfer(" << getFile() << ")" << endl;

  delete[] buffer;

  if(dccSocket)
  {
    dccSocket->close();
    delete dccSocket;
  }
}

void DccTransfer::abort()
{
  if(dccSocket)
  {
    dccSocket->closeNow();
    delete dccSocket;
    dccSocket=0;
  }
  if(sendSocket)
  {
    sendSocket->closeNow();
    delete sendSocket;
    sendSocket=0;
  }

  setStatus(Aborted);
}

void DccTransfer::startGet()
{
  // Append folder with partner's name if wanted
  if(KonversationApplication::preferences.getDccCreateFolder())
    dir.setPath(dccFolder+"/"+dccPartner.lower());
  else
    dir.setPath(dccFolder);

  if(!dir.exists())
  {
    if(!KStandardDirs::makeDir(dir.path()))
      KMessageBox::sorry(static_cast<QWidget*>(0),i18n("Cannot create received files directory %1").arg(dir.path()),i18n("DCC error"));
  }

  QString fullName(dccFile);
  // Append partner's name to file name if wanted
  if(KonversationApplication::preferences.getDccAddPartner()) fullName=dccPartner.lower()+"."+fullName;
  file.setName(dir.path()+"/"+fullName);

  if(file.exists())
  {
    long fileSize=0;

    if(file.open(IO_ReadOnly))
    {
      fileSize=file.size();
      file.close();
    }

    setPosition(fileSize);

    int doResume=KMessageBox::questionYesNoCancel
                 (
                   0,
                   i18n("<qt>The file \"%1\" already exists. Do you want to resume the transfer</qt>").arg(getFile()),
                   i18n("Resume transfer"),
                   i18n("Resume"),
                   i18n("Overwrite"),
                   "ResumeTransfer"
                 );

    // If the file is empty we can forget about resuming
    if(fileSize && doResume==KMessageBox::Yes)
    {
      setType(ResumeGet);
      setStatus(Resuming);
      // Rollback for Resume
      fileSize-=KonversationApplication::preferences.getDccRollback();
      if(fileSize<0) fileSize=0;
      setPosition(fileSize);

      emit resumeGet(getPartner(),getFile(),getPort(),getPosition());
    }
    else
    {
      setPosition(0);
      // just overwrite the old file
      if(doResume==KMessageBox::No) connectToSender();
      // or abort
      else abort();
    }
  }
  else connectToSender();
}

void DccTransfer::startSend()
{
  // Set up server socket
  dccSocket=new KExtendedSocket();
  // Listen on all available interfaces
  dccSocket->setHost("0.0.0.0");

  /*
reset()
setPort(x)
listen()
until port found
*/

  dccSocket->setSocketFlags(KExtendedSocket::passiveSocket |
                            KExtendedSocket::inetSocket |
                            KExtendedSocket::streamSocket);

  if(dccSocket->listen(5)==0)
  {
    // Get our own port number
    const KSocketAddress* ipAddr=dccSocket->localAddress();
    const struct sockaddr_in* socketAddress=(sockaddr_in*)ipAddr->address();

//    kdDebug() << ipAddr->pretty() << " " << ntohs(socketAddress->sin_port) << endl;
    setPort(QString::number(ntohs(socketAddress->sin_port)));

    connect(dccSocket,SIGNAL (readyAccept()),this,SLOT(heard()) );

    file.setName(getFile());

    emit send(getPartner(),getFile(),getNumericalIp(),getPort(),getSize());
  }
  else kdDebug() << this << "DccTransfer::startSend(): listen() failed!" << endl;
}

void DccTransfer::startResumeSend(QString position)
{
  setType(ResumeSend);
  setStatus(Resuming);
  setPosition(position.toULong());
  kdDebug() << this << "DccTransfer::startResumeSend(" << position << ")" << endl;
}

void DccTransfer::heard()
{
//  kdDebug() << this << "DccTransfer::heard(): accepting ..." << endl;

  int fail=dccSocket->accept(sendSocket);

  connect(sendSocket,SIGNAL (readyRead()),this,SLOT (getAck()) );
  connect(sendSocket,SIGNAL (readyWrite()),this,SLOT (writeData()) );
/*
  kdDebug() << this << "DccTransfer::heard(): accept() returned " << fail << endl;
  kdDebug() << this << "DccTransfer::heard(): getType() returns " << getType() << endl;
  kdDebug() << this << "DccTransfer::heard(): getPosition() returns " << getPosition() << endl;
*/
  if(!fail)
  {
    if(file.open(IO_ReadOnly))
    {
      // seek to file position to make resume work
      file.at(getPosition());
      setStatus(Sending);
      sendSocket->enableRead(true);
      sendSocket->enableWrite(true);
    }
    else
    {
      QString errorString=getErrorString(file.status());

      KMessageBox::sorry(0,QString(errorString).arg(file.name()),i18n("DCC Send error"));
      setStatus(Failed);
    }
  }
  else
  {
    setStatus(Failed);
    dccSocket->close();
  }
}

void DccTransfer::startResumeGet(QString position)
{
  setPosition(position.toULong());
  connectToSender();
}

void DccTransfer::connectToSender()
{
  setStatus(Lookup);

  dccSocket=new KExtendedSocket(getIp(),getPort().toUInt(),KExtendedSocket::inetSocket);

  dccSocket->enableRead(false);
  dccSocket->enableWrite(false);
  dccSocket->setTimeout(5);

  connect(dccSocket,SIGNAL (lookupFinished(int))  ,this,SLOT (lookupFinished(int)) );
  connect(dccSocket,SIGNAL (connectionSuccess())  ,this,SLOT (dccGetConnectionSuccess()) );
  connect(dccSocket,SIGNAL (connectionFailed(int)),this,SLOT (dccGetBroken(int)) );

  connect(dccSocket,SIGNAL (readyRead()),this,SLOT (readData()) );
  connect(dccSocket,SIGNAL (readyWrite()),this,SLOT (sendAck()) );

  dccSocket->startAsyncConnect();
}

void DccTransfer::lookupFinished(int /* numOfResults */)
{
  setStatus(Connecting);
}

void DccTransfer::dccGetConnectionSuccess()
{
  if(file.open(IO_ReadWrite))
  {
    // Set position for DCC Resume Get
    file.at(getPosition());

    setStatus(Receiving);
    dccSocket->enableRead(true);

//   if (getType()==ResumeGet) sendAck(); // seems not to work ...
  }
  else
  {
    QString errorString=getErrorString(file.status());

    KMessageBox::sorry (0,"<qt>"+QString(errorString).arg(file.name())+"</qt>",i18n("DCC Get error"));
    setStatus(Failed);
  }
}

void DccTransfer::dccGetBroken(int errNo)
{
  kdDebug() << this << "DccTransfer: Error " << errNo << endl;

  setStatus(Failed);
  file.close();
}

void DccTransfer::readData()
{
  int actual=dccSocket->readBlock(buffer,bufferSize);
  if(actual>0)
  {
    setPosition(getPosition()+actual);
    dccSocket->enableWrite(true);
    file.writeBlock(buffer,actual);
    updateCPS();
  }
}

void DccTransfer::writeData()
{
  int actual=file.readBlock(buffer,bufferSize);
  if(actual>0)
  {
    sendSocket->enableWrite(true);
    sendSocket->writeBlock(buffer,actual);
    setPosition(getPosition()+actual);
  }
  updateCPS();
}

unsigned long intel(unsigned long value)
{
  value=((value & 0xff000000) >> 24) +
        ((value & 0xff0000) >> 8) +
        ((value & 0xff00) << 8) +
        ((value & 0xff) << 24);

  return value;
}

void DccTransfer::getAck()
{
  unsigned long pos;
  sendSocket->readBlock((char *) &pos,4);
  pos=intel(pos);

  if(pos==getSize())
  {
    setStatus(Done);
    sendSocket->close();
    file.close();
    emit dccSendDone(getFile());
  }
}

void DccTransfer::sendAck()
{
  unsigned long pos=intel(getPosition());

  dccSocket->enableWrite(false);
  dccSocket->writeBlock((char*) &pos,4);

  if(getPosition()==getSize())
  {
    setStatus(Done);
    dccSocket->close();
    file.close();
    emit dccGetDone(getFile());
  }
}

void DccTransfer::setStatus(DccStatus status)
{
  dccStatus=status;
  setText(8,statusText[status]);
}

void DccTransfer::setSize(unsigned long size)
{
  fileSize=size;
  setText(2,QString::number(fileSize));
}

void DccTransfer::setPosition(unsigned long pos)
{
  transferred=pos;
  setText(3,QString::number(transferred));
  // some clients fail to send the file size
  if(fileSize)
    setText(4,QString::number((int) (transferred*100.0/fileSize))+"%");
  else
    setText(4,i18n("unknown"));
}

void DccTransfer::setPartner(QString partner)
{
  dccPartner=partner;
  setText(0,dccPartner);
}

QString DccTransfer::getPartner()
{
  return dccPartner;
}

void DccTransfer::setFile(QString file)
{
  dccFile=file;
  setText(1,dccFile);
}

QString DccTransfer::getFile()
{
  return dccFile;
}

void DccTransfer::updateCPS()
{
  int elapsed=transferStarted.secsTo(QDateTime::currentDateTime());
  // prevent division by zero
  int cps=(elapsed) ? transferred/elapsed : 0;
  setText(5,QString::number(cps));
  lastActive=QDateTime::currentDateTime();
}

DccTransfer::DccStatus DccTransfer::getStatus() { return dccStatus; }
unsigned long DccTransfer::getPosition() { return transferred; }
unsigned long DccTransfer::getSize() { return fileSize; }

void DccTransfer::setType(DccType type) { dccType=type; }
DccTransfer::DccType DccTransfer::getType() { return dccType; }

void DccTransfer::setIp(QString ip)
{
  dccIp=ip;
  setText(6,ip);
}

QString DccTransfer::getIp() { return dccIp; }

QString DccTransfer::getNumericalIp()
{
  QHostAddress ip;
  ip.setAddress(getIp());

  return QString::number(ip.ip4Addr());
}

void DccTransfer::setPort(QString port)
{
  dccPort=port;
  setText(7,port);
}

QString DccTransfer::getErrorString(int code)
{
  QString errorString(QString::null);

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

QString DccTransfer::getPort() { return dccPort; }
QString DccTransfer::getFullPath() { return file.name(); }

void DccTransfer::setBufferSize(unsigned long size) { bufferSize=size; }
unsigned long DccTransfer::getBufferSize() { return bufferSize; }

void DccTransfer::setFolder(QString folder) { dccFolder=folder; }
QString DccTransfer::getFolder() { return dccFolder; }

#include "dcctransfer.moc"
