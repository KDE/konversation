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

#include "dcctransfer.h"

class QTimer;
class KExtendedSocket;

class DccTransferRecv : public DccTransfer
{
  Q_OBJECT
  friend class DccResumeDialog;
  
  public:
    DccTransferRecv(KListView* _parent, const QString& _partnerNick, const QString& _fileFolder, const QString& _fileName, unsigned long _fileSize, const QString& _partnerIp, const QString& _partnerPort);
    virtual ~DccTransferRecv();
    
  signals:
    void resumeRequest(const QString&,const QString&,const QString&,int);  // emitted by requestResume()
    
  public slots:
    virtual void start();
    virtual void abort();
    void startResume(unsigned long _position);
    
  protected slots:
    void lookupFinished(int numOfResults);
    void connectionSuccess();
    void connectionFailed(int errorCode);
    void readData();
    void sendAck();
    void connectionTimeout();
    
  protected:
    void requestResume();
    void connectToSender();
    void cleanUp();
    void startConnectionTimer(int sec);
    void stopConnectionTimer();
    
    virtual void setFilePath(const QString& _filePath);
    
  protected:
    QString fileTmpPath;
    QTimer* connectionTimer;
    KExtendedSocket* recvSocket;
    bool bTemporaryFileExists;
    bool bCompletedFileExists;
};

#endif // DCCTRANSFERRECV_H
