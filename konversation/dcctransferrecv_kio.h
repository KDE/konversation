/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

// dcctransferrecv.h : separated from dcctransfer.h
/*
  dcctransfer.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef DCCTRANSFERRECV_H
#define DCCTRANSFERRECV_H

#include <qptrlist.h>
#include <qthread.h>

#include "dcctransfer.h"

class QFile;
class QTimer;

namespace KIO
{
  class TransferJob;
}

namespace KNetwork
{
  class KStreamSocket;
}

class DccTransferRecvWriteCacheHandler;

class DccTransferRecv : public DccTransfer
{
  Q_OBJECT
  friend class DccDetailDialog;
  friend class DccResumeDialog;
  
  public:
    DccTransferRecv(KListView* _parent, const QString& _partnerNick, const KURL& _defaultFolderURL, const QString& _fileName, unsigned long _fileSize, const QString& _partnerIp, const QString& _partnerPort);
    virtual ~DccTransferRecv();
    
  signals:
    void resumeRequest(const QString&,const QString&,const QString&,int);  // emitted by requestResume()
    
  public slots:
    virtual void start();
    virtual void abort();
    void startResume(unsigned long _position);
    
  protected slots:
    void connectionSuccess();
    void connectionFailed(int errorCode);
    void readData();
    void sendAck();
    void connectionTimeout();
    void writeDone();
    void gotWriteError(int errorCode);
    
  protected:
    void requestResume();
    void connectToSender();
    void cleanUp();
    void startConnectionTimer(int sec);
    void stopConnectionTimer();
    
    void setLocalFileURL(const KURL& _url);
    
  protected:
    KURL localTmpFileURL;
    unsigned long localTmpFileSize;
    DccTransferRecvWriteCacheHandler* writeCacheHandler;
    bool bTemporaryFileExists;
    bool bCompletedFileExists;
    
    QTimer* connectionTimer;
    KNetwork::KStreamSocket* recvSocket;
};

class DccTransferRecvWriteCacheHandler : public QObject, protected QThread
{
  Q_OBJECT
  
  public:
    DccTransferRecvWriteCacheHandler(KIO::TransferJob* _transferJob);
    virtual ~DccTransferRecvWriteCacheHandler();
    
    void start();
    void addCache(QByteArray* newCache);
    void finish();
    void finishNow();
    
  signals:
    void dataFinished();           // will connect with transferJob->slotFinished()
    void done();                   // will connect with DccTransferRecv::writeDone()
    void gotError(int errorCode);  // will connect with DccTransferRecv::slotWriteError()
    
  protected slots:
    void slotKIOWriteReady();  // will connect with transferJob->dataReq()
    void slotKIOResult();      // will connect with transferJob->result()
    
  protected:
    virtual void run();
    
    KIO::TransferJob* transferJob;
    QPtrList<QByteArray> cache;
    bool bKIOWriteReady;
    bool bFinishing;
};

#endif // DCCTRANSFERRECV_H
