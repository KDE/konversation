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

#include <qtimer.h>

#include <klocale.h>

#include "dcctransfer.h"

DccTransfer::DccTransfer(KListView* parent,DccType type,QString partner,QString name) :
             KListViewItem(parent)
{
  setType(type);

  statusText.append(i18n("Queued"));
  statusText.append(i18n("Connecting ..."));
  statusText.append(i18n("Offering ..."));
  statusText.append(i18n("Running"));
  statusText.append(i18n("Stalled"));
  statusText.append(i18n("Failed"));
  statusText.append(i18n("Done"));

  transferStarted=QDateTime::currentDateTime();
  lastActive=QDateTime::currentDateTime();

  setPartner(partner);
  setFile(name);
  setSize(67588);
  setPosition(0);
  updateCPS();

  if(type==Get) setStatus(Queued);
  else setStatus(Offering);

  QTimer* dummyTimer=new QTimer();
  QTimer* dummyTimer2=new QTimer();
  connect(dummyTimer,SIGNAL(timeout()),this,SLOT(updateCPS()));
  connect(dummyTimer2,SIGNAL(timeout()),this,SLOT(increase()));

  dummyTimer->start(1000);
  dummyTimer2->start(200);
}

DccTransfer::~DccTransfer()
{
}

void DccTransfer::setStatus(DccStatus status)
{
  dccStatus=status;
  setText(6,statusText[status]);
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
  setText(4,QString::number((int) (transferred*100.0/fileSize))+"%");
}

void DccTransfer::setPartner(QString partner)
{
  dccPartner=partner;
  setText(0,dccPartner);
}

void DccTransfer::setFile(QString file)
{
  dccFile=file;
  setText(1,dccFile);
}

void DccTransfer::updateCPS()
{
  int elapsed=transferStarted.secsTo(QDateTime::currentDateTime());
  /* prevent division by zero */
  int cps=(elapsed) ? transferred/elapsed : 0;
  setText(5,QString::number(cps));
}

void DccTransfer::increase()
{
  setPosition(getPosition()+300);
}

DccTransfer::DccStatus DccTransfer::getStatus() { return dccStatus; }
unsigned long DccTransfer::getPosition() { return transferred; }
unsigned long DccTransfer::getSize() { return fileSize; }

void DccTransfer::setType(DccType type) { dccType=type; }
DccTransfer::DccType DccTransfer::getType() { return dccType; }
