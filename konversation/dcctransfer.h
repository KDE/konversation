/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dcctransfer.h  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef DCCTRANSFER_H
#define DCCTRANSFER_H

#include <klistview.h>
#include <ksock.h>

#include <qdatetime.h>

/*
  @author Dario Abatianni
*/

class DccTransfer : public KListViewItem
{
  public:
    enum DccType
    {
      Send=0,
      Get
    };

    enum DccStatus
    {
      Queued=0,    /* Newly added DCC */
      Connecting,  /* DCC-GET, trying to connect to sender */
      Waiting,     /* DCC-SEND, waiting for receiver to connect to us */
      Running,     /* Transferring data */
      Stalled,     /* Transfer stalls */
      Failed,      /* Transfer failed */
      Done         /* Transfer done */
    };

    DccTransfer(KListView* parent,DccType type,QString partner,QString name);
    ~DccTransfer();

  protected:
    void setType(DccType type);
    DccType getType();

    void setStatus(DccStatus status);
    DccStatus getStatus();

    DccType dccType;
    DccStatus dccStatus;

    QString fileName;

    long fileSize;
    long transferred;

    KSocket* dccPartner;
    int port;

    QDateTime transferStarted;
    QDateTime lastActive;         /* To find out if the transfer stalled */

};

#endif
