/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

// dcctransfersend.h : separated from dcctransfer.h
/*
  dcctransfer.h  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef DCCTRANSFERSEND_H
#define DCCTRANSFERSEND_H

#include "dcctransfer.h"

class QTimer;

namespace KNetwork
{
  class KServerSocket;
  class KStreamSocket;
}

class DccTransferSend : public DccTransfer
{
  Q_OBJECT
  
  public:
    DccTransferSend(KListView* _parent, const QString& _partnerNick, const KURL& _fileURL, const QString& _ownIp);
    virtual ~DccTransferSend();
    
    void setResume(unsigned long _position);
    
  signals:
    void sendReady(const QString& partner, const QString& fileName, const QString& ownIp, const QString& ownPort, unsigned long fileSize);
    
  public slots:
    virtual void start();
    virtual void abort();
    
  protected slots:
    void heard();
    void writeData();
    void getAck();
    void socketError(int errorCode);
    void connectionTimeout();
    
  protected:
    void cleanUp();
    void startConnectionTimer(int sec);
    void stopConnectionTimer();
    QFile file;

    /*The filename of the temporary file that we downloaded.  So if send a file ftp://somewhere/file.txt
     * Then this will be downloaded to /tmp.
     */
    QString tmpFile;
    KNetwork::KServerSocket* serverSocket;
    KNetwork::KStreamSocket* sendSocket;
    QTimer* connectionTimer;
};

#endif // DCCTRANSFERSEND_H
