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
      Get,
      Resume
    };

    enum DccStatus
    {
      Queued=0,    // Newly added DCC
      Resuming,    // DCC-GET, trying to negotiate resume position
      Lookup,      // DCC-GET, trying to find ip address of sender
      Connecting,  // DCC-GET, trying to connect to sender
      Offering,    // DCC-SEND, waiting for receiver to connect to us
      Running,     // Transferring data
      Stalled,     // Transfer stalls
      Failed,      // Transfer failed
      Aborted,     // Transfer aborted by user
      Done         // Transfer done
    };

    DccType getType();
    DccStatus getStatus();

    DccTransfer(KListView* parent,DccType type,QString folder,QString partner,QString name,QString size,QString ipString,QString portString);
    ~DccTransfer();

    QString getPort();
    unsigned long getSize();
    unsigned long getPosition();
    QString getIp();
    QString getPartner();
    QString getFile();
    QString getFolder();
    unsigned long getBufferSize();

  signals:
    void resume(QString partner,QString fileName,QString port,int startAt);

  public slots:
    void startGet();
    void startResume(QString position);

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
    void connectToSender();

    void setType(DccType type);
    void setStatus(DccStatus status);
    void setSize(unsigned long size);
    void setPosition(unsigned long pos);
    void setIp(QString ip);
    void setPort(QString port);
    void setPartner(QString partner);
    void setFile(QString file);
    void setFolder(QString folder);
    void setBufferSize(unsigned long size);

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
