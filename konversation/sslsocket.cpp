/*
    Copyright (c) 2004,2005 by İsmail Dönmez <ismail.donmez@boun.edu.tr>

    based on the code by :
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

struct SSLSocketPrivate 
{
  int m_sslCertState;
  QString remoteHost;
  QString url;
  QString m_sslCertErrors;

  KSSL* kssl;
  KSSLCertificateCache* cc;
};

SSLSocket::SSLSocket(QWidget* serverParent, QObject* parent, const char* name)
  : KBufferedSocket(0L,0L,parent,name), m_serverParent(serverParent) 
{
  d = new SSLSocketPrivate;
  d->kssl = 0L;
  d->cc = new KSSLCertificateCache;
  d->cc->reload();

  enableRead(true);
  enableWrite(true);
}

SSLSocket::~SSLSocket()
{
  // Close stream socket
  close();
  
  // close ssl socket
  if( d->kssl ) d->kssl->close();
  
  delete d->kssl;
  delete d->cc;
  delete d;
}

Q_LONG SSLSocket::writeBlock(const char *data, Q_ULONG len)
{
  kdDebug() << "SSLSocket::writeBlock : " << data << endl;
  return d->kssl->write( data,len );
}

Q_LONG SSLSocket::readBlock(char *data, Q_ULONG maxlen)
{
  int err = d->kssl->read( data, maxlen );
  kdDebug() << "SSLSocket::readBlock : " << QCString(data) << endl;
  return err;
}

void SSLSocket::stateChanging(KClientSocketBase::SocketState newState)
{
  //kdDebug() << "SSLSocket::stateChanging" << endl;
  if(newState == KClientSocketBase::Connected)
    {
      KClientSocketBase::stateChanging(KClientSocketBase::Connected);
      connected();
    }
  else
    {
      kdDebug() << "New state " << (int)newState << endl;
      KClientSocketBase::stateChanging(newState);
    }
}

const QString SSLSocket::details()
{
    QString details;
    int strength = d->kssl->connectionInfo().getCipherUsedBits();
    
    details = "Connection is secured with ";
    details += QString::number(strength);
    details += " bit SSL";

    return details;
}

void SSLSocket::connected()
{
  
  if( KSSL::doesSSLWork() )
    {
      kdDebug() << "Trying SSL connection..." << endl;
      if( !d->kssl )
	{
	  d->kssl = new KSSL();
	  if( d->kssl->connect( socketDevice()->socket() ) )
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
	  d->kssl->reInitialize();
	}
    }
  else
    {
      kdError() << "SSL not functional!" << endl;
      emit sslFailure(i18n("The functionality to connect to servers using encrypted SSL communications is not available to Konversation because OpenSSL support was not enabled at compile time.  You will need to get new version of KDE that has SSL support."));
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
  
  KSSLInfoDlg* sslInfoDlg = new KSSLInfoDlg(true, m_serverParent, "sslInfoDlg", true);
  sslInfoDlg->setCertState( d->m_sslCertErrors );
  sslInfoDlg->setup( *(d->kssl),
		     (const QString&) d->remoteHost,
		     (const QString&) d->url
		     );
  sslInfoDlg->exec();
}

int SSLSocket::verifyCertificate()
{
  int rc = 0;
  int result;
  bool permacache = false;
  bool ipMatchesCN = false;
  bool doAddHost = false;
  QString hostname;
  KSSLCertificate::KSSLValidation validation;
  
  d->remoteHost = peerAddress().nodeName();
  d->url = "irc://"+d->remoteHost+":"+peerAddress().serviceName();
  
  KSSLCertificate& peerCertificate = d->kssl->peerInfo().getPeerCertificate();

  validation = peerCertificate.validate();
  if(validation == KSSLCertificate::Unknown ) {
    emit sslFailure(i18n("The SSL certificate returned from the server was not recognized.  Maybe this server does not support SSL on the given port?  If this server supports normal, non-SSL communications as well, then SSL will be on a different port."));
    return 0;
  }
  
  KSSLX509Map certinfo(peerCertificate.getSubject());
  hostname = certinfo.getValue("CN");
    
  KSSLCertificate::KSSLValidationList validationList
    = peerCertificate.validateVerbose(KSSLCertificate::SSLServer);
  
  ipMatchesCN = d->kssl->peerInfo().certMatchesAddress();
  
  validation = KSSLCertificate::Ok;
  
  if (!validationList.isEmpty())
    validation = validationList.first();
  
  
  for(KSSLCertificate::KSSLValidationList::ConstIterator it = validationList.begin();
      it != validationList.end(); ++it)
    {
      d->m_sslCertErrors += QString::number(*it)+":";
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

  d->m_sslCertState = validation;
  
  if (validation == KSSLCertificate::Ok)
    rc = 1;
  
  //  - Read from cache and see if there is a policy for this
  KSSLCertificateCache::KSSLCertificatePolicy cp = d->cc->getPolicyByCertificate(peerCertificate);
  
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
	  permacache = d->cc->isPermanent(peerCertificate);
	}
      
      if (!ipMatchesCN && cp == KSSLCertificateCache::Accept )
	{
	  do 
	    {
	      QString msg = i18n("The IP address of the host %1 "
				 "does not match the one the "
				 "certificate was issued to.");
	      result = KMessageBox::warningYesNoCancel( m_serverParent,
							msg.arg(hostname),
							i18n("Server Authentication"),
							KGuiItem(i18n("Details")),
							KGuiItem(i18n("Continue")),
							"SslIpCNMismatch");
	      if(result == KMessageBox::Yes)
		showInfoDialog();
	      else if(result == KMessageBox::Cancel)
		{
		  return 0;
		}
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
		    result = KMessageBox::warningYesNoCancel( m_serverParent,
							      msg.arg(hostname),
							      i18n("Server Authentication"),
							      KGuiItem(i18n("Details")),
							      KGuiItem(i18n("Continue")),
							      "SslInvalidHost");
		  }
		else
		  {
		    QString msg = i18n("The server (%1) certificate failed the "
				       "authenticity test.");
		    result = KMessageBox::warningYesNoCancel( m_serverParent,
							      msg.arg(hostname),
							      i18n("Server Authentication"),
							      KGuiItem(i18n("Details")),
							      KGuiItem(i18n("Continue")),
							      "SslCertificateNotAuthentic" );
		  }
		
		if (result == KMessageBox::Yes)
		  {
		    showInfoDialog();
		  }
		else if(result == KMessageBox::Cancel)
		  {
		    return 0;
		  }
	      }
	    while (result == KMessageBox::Yes);
	    
	    if (result == KMessageBox::No)
	      {
		rc = 1;
		cp = KSSLCertificateCache::Accept;
		doAddHost = true;
		result = KMessageBox::warningYesNo( m_serverParent,
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
  d->cc->addCertificate(peerCertificate, cp, permacache);
  if (doAddHost)
    d->cc->addHost(peerCertificate, d->remoteHost);
  
  
  
  if (rc == -1)
    return rc;
  
  
  kdDebug() << "SSL connection information follows:" << endl
	    << "+-----------------------------------------------" << endl
	    << "| Cipher: " << d->kssl->connectionInfo().getCipher() << endl
	    << "| Description: " << d->kssl->connectionInfo().getCipherDescription() << endl
	    << "| Version: " << d->kssl->connectionInfo().getCipherVersion() << endl
	    << "| Strength: " << d->kssl->connectionInfo().getCipherUsedBits()
	    << " of " << d->kssl->connectionInfo().getCipherBits()
	    << " bits used." << endl
	    << "| PEER:" << endl
	    << "| Subject: " << d->kssl->peerInfo().getPeerCertificate().getSubject() << endl
	    << "| Issuer: " << d->kssl->peerInfo().getPeerCertificate().getIssuer() << endl
	    << "| Validation: " << (int) validation << endl
	    << "+-----------------------------------------------"
	    << endl;
  
  return rc;
}


#include "sslsocket.moc"
