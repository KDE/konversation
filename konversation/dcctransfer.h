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
      ResumeSend,
      ResumeGet
    };

    enum DccStatus
    {
      Queued=0,    // Newly added DCC
      Resuming,    // DCC-GET, trying to negotiate resume position
      Lookup,      // DCC-GET, trying to find ip address of sender
      Connecting,  // DCC-GET, trying to connect to sender
      Offering,    // DCC-SEND, waiting for receiver to connect to us
      Sending,     // Sending data
      Receiving,   // Receiving data
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
    QString getNumericalIp();
    QString getPartner();
    QString getFile();
    QString getFolder();
    unsigned long getBufferSize();

  signals:
    void send(QString partner,QString fileName,QString ip,QString port,unsigned long size);
    void resumeGet(QString partner,QString fileName,QString port,int startAt);
    void dccGetDone(QString fileName);
    void dccSendDone(QString fileName);

  public slots:
    void startGet();
    void startSend();
    void startResumeGet(QString position);
    void startResumeSend(QString position);
    void abort();

  protected slots:
    void updateCPS();
    void lookupFinished(int numOfResults);
    void dccGetConnectionSuccess();
    void dccGetBroken(int errorCode);
    void readData();
    void writeData();
    void getAck();
    void sendAck();
/*
    void dccSendConnectionSuccess();
    void dccSendBroken(int errorCode);
*/
    void heard();

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
    KExtendedSocket* sendSocket;  // accept() needs a new socket
    QDir dir;
    QFile file;

    QDateTime transferStarted;
    QDateTime lastActive;         // To find out if the transfer stalled

    char* buffer;
};

#endif
