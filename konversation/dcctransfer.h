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
*/

#ifndef DCCTRANSFER_H
#define DCCTRANSFER_H

#include <qdatetime.h>

#include <klistview.h>
#include <kurl.h>
#include <kio/global.h>

class QDateTime;
class QStringList;
class QTimer;

class KProgress;

namespace KIO
{
  class Job;
}

class DccDetailDialog;
class DccPanel;

/*
  @author Dario Abatianni
*/

class DccTransfer : public QObject, public KListViewItem
{
  Q_OBJECT
  friend class DccDetailDialog;
  
  public:
    enum DccType
    {
      Send,
      Receive,
      DccTypeCount
    };
    
    enum DccStatus
    {
      Queued = 0,    // Newly added DCC, RECV: Waiting for local user's response
      WaitingRemote, // Waiting for remote host's response
      Connecting,    // RECV: trying to connect to the server
      Sending,       // Sending
      Receiving,     // Receiving
      Done,          // Transfer done
      Failed,        // Transfer failed
      Aborted,       // Transfer aborted by user
      Removed,       // The file was removed
      DccStatusCount
    };
    
    DccTransfer( DccPanel* panel, DccType dccType, const QString& partnerNick, const QString& fileName );
    virtual ~DccTransfer();
    
    virtual void paintCell(QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment);
    
    DccType         getType()           const;
    DccStatus       getStatus()         const;
    QString         getOwnIp()          const;
    QString         getOwnPort()        const;
    QString         getPartnerNick()    const;
    QString         getFileName()       const;
    KIO::filesize_t getFileSize()       const;
	
    KURL getFileURL() const;
    bool isResumed() const;
    
    void runFile();
    void removeFile();
    void openFileInfoDialog();
    
    void openDetailDialog();
    void closeDetailDialog();
    
  signals:
    void done( const QString& filename );
    void statusChanged( const DccTransfer* item );
    
  public slots:
    virtual void start() = 0;
    virtual void abort() = 0;
    
  protected slots:
    void updateView();
    
    void slotRemoveFileDone( KIO::Job* job );
    
  protected:
    void showProgressBar();  // called from printCell()
    
    void startAutoUpdateView();
    void stopAutoUpdateView();
    
    void setStatus( DccStatus status, const QString& statusDetail = QString::null );
    
    // called from updateView()
    QString         getTypeText()                                   const;
    QPixmap         getTypeIcon()                                   const;
    QPixmap         getStatusIcon()                                 const;
    QString         getStatusText()                                 const;
    QString         getFileSizePrettyText()                         const;
    int             getProgress()                                   const;
    QString         getPositionPrettyText( bool detailed = false )  const;
    QString         getTimeRemainingPrettyText()                    const;
    QString         getCPSPrettyText()                              const;
    unsigned long   getCPS()                                        const;
    
    static QString getNumericalIpText( const QString& ipString );
    static QString getErrorString( int code );
    static unsigned long intel( unsigned long value );
    static QString getPrettyNumberText( const QString& numberText );
    
    // transfer information
    DccType m_dccType;
    DccStatus m_dccStatus;
    QString m_dccStatusDetail;
    bool m_resumed;
    KIO::fileoffset_t m_transferringPosition;
    KIO::fileoffset_t m_transferStartPosition;
    QString m_partnerNick;
    QString m_partnerIp;  // null when unknown
    QString m_partnerPort;
    QString m_ownIp;
    QString m_ownPort;
    QDateTime m_timeOffer;
    QDateTime m_timeTransferStarted;
    QDateTime m_timeLastActive;
    
    unsigned long m_bufferSize;
    char* m_buffer;
    
    // file information
    QString m_fileName;
    /* The file size of the complete file sending/recieving. */
    KIO::filesize_t  m_fileSize;
    /* If we are sending a file, this is the url of the file we are sending.
     * If we are recieving a file, this is the url of the file we are saving
     * to in the end (Temporararily it will be filename+".part" ).
     */
    KURL m_fileURL;
    
    // UI
    QTimer* m_autoUpdateViewTimer;
    KProgress* m_progressBar;
    DccDetailDialog* m_detailDialog;
    
    static QString s_dccTypeText[ DccTypeCount ];
    static QString s_dccStatusText[ DccStatusCount ];
    
};

#endif
