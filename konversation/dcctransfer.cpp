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

#include <qtimer.h>
#include <qhostaddress.h>

#include <klocale.h>
#include <kdebug.h>

#include "dcctransfer.h"
#include "konversationapplication.h"

DccTransfer::DccTransfer(KListView* parent,DccType type,QString folder,QString partner,QString name,QString size,QString ipString,QString portString) :
             KListViewItem(parent)
{
  kdDebug() << "DccTransfer::DccTransfer()" << endl;

  dccSocket=0;
  
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
  statusText.append(i18n("Running"));
  statusText.append(i18n("Stalled"));
  statusText.append(i18n("Failed"));
  statusText.append(i18n("Aborted"));
  statusText.append(i18n("Done"));

  transferStarted=QDateTime::currentDateTime();
  lastActive=QDateTime::currentDateTime();

  updateCPS();

  if(getType()==Get)
  {
  }
  else
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

void DccTransfer::startGet()
{
  // Append folder with partner's name if wanted
  if(KonversationApplication::preferences.getDccCreateFolder())
    dir.setPath(dccFolder+"/"+dccPartner.lower());
  else
    dir.setPath(dccFolder);

  if(!dir.exists())
  {
    // QT's mkdir() is too stupid to do this alone, so we use the shell command
    system(QString("mkdir -p "+dir.path()).latin1());
  }

  QString fullName(dccFile);
  // Append partner's name to file name if wanted
  if(KonversationApplication::preferences.getDccAddPartner()) fullName=dccPartner.lower()+"."+fullName;
  file.setName(dir.path()+"/"+fullName);

  if(file.exists())
  {
    file.open(IO_ReadOnly);
    long fileSize=file.size();
    file.close();
    // If the file is empty we can forget about resuming
    if(fileSize)
    {
      // TODO: Ask user if they want to resume
      setType(Resume);
      setStatus(Resuming);
      // Rollback for Resume
      fileSize-=KonversationApplication::preferences.getDccRollback();
      if(fileSize<0) fileSize=0;
      setPosition(fileSize);

      emit resume(getPartner(),getFile(),getPort(),getPosition());
    }
    else connectToSender();
  }
  else connectToSender();
}

void DccTransfer::startSend()
{
  // Set up server socket
  dccSocket=new KExtendedSocket();
  // Listen on all available interfaces
  dccSocket->setHost("0.0.0.0");
  dccSocket->setSocketFlags(KExtendedSocket::passiveSocket |
                            KExtendedSocket::inetSocket |
                            KExtendedSocket::streamSocket);

  if(dccSocket->listen(5)==0)
  {
    // FIXME: This seems to be a laugh but it works ...
    setPort(dccSocket->localAddress()->pretty().section(' ',1,1));
    connect(dccSocket,SIGNAL (readyAccept()),this,SLOT(heard()) );

    file.setName(getFile());

    emit send(getPartner(),getFile(),getNumericalIp(),getPort(),getSize());
  }
  else kdDebug() << "DccTransfer::startSend(): listen() failed!" << endl;
}

void DccTransfer::heard()
{
  kdDebug() << "DccTransfer::heard(): accepting ..." << endl;

  int fail=dccSocket->accept(sendSocket);

  connect(sendSocket,SIGNAL (readyRead()),this,SLOT (getAck()) );
  connect(sendSocket,SIGNAL (readyWrite()),this,SLOT (writeData()) );

  kdDebug() << "DccTransfer::heard(): accept() returned " << fail << endl;

  if(!fail)
  {
    file.open(IO_ReadOnly);
    setStatus(Running);
    sendSocket->enableRead(true);
    sendSocket->enableWrite(true);
  }
  else
  {
    setStatus(Failed);
    dccSocket->close();
  }
}

void DccTransfer::startResume(QString position)
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

  kdDebug() << "Socket created." << endl;

  connect(dccSocket,SIGNAL (lookupFinished(int))  ,this,SLOT (lookupFinished(int)) );
  connect(dccSocket,SIGNAL (connectionSuccess())  ,this,SLOT (dccGetConnectionSuccess()) );
  connect(dccSocket,SIGNAL (connectionFailed(int)),this,SLOT (dccGetBroken(int)) );

  connect(dccSocket,SIGNAL (readyRead()),this,SLOT (readData()) );
  connect(dccSocket,SIGNAL (readyWrite()),this,SLOT (sendAck()) );

  kdDebug() << "Lookup ..." << endl;
  dccSocket->startAsyncConnect();
}

void DccTransfer::lookupFinished(int numOfResults)
{
  kdDebug() << "Lookup finished. Connecting ..." << endl;
  setStatus(Connecting);
  numOfResults=0; // suppress compiler warning
}

void DccTransfer::dccGetConnectionSuccess()
{
  kdDebug() << "Connected! Starting transfer ..." << endl;
  setStatus(Running);
  dccSocket->enableRead(true);

  file.open(IO_ReadWrite);
  // Set position
  file.at(getPosition());
  // for DCC Resume
  if (getType()==Resume) sendAck();
}

void DccTransfer::dccGetBroken(int errNo)
{
  kdDebug() << "DccTransfer: Error " << errNo << endl;

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

QString DccTransfer::getPort() { return dccPort; }

void DccTransfer::setBufferSize(unsigned long size) { bufferSize=size; }
unsigned long DccTransfer::getBufferSize() { return bufferSize; }

void DccTransfer::setFolder(QString folder) { dccFolder=folder; }
QString DccTransfer::getFolder() { return dccFolder; }
