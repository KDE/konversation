/*
    sslsocket.cpp - KDE SSL Socket

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

#include <klocale.h>
#include <kdebug.h>
#include <kssl.h>
#include <ksslinfodlg.h>
#include <ksslpeerinfo.h>
#include <ksslcertchain.h>
#include <ksslcertificatecache.h>
#include <kmessagebox.h>
#include <ksocketdevice.h>

#include "sslsocket.h"

SSLSocket::SSLSocket(QObject* parent,const char* name)
  : KStreamSocket("","",parent,name), kssl(0L), sslInfoDlg(0L)
{
	cc = new KSSLCertificateCache;
	cc->reload();

	QObject::connect(this,SIGNAL(connected(const KResolverEntry&)),this,SLOT(slotConnected()));
}

SSLSocket::~SSLSocket()
{
    // Close stream socket
    close();

    // close ssl socket
    if( kssl ) kssl->close();

    delete kssl;
    delete cc;
    delete sslInfoDlg;
}

Q_LONG SSLSocket::writeBlock(const char *data, Q_ULONG len)
{
  //kdDebug() << "SSLSocket::writeBlock : " << data << endl;
  return kssl->write( data,len );
}

Q_LONG SSLSocket::readBlock(char *data, Q_ULONG maxlen)
{
  //kdDebug() << "SSLSocket::readBlock : " << QCString(data) << endl;
  return kssl->read( data, maxlen );
}

void SSLSocket::slotConnected()
{
  
  if( KSSL::doesSSLWork() )
    {
      kdDebug() << k_funcinfo << "Trying SSL connection..." << endl;
      if( !kssl )
	{
	  kssl = new KSSL();
	  if( kssl->connect( socketDevice()->socket() ) )
	    {
	      if( verifyCertificate() != 1 )
		{
		  kdDebug() << "Closing socket!" << endl;
		  close();
		}
	      else
		emit sslInitDone();
	    }
	}
      else
	{
	  kssl->reInitialize();
	}
    }
  else
    {
      kdError() << k_funcinfo << "SSL not functional!" << endl;
      kssl = 0L;
      emit sslFailure();
      close();
    }
}

void SSLSocket::showInfoDialog()
{
    if( state() == KNetwork::KClientSocketBase::Connected )
    {
        showSSLInfoDialog();
    }
}

void SSLSocket::showSSLInfoDialog()
{
    // Taken from kio/misc/uiserver.cpp
    // Variable names changed and adopted to our usage for Konversation
    // Copyright ( C ) 2000 Matej Koss <koss@miesto.sk>
    // Copyright ( C ) David Faure <faure@kde.org>
    // Copyright ( C ) 2001 George Staikos <staikos@kde.org>

  if( !sslInfoDlg ) sslInfoDlg = new KSSLInfoDlg(true, 0L, 0L, true);
    KSSLCertificate *sslCert = KSSLCertificate::fromString(m_sslPeerCertificate.local8Bit());

    if ( sslCert ) {
        QStringList chainList = QStringList::split("\n", m_sslPeerChain );
        QPtrList<KSSLCertificate> newChainList;

        newChainList.setAutoDelete(true);

        for ( QStringList::Iterator it = chainList.begin(); it != chainList.end(); ++it ) {
            KSSLCertificate *tmpCert = KSSLCertificate::fromString( ( *it ).local8Bit() );
            if ( tmpCert ) newChainList.append( tmpCert );
        }

        if ( newChainList.count() > 0 )
            sslCert->chain().setChain( newChainList );

        sslInfoDlg->setCertState( m_sslCertErrors );
        sslInfoDlg->setup( sslCert,
                           remoteHost,
                           url,
                           kssl->connectionInfo().getCipher(),
                           kssl->connectionInfo().getCipherDescription(),
                           kssl->connectionInfo().getCipherVersion(),
                           kssl->connectionInfo().getCipherUsedBits(),
                           kssl->connectionInfo().getCipherBits(),
                           KSSLCertificate::KSSLValidation(m_sslCertState)
            );

        sslInfoDlg->exec();
    }
    else
        KMessageBox::information(0L,
                                 i18n( "The peer SSL certificate appears to be corrupt." ),
                                 i18n( "SSL" ) );
}

int SSLSocket::verifyCertificate()
{
	int rc = 0;
	bool permacache = false;
	bool _IPmatchesCN = false;
	int result;
	bool doAddHost = false;

        remoteHost = peerAddress().nodeName();
        url = "irc://"+remoteHost+":"+peerAddress().serviceName();

	KSSLCertificate& peerCertificate = kssl->peerInfo().getPeerCertificate();
	m_sslPeerCertificate = peerCertificate.toString();

	KSSLCertificate::KSSLValidationList validationList
            = peerCertificate.validateVerbose(KSSLCertificate::SSLServer);

	KSSLCertificate::KSSLValidation validation = KSSLCertificate::Ok;

        if (!validationList.isEmpty())
            validation = validationList.first();


	for(KSSLCertificate::KSSLValidationList::ConstIterator it = validationList.begin();
		it != validationList.end(); ++it)
	{
		m_sslCertErrors += QString::number(*it)+":";
	}


	if (peerCertificate.chain().isValid() && peerCertificate.chain().depth() > 1)
	{
		QString theChain;
		QPtrList<KSSLCertificate> chain = peerCertificate.chain().getChain();
		for (KSSLCertificate *c = chain.first(); c; c = chain.next())
		{
			theChain += c->toString();
			theChain += "\n";
		}
        }

	m_sslCertState = validation;

	if (validation == KSSLCertificate::Ok)
            rc = 1;

	//  - Read from cache and see if there is a policy for this
	KSSLCertificateCache::KSSLCertificatePolicy cp = cc->getPolicyByCertificate(peerCertificate);

	//  - validation code
	if (validation != KSSLCertificate::Ok)
	{
		if( cp == KSSLCertificateCache::Unknown || cp == KSSLCertificateCache::Ambiguous)
		{
			cp = KSSLCertificateCache::Prompt;
		}
		else
		{
			// A policy was already set so let's honor that.
			permacache = cc->isPermanent(peerCertificate);
		}

		if (!_IPmatchesCN && cp == KSSLCertificateCache::Accept )
		  {
		    do 
		      {
			QString msg = i18n("The IP address of the host %1 "
					   "does not match the one the "
					   "certificate was issued to.");
			result = KMessageBox::warningYesNo( 0L,
							    msg.arg(remoteHost),
							    i18n("Server Authentication"),
							    KGuiItem(i18n("Details")),
							    KGuiItem(i18n("Continue")),
							    "SslIpCNMismatch");
			if( result == KMessageBox::Yes )
			  showInfoDialog();
		      } 
		    while ( result == KMessageBox::Yes );

		  }


		// Precondition: cp is one of Reject, Accept or Prompt
		switch (cp)
		{
			case KSSLCertificateCache::Accept:
				rc = 1;
				break;

			case KSSLCertificateCache::Reject:
				rc = -1;
				break;

			case KSSLCertificateCache::Prompt:
			{
				do
				{
					if (validation == KSSLCertificate::InvalidHost)
					{
						QString msg = i18n("The IP address of the host %1 "
								"does not match the one the "
								"certificate was issued to.");
						result = KMessageBox::warningYesNo( 0L,
						msg.arg(remoteHost),
						i18n("Server Authentication"),
                                                KGuiItem(i18n("Details")),
                                                KGuiItem(i18n("Continue")),
						"SslInvalidHost");
					}
					else
					{
						QString msg = i18n("The server certificate failed the "
                                                                   "authenticity test (%1).");
						result = KMessageBox::warningYesNo( 0L,
                                                msg.arg(remoteHost),
						i18n("Server Authentication"),
                                                KGuiItem(i18n("Details")),
                                                KGuiItem(i18n("Continue")),
                                                "SslCertificateNotAuthentic" );
					}

					if (result == KMessageBox::Yes)
					{
						showInfoDialog();
					}
				}
				while (result == KMessageBox::Yes);

				if (result == KMessageBox::No)
				{
                                    rc = 1;
                                    cp = KSSLCertificateCache::Accept;
                                    doAddHost = true;
                                    result = KMessageBox::warningYesNo( 0L,
                                                           i18n("Would you like to accept this "
                                                                "certificate forever without "
                                                                "being prompted?"),
                                                           i18n("Server Authentication"),
                                                           KGuiItem(i18n("&Forever")),
                                                           KGuiItem(i18n("&Current Sessions Only"))
                                        );

					if (result == KMessageBox::Yes)
						permacache = true;
					else
						permacache = false;
				}
				else
				{
					rc = -1;
					cp = KSSLCertificateCache::Prompt;
				}

				break;
		}
		default:
                    kdDebug() << "SSL error in cert code." << endl;
                    break;
		}
	}

	//  - cache the results
	cc->addCertificate(peerCertificate, cp, permacache);
	if (doAddHost)
	  cc->addHost(peerCertificate, remoteHost);
	 


	if (rc == -1)
		return rc;


	kdDebug() << "SSL connection information follows:" << endl
		<< "+-----------------------------------------------" << endl
		<< "| Cipher: " << kssl->connectionInfo().getCipher() << endl
		<< "| Description: " << kssl->connectionInfo().getCipherDescription() << endl
		<< "| Version: " << kssl->connectionInfo().getCipherVersion() << endl
		<< "| Strength: " << kssl->connectionInfo().getCipherUsedBits()
		<< " of " << kssl->connectionInfo().getCipherBits()
		<< " bits used." << endl
		<< "| PEER:" << endl
		<< "| Subject: " << kssl->peerInfo().getPeerCertificate().getSubject() << endl
		<< "| Issuer: " << kssl->peerInfo().getPeerCertificate().getIssuer() << endl
		<< "| Validation: " << (int) validation << endl
		<< "+-----------------------------------------------"
		<< endl;

	return rc;
}


#include "sslsocket.moc"
