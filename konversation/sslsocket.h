#ifndef SSL_SOCKET_H
#define SSL_SOCKET_H

/*
    sslsocket.h - KDE SSL Socket

    Copyright (c) 2004      by Jason Keirstead <jason@keirstead.org>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>
    Copyright (c) 2004      by İsmail Dönmez <ismail.donmez@boun.edu.tr>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qvariant.h>

#include <kstreamsocket.h>
using namespace KNetwork;

class KSSL;
class KSSLCertificateCache;
class KSSLInfoDlg;

class SSLSocket : public KStreamSocket
{
	Q_OBJECT

	public:
                SSLSocket(QObject* parent = 0L, const char* name = 0L);
		~SSLSocket();

		void showInfoDialog();
		QString details();

		Q_LONG writeBlock (const char *data, Q_ULONG len);
		Q_LONG readBlock  (char *data, Q_ULONG maxlen);

	signals:
		void sslFailure();
		void sslInitDone();

	private slots:
		void slotConnected();
	
        private:
		int verifyCertificate();
		void showSSLInfoDialog();

		QString remoteHost;
		QString url;
		int m_sslCertState;
		QString m_sslInUse;
		QString m_sslPeerCertificate;
		QString m_sslPeerChain;
		QString m_sslCertErrors;

		KSSL* kssl;
		KSSLCertificateCache* cc;
		KSSLInfoDlg *sslInfoDlg;

};

#endif
