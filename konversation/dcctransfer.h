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
#include <qstringlist.h>

/*
  @author Dario Abatianni
*/

class DccTransfer : public QObject, public KListViewItem
{
  Q_OBJECT

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
      Offering,    /* DCC-SEND, waiting for receiver to connect to us */
      Running,     /* Transferring data */
      Stalled,     /* Transfer stalls */
      Failed,      /* Transfer failed */
      Done         /* Transfer done */
    };

    DccTransfer(KListView* parent,DccType type,QString partner,QString name,QString size,QString ipString,QString portString);
    ~DccTransfer();

  protected slots:
    void updateCPS();
    void increase();

  protected:
    void setType(DccType type);
    DccType getType();

    void setStatus(DccStatus status);
    DccStatus getStatus();

    void setSize(unsigned long size);
    unsigned long getSize();

    void setPosition(unsigned long pos);
    unsigned long getPosition();

    void setPartner(QString partner);
    void setFile(QString file);

    DccType dccType;
    DccStatus dccStatus;
    QStringList statusText;
    QString dccPartner;
    QString dccFile;

    unsigned long fileSize;
    unsigned long transferred;

    KSocket* dccSocket;

    QDateTime transferStarted;
    QDateTime lastActive;         /* To find out if the transfer stalled */

};

#endif
