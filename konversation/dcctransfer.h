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
#include <qfile.h>

#include <klistview.h>

class QDateTime;
class QStringList;
class QTimer;
class KProgress;

/*
  @author Dario Abatianni
*/

class DccTransfer : public QObject, public KListViewItem
{
  Q_OBJECT
  
  public:
    enum DccType
    {
      Send,
      Receive,
      DccTypeCount
    };
    
    enum DccStatus
    {
      Queued=0,      // Newly added DCC, RECV: Waiting for local user's response
      WaitingRemote, // SEND: Waiting for remote host's response
      LookingUp,     // RECV: looking up the server
      Connecting,    // RECV: trying to connect to the server
      Sending,       // Sending
      Receiving,     // Receiving
      Failed,        // Transfer failed
      Aborted,       // Transfer aborted by user
      Done,          // Transfer done
      DccStatusCount
    };
    
    DccTransfer(KListView* _parent, DccType _dccType, const QString& _partnerNick);
    virtual ~DccTransfer();
    
    virtual void paintCell(QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment);
    
    DccType getType() const { return dccType; }
    DccStatus getStatus() const { return dccStatus; }
    QString getOwnPort() const { return ownPort; }
    QString getPartnerNick() const { return partnerNick; }
    QString getFileName() const { return fileName; }
    QString getFilePath() const { return filePath; }
    bool isResumed() const { return bResumed; }
    
  signals:
    void done(const QString& filename);
    void statusChanged(const DccTransfer* item);
    
  public slots:
    virtual void start() = 0;
    virtual void abort() = 0;
    
  protected slots:
    void updateView();
    
  protected:
    void showProgressBar();  // called from printCell()
    
    void startAutoUpdateView();
    void stopAutoUpdateView();
    
    void setStatus(DccStatus status);
    
    // called from updateView()
    QString getTypeText() const;
    QPixmap getTypeIcon() const;
    QPixmap getStatusIcon() const;
    QString getStatusText() const;
    QString getFileSizePrettyText() const;
    QString getPositionPrettyText() const;
    QString getTimeRemainingPrettyText() const;
    QString getCPSPrettyText() const;
    
    unsigned long getCPS() const;
    
    static QString getNumericalIpText(const QString& _ip);
    static QString getErrorString(int code);
    static unsigned long intel(unsigned long value);
    
    // transfer information
    DccType dccType;
    DccStatus dccStatus;
    bool bResumed;
    unsigned long transferringPosition;
    unsigned long transferStartPosition;
    QString partnerNick;
    QString partnerIp;  // null when unknown
    QString partnerPort;
    QString ownIp;
    QString ownPort;
    QDateTime timeOffer;
    QDateTime timeTransferStarted;
    QDateTime timeLastActive;
    
    unsigned long bufferSize;
    char* buffer;
    
    // file information
    QFile file;
    QString fileName;
    QString filePath;
    unsigned long fileSize;
    
    // UI
    QTimer* autoUpdateViewTimer;
    
    KProgress* progressBar;
    
    static QString TypeText[DccTypeCount];
    static QString StatusText[DccStatusCount];
    
};

#endif
