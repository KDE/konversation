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

#include "dcctransfer.h"

class QFile;
class QTimer;

namespace KIO
{
  class Job;
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
    void resumeRequest(const QString&,const QString&,const QString&,KIO::filesize_t);  // emitted by requestResume()
    
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
    
    void setSaveToFileURL(const KURL& _url);
    
  protected:
    KURL m_saveToTmpFileURL;
    ///Current filesize of the file saved on the disk.
    KIO::filesize_t m_saveToFileSize;
    ///Current filesize of the file+".part" saved on the disk.
    KIO::filesize_t m_partialFileSize;
    DccTransferRecvWriteCacheHandler* m_writeCacheHandler;
    bool m_saveToFileExists;
    bool m_partialFileExists;
    
    QTimer* m_connectionTimer;
    KNetwork::KStreamSocket* m_recvSocket;
};

class DccTransferRecvWriteCacheHandler : public QObject
{
  Q_OBJECT
  
  public:
    DccTransferRecvWriteCacheHandler(KIO::TransferJob* transferJob);
    virtual ~DccTransferRecvWriteCacheHandler();
    
    void append( QByteArray* cache );
    bool write( bool force = false );
    void close();
    void closeNow();
    
  signals:
    void dataFinished();           // will connect with transferJob->slotFinished()
    void done();                   // will connect with DccTransferRecv::writeDone()
    void gotError(int errorCode);  // will connect with DccTransferRecv::slotWriteError()
    
  protected slots:
    void slotKIODataReq( KIO::Job*, QByteArray& data );  // will connect with transferJob->dataReq()
    void slotKIOResult();      // will connect with transferJob->result()
    
  protected:
    unsigned long allCacheSize();
    QByteArray* popCache();
    
    KIO::TransferJob* m_transferJob;
    bool m_writeAsyncMode;
    bool m_writeReady;
    
    QPtrList<QByteArray> m_cacheList;
};

#endif // DCCTRANSFERRECV_H
