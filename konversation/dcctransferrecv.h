// dcctransferrecv.h - receive a file on DCC protocol
/*
  dcctransfer.h  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
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
    // Local KIO
    void slotLocalCanResume( KIO::Job* job, KIO::filesize_t size );
    void slotLocalGotResult( KIO::Job* job );
    void slotLocalReady( KIO::Job* job );
    void slotLocalWriteDone();
    void slotLocalGotWriteError( const QString& errorString );
    
    // Remote DCC
    void connectionSuccess();
    void connectionFailed( int errorCode );
    void readData();
    void sendAck();
    void connectionTimeout();
    void slotSocketClosed();
    
  protected:
     /** 
     * This function reads the member variables set in the constructor, and
     * calls @ref saveToFileURL() based on these, and konversation's preferences.
     * It may not call @ref saveToFileURL(), may not give it a valid url,
     * and may set it to empty.
     * Checking @ref saveToFileURL isn't done until the user accepts the dcc
     * and @ref start() is called, which calls validateSaveToFileURL.
     * @param folderURL The directory url to save in.  The ircnick is added if needed.
     * @see validateSaveToFileURL()
     */
    void calculateSaveToFileURL( const KURL& defaultFolderURL );
    
    void prepareLocalKio( bool overwrite, KIO::fileoffset_t startPosition );  // (startPosition == 0) means "don't resume"
    
    /**
     * This calls KIO::NetAccess::mkdir on all the subdirectories of dirURL, to
     * create the given directory.  Note that a url like  file:/foo/bar  will
     * make sure both foo and bar are created.  It assumes everything in the path is
     * a directory.  
     * Note: If the directory already exists, returns true.
     * 
     * @param A url for the directory to create.
     * @return True if the directory now exists.  False if there was a problem and the directory doesn't exist.
     */
    bool createDirs(const KURL &dirURL) const;
    
    void requestResume();
    void connectToSender();
    
    void cleanUp();
    
    void startConnectionTimer( int sec );
    void stopConnectionTimer();
    
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
    DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob );
    virtual ~DccTransferRecvWriteCacheHandler();
    
    void append( QByteArray cache );
    bool write( bool force );
    void close();
    void closeNow();
    
  signals:
    void dataFinished();                          // ->  m_transferJob->slotFinished()
    void done();                                  // ->  DccTransferRecv::writeDone()
    void gotError( const QString& errorString );  // ->  DccTransferRecv::slotWriteError()
    
  protected slots:
    void slotKIODataReq( KIO::Job* job, QByteArray& data );  // <-  m_transferJob->dataReq()
    void slotKIOResult( KIO::Job* job );                     // <-  m_transferJob->result()
    
  protected:
    QByteArray popCache();
    
    KIO::TransferJob* m_transferJob;
    bool m_writeAsyncMode;
    bool m_writeReady;
    
    QValueList<QByteArray> m_cacheList;
    unsigned long m_wholeCacheSize;
};

#endif // DCCTRANSFERRECV_H

