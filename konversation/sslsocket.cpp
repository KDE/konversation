/*
    sslsocket.cpp

    Copyright (c) 2004      by İsmail Dönmez <ismail.donmez@boun.edu.tr>

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
#include <unistd.h>

#include "sslsocket.h"

SSLSocket::SSLSocket(QWidget* serverParent, QObject* parent, const char* name)
  : KStreamSocket("","",parent,name), m_serverParent(serverParent), kssl(0L) 
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
}

Q_LONG SSLSocket::writeBlock(const char *data, Q_ULONG len)
{
  //kdDebug() << "SSLSocket::writeBlock : " << data << endl;
  return kssl->write( data,len );
}

Q_LONG SSLSocket::readBlock(char *data, Q_ULONG maxlen)
{
  //kdDebug() << "SSLSocket::readBlock : " << QCString(data) << endl;
  int err = 0;

  /* Default KSSL timeout is 0.2 seconds so we loop here until socket times out */

    while(bytesAvailable() && err == 0) {
      err = kssl->read( data, maxlen );
      if (err == 0) {
	::sleep(1);
      }
    }

  return err;
}

QString SSLSocket::details()
{
  if(state() == KNetwork::KClientSocketBase::Connected) {
    QString details;
    int strength = kssl->connectionInfo().getCipherUsedBits();
    
    details = "Connection is secured with ";
    details += QString::number(strength);
    details += " bit SSL";

    return details;
  }
  else
    return "";
}

void SSLSocket::slotConnected()
{
  
  if( KSSL::doesSSLWork() )
    {
      kdDebug() << "Trying SSL connection..." << endl;
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
      kdError() << "SSL not functional!" << endl;
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
  /* We don't delete sslInfoDlg here as code in kio/misc/uiserver.cpp says not to do so*/

  KSSLInfoDlg* sslInfoDlg = new KSSLInfoDlg(true, 0L, 0L, true);
  sslInfoDlg->setCertState( m_sslCertErrors );
  sslInfoDlg->setup( *kssl,
		     (const QString&) remoteHost,
		     (const QString&) url
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
  
  remoteHost = peerAddress().nodeName();
  url = "irc://"+remoteHost+":"+peerAddress().serviceName();
  
  KSSLCertificate& peerCertificate = kssl->peerInfo().getPeerCertificate();

  validation = peerCertificate.validate();
  if(validation == KSSLCertificate::Unknown ) {
    emit sslFailure();
    return 0;
  }
  
  KSSLX509Map certinfo(peerCertificate.getSubject());
  hostname = certinfo.getValue("CN");
    
  KSSLCertificate::KSSLValidationList validationList
    = peerCertificate.validateVerbose(KSSLCertificate::SSLServer);
  
  ipMatchesCN = kssl->peerInfo().certMatchesAddress();
  
  validation = KSSLCertificate::Ok;
  
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
