/*
  send a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004,2005 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
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

class DccPanel;

class DccTransferSend : public DccTransfer
{
    Q_OBJECT

        public:
        DccTransferSend( DccPanel* panel, const QString& partnerNick, const KURL& fileURL, const QString& ownIp, const QString &altFileName = QString::null, uint fileSize = -1);
        virtual ~DccTransferSend();

        bool setResume( unsigned long position );

        signals:
        void sendReady( const QString& partner, const QString& fileName, const QString& ownIp, const QString& ownPort, unsigned long fileSize );

    public slots:
        virtual void start();
        virtual void abort();

    protected slots:
        void heard();
        void writeData();
        void getAck();
        void socketError( int errorCode );
        void connectionTimeout();
        void slotSendSocketClosed();
        void slotServerSocketClosed();

    protected:
        void cleanUp();
        void failed(const QString& errorMessage = QString::null );
        void startConnectionTimer( int sec );
        void stopConnectionTimer();

        static QString getQFileErrorString( int code );

        QFile m_file;

        /*The filename of the temporary file that we downloaded.  So if send a file ftp://somewhere/file.txt
         * Then this will be downloaded to /tmp.
         */
        QString m_tmpFile;

        KNetwork::KServerSocket* m_serverSocket;
        KNetwork::KStreamSocket* m_sendSocket;
        bool m_fastSend;

        QTimer* m_connectionTimer;

    private:
        virtual QString getTypeText() const;
        virtual QPixmap getTypeIcon() const;
};
#endif                                            // DCCTRANSFERSEND_H
