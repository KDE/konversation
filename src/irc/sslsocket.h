#ifndef SSL_SOCKET_H
#define SSL_SOCKET_H
/*
    KDE SSL Socket

    Copyright (c) 2004,2005 by İsmail Dönmez <ismail.donmez@boun.edu.tr>

    based on the code by:
    Copyright (c) 2004      by Jason Keirstead <jason@keirstead.org>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
* This program is free software; you can redistribute it and/or modify  *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation; either version 2 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
*************************************************************************
*/

#include <k3streamsocket.h>
using namespace KNetwork;

struct SSLSocketPrivate;

class SSLSocket : public KStreamSocket
{
    Q_OBJECT

    public:
        explicit SSLSocket(QWidget* serverParent, QObject* parent = 0L, const char* name = 0L);
        ~SSLSocket();

        void showInfoDialog();
        const QString details();

        Q_LONG writeBlock (const char *data, Q_ULONG len);
        Q_LONG readBlock  (char *data, Q_ULONG maxlen);

    protected:
        void stateChanging (KClientSocketBase::SocketState newState);

        signals:
        /** Emitted when there is a problem with SSL
         *  @param reason An error string that has already been through i18n
         */
        void sslFailure(const QString& reason);
        void sslInitDone();

    private:
        void connected();
        int verifyCertificate();
        void showSSLInfoDialog();

        QWidget* m_serverParent;

        SSLSocketPrivate* d;
};
#endif
