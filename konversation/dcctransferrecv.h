// dcctransferrecv.h - receive a file on DCC protocol
// Copyright (C) 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
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

class DccPanel;

class DccTransferRecvWriteCacheHandler;

class DccTransferRecv : public DccTransfer
{
  Q_OBJECT
  friend class DccDetailDialog;
  friend class DccResumeDialog;
  
  public:
    /** Constructor.  This sets up the variables and updates the view, so the
     * user can see the filename, filesize etc, and can accept it. */
    DccTransferRecv( DccPanel* panel, const QString& partnerNick, const KURL& defaultFolderURL, const QString& fileName, unsigned long fileSize, const QString& partnerIp, const QString& partnerPort );
    virtual ~DccTransferRecv();
    
    /** 
     * This function reads the member variables set in the constructor, and
     * calls saveToFileURL() based on these, and konversation's preferences.
     * It may not call saveToFileURL(), may not give it a valid url,
     * and may set it to empty.
     * Checking saveToFileURL isn't done until the user accepts the dcc
     * and start() is called, which calls validateSaveToFileURL.
     * @see validateSaveToFileURL()
     */
    void calculateSaveToFileURL();
    /**
     * This checks saveToFileURL, and if it's empty, asks the user for a new
     * directory to save to.
     * If the directory doesn't exist, it attempts to create it.
     * This function will either return with saveToFileURL set to a valid,
     * existing directory, or will call abort() and return
     * @see abort()
     */
    void validateSaveToFileURL();
	
  signals:
    void resumeRequest( const QString& partnerNick, const QString& fileName, const QString& partnerPort, KIO::filesize_t filePosition);  // emitted by requestResume()
    
  public slots:
    /** The user has accepted the download.
     *  Check we are saving it somewhere valid, create any directories needed, and
     *  connect to remote host.
     */
    virtual void start();
    /** The user has chosen to abort.
     *  Either by chosen to abort directly, or by chosing cancel when
     *  prompted for information on where to save etc.
     *  Not called when it fails due to another problem.
     */
    virtual void abort();
    void startResume( unsigned long position );
    
  protected slots:
    void connectionSuccess();
    void connectionFailed( int errorCode );
    void readData();
    void sendAck();
    void connectionTimeout();
    void writeDone();
    void gotWriteError( int errorCode );
    
  protected:
    void requestResume();
    void connectToSender();
    void cleanUp();
    void startConnectionTimer( int sec );
    void stopConnectionTimer();
    
    void setSaveToFileURL( const KURL& url );
    
  protected:
    KURL m_saveToTmpFileURL;
    ///Current filesize of the file saved on the disk.
    KIO::filesize_t m_saveToFileSize;
    ///Current filesize of the file+".part" saved on the disk.
    KIO::filesize_t m_partialFileSize;
    DccTransferRecvWriteCacheHandler* m_writeCacheHandler;
    bool m_saveToFileExists;
    bool m_partialFileExists;
    KURL m_defaultFolderURL;
    QTimer* m_connectionTimer;
    KNetwork::KStreamSocket* m_recvSocket;
};

class DccTransferRecvWriteCacheHandler : public QObject
{
  Q_OBJECT
  
  public:
    DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob );
    virtual ~DccTransferRecvWriteCacheHandler();
    
    void append( QByteArray* cache );
    bool write( bool force = false );
    void close();
    void closeNow();
    
  signals:
    void dataFinished();             // will connect with transferJob->slotFinished()
    void done();                     // will connect with DccTransferRecv::writeDone()
    void gotError( int errorCode );  // will connect with DccTransferRecv::slotWriteError()
    
  protected slots:
    void slotKIODataReq( KIO::Job*, QByteArray& data );  // will connect with transferJob->dataReq()
    void slotKIOResult();  // will connect with transferJob->result()
    
  protected:
    unsigned long allCacheSize();
    QByteArray* popCache();
    
    KIO::TransferJob* m_transferJob;
    bool m_writeAsyncMode;
    bool m_writeReady;
    
    QPtrList<QByteArray> m_cacheList;
};

#endif // DCCTRANSFERRECV_H
