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
#include <kextsock.h>

#include <qdir.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qstringlist.h>
#include <qthread.h>

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
      Queued=0,    // Newly added DCC
      Lookup,      // DCC-GET, trying to find ip address of sender
      Connecting,  // DCC-GET, trying to connect to sender
      Offering,    // DCC-SEND, waiting for receiver to connect to us
      Running,     // Transferring data
      Stalled,     // Transfer stalls
      Failed,      // Transfer failed
      Done         // Transfer done
    };

    DccType getType();
    DccStatus getStatus();

    DccTransfer(KListView* parent,DccType type,QString folder,QString partner,QString name,QString size,QString ipString,QString portString);
    ~DccTransfer();

  public slots:
    void startGet();

  protected slots:
    void updateCPS();
    void lookupFinished(int numOfResults);
    void connectionSuccess();
    void broken(int errorCode);
    void check();
    void readData();
    void writeData();
    void sendAck();
    
  protected:
    void setType(DccType type);
    void setStatus(DccStatus status);

    void setSize(unsigned long size);
    unsigned long getSize();

    void setPosition(unsigned long pos);
    unsigned long getPosition();

    void setIp(QString ip);
    QString getIp();

    void setPort(QString port);
    QString getPort();

    void setPartner(QString partner);
    QString getPartner();

    void setFile(QString file);
    QString getFile();

    void setFolder(QString folder);
    QString getFolder();

    void setBufferSize(unsigned long size);
    unsigned long getBufferSize();

    DccType dccType;
    DccStatus dccStatus;
    QStringList statusText;
    QString dccPartner;
    QString dccFolder;
    QString dccFile;
    QString dccIp;
    QString dccPort;

    unsigned long bufferSize;
    unsigned long fileSize;
    unsigned long transferred;

    KExtendedSocket* dccSocket;
    QDir dir;
    QFile file;

    QDateTime transferStarted;
    QDateTime lastActive;         // To find out if the transfer stalled

    char* buffer;
};

#endif
