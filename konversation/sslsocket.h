#ifndef SSL_SOCKET_H
#define SSL_SOCKET_H

/*
    ksslsocket.h - KDE SSL Socket

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

class KSSLSocketPrivate;

class KSSLSocket : public QObject
{
	Q_OBJECT

	public:
		KSSLSocket();
		~KSSLSocket();

		KStreamSocket* m_streamSocket;
		void showInfoDialog();

	signals:
		void sslFailure();

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

		KSSLSocketPrivate *d;

};

#endif
