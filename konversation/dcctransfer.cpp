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

#include "dcctransfer.h"

DccTransfer::DccTransfer(KListView* parent,DccType type,QString partner,QString name) :
             KListViewItem(parent)
{
  setType(type);
}

DccTransfer::~DccTransfer()
{
}

/* Accessors */

void DccTransfer::setType(DccType type) { dccType=type; }
DccTransfer::DccType DccTransfer::getType() { return dccType; }

void DccTransfer::setStatus(DccStatus status) { dccStatus=status; }
DccTransfer::DccStatus DccTransfer::getStatus() { return dccStatus; }
