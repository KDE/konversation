/*  -*- C++ -*-
 *  Copyright (C) 2003 Thiago Macieira <thiago.macieira@kdemail.net>
 *
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included 
 *  in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

// System includes
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Qt includes
#include <qapplication.h>
#include <qstring.h>
#include <qcstring.h>
#include <qstrlist.h>
#include <qstringlist.h>
#include <qshared.h>
#include <qdatetime.h>
#include <qtimer.h>
#include <qmutex.h>
#include <qguardedptr.h>

// IDN
#ifdef HAVE_IDNA_H
# include <idna.h>
#endif

// KDE
#include <klocale.h>

// Us
#include "kresolver.h"
#include "kresolver_p.h"
#include "ksocketaddress.h"

using namespace KNetwork;
using namespace KNetwork::Internal;

/////////////////////////////////////////////
// class KResolverEntry

class KNetwork::KResolverEntryPrivate: public QShared
{
public:
  KSocketAddress addr;
  int socktype;
  int protocol;
  QString canonName;
  QCString encodedName;

  inline KResolverEntryPrivate() :
    socktype(0), protocol(0)
  { }
};

// default constructor
KResolverEntry::KResolverEntry() :
  d(0L)
{
}

// constructor with stuff
KResolverEntry::KResolverEntry(const KSocketAddress& addr, int socktype, int protocol,
			       const QString& canonName, const QCString& encodedName) :
  d(new KResolverEntryPrivate)
{
  d->addr = addr;
  d->socktype = socktype;
  d->protocol = protocol;
  d->canonName = canonName;
  d->encodedName = encodedName;
}

// constructor with even more stuff
KResolverEntry::KResolverEntry(const struct sockaddr* sa, Q_UINT16 salen, int socktype,
			       int protocol, const QString& canonName,
			       const QCString& encodedName) :
  d(new KResolverEntryPrivate)
{
  d->addr = KSocketAddress(sa, salen);
  d->socktype = socktype;
  d->protocol = protocol;
  d->canonName = canonName;
  d->encodedName = encodedName;
}

// copy constructor
KResolverEntry::KResolverEntry(const KResolverEntry& that) :
  d(0L)
{
  *this = that;
}

// destructor
KResolverEntry::~KResolverEntry()
{
  if (d == 0L)
    return;

  if (d->deref())
    delete d;
}

// returns the socket address
KSocketAddress KResolverEntry::address() const
{
  return d ? d->addr : KSocketAddress();
}

// returns the length
Q_UINT16 KResolverEntry::length() const
{
  return d ? d->addr.length() : 0;
}

// returns the family
int KResolverEntry::family() const
{
  return d ? d->addr.family() : AF_UNSPEC;
}

// returns the canonical name
QString KResolverEntry::canonicalName() const
{
  return d ? d->canonName : QString::null;
}

// returns the encoded name
QCString KResolverEntry::encodedName() const
{
  return d ? d->encodedName : QCString();
}

// returns the socket type
int KResolverEntry::socketType() const
{
  return d ? d->socktype : 0;
}

// returns the protocol
int KResolverEntry::protocol() const
{
  return d ? d->protocol : 0;
}

// assignment operator
KResolverEntry& KResolverEntry::operator= (const KResolverEntry& that)
{
  // copy the data
  if (that.d)
    that.d->ref();

  if (d && d->deref())
    delete d;

  d = that.d;
  return *this;
}

/////////////////////////////////////////////
// class KResolverResults

class KNetwork::KResolverResultsPrivate: public QShared
{
public:
  QString node, service;
  int errorcode, syserror;

  KResolverResultsPrivate() :
    errorcode(0), syserror(0)
  { }

  // duplicate the data if necessary, while decreasing the reference count
  // on the original data
  inline void dup(KResolverResultsPrivate*& d)
  {
    if (!d->count > 1)
      {
	d->deref();
	KResolverResultsPrivate *e = new KResolverResultsPrivate(*d);
	e->count = 1;
	d = e;			// set the pointer
      }
  }
};

// default constructor
KResolverResults::KResolverResults()
  : d(new KResolverResultsPrivate)
{
}

// copy constructor
KResolverResults::KResolverResults(const KResolverResults& other)
  : QValueList<KResolverEntry>(other), d(other.d)
{
  d->ref();
}

// destructor
KResolverResults::~KResolverResults()
{
  if (d->deref())
    delete d;
}

// assignment operator
KResolverResults&
KResolverResults::operator= (const KResolverResults& other)
{
  other.d->ref();

  // release our data
  if (d->deref())
    delete d;

  // copy over the other data
  d = other.d;

  // now let QValueList do the rest of the work
  QValueList<KResolverEntry>::operator =(other);

  return *this;
}

// gets the error code
int KResolverResults::error() const
{
  return d->errorcode;
}

// gets the system errno
int KResolverResults::systemError() const
{
  return d->syserror;
}

// sets the error codes
void KResolverResults::setError(int errorcode, int systemerror)
{
  d->dup(d);

  d->errorcode = errorcode;
  d->syserror = systemerror;
}

// gets the hostname
QString KResolverResults::nodeName() const
{
  return d->node;
}

// gets the service name
QString KResolverResults::serviceName() const
{
  return d->service;
}

// sets the address
void KResolverResults::setAddress(const QString& node,
				  const QString& service)
{
  d->dup(d);

  d->node = node;
  d->service = service;
}
  
void KResolverResults::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }


///////////////////////
// class KResolver

// default constructor
KResolver::KResolver(QObject *parent, const char *name)
  : QObject(parent, name), d(new KResolverPrivate(this))
{
}

// constructor with host and service
KResolver::KResolver(const QString& nodename, const QString& servicename,
		   QObject *parent, const char *name)
  : QObject(parent, name), d(new KResolverPrivate(this, nodename, servicename))
{
}

// destructor
KResolver::~KResolver()
{
  // this deletes our d pointer (if necessary)
  // and cancels the lookup as well
  KResolverManager::manager()->aboutToBeDeleted(this);
  d = 0L;
}

// get the status
int KResolver::status() const
{
  return d->status;
}

// get the error code
int KResolver::error() const
{
  return d->errorcode;
}

// get the errno
int KResolver::systemError() const
{
  return d->syserror;
}

// are we running?
bool KResolver::isRunning() const
{
  return d->status > 0 && d->status < Success;
}

// get the hostname
QString KResolver::nodeName() const
{
  return d->input.node;
}

// get the service
QString KResolver::serviceName() const
{
  return d->input.service;
}

// sets the hostname
void KResolver::setNodeName(const QString& nodename)
{
  // don't touch those values if we're working!
  if (!isRunning())
    {
      d->input.node = nodename;
      d->status = Idle;
      d->results.setAddress(nodename, d->input.service);
    }
}

// sets the service
void KResolver::setServiceName(const QString& service)
{
  // don't change if running
  if (!isRunning())
    {
      d->input.service = service;
      d->status = Idle;
      d->results.setAddress(d->input.node, service);
    }
}

// sets the address
void KResolver::setAddress(const QString& nodename, const QString& service)
{
  setNodeName(nodename);
  setServiceName(service);
}

// get the flags
int KResolver::flags() const
{
  return d->input.flags;
}

// sets the flags
int KResolver::setFlags(int flags)
{
  int oldflags = d->input.flags;
  if (!isRunning())
    {
      d->input.flags = flags;
      d->status = Idle;
    }
  return oldflags;
}

// sets the family mask
void KResolver::setFamily(int families)
{
  if (!isRunning())
    {
      d->input.familyMask = families;
      d->status = Idle;
    }
}

// sets the socket type
void KResolver::setSocketType(int type)
{
  if (!isRunning())
    {
      d->input.socktype = type;
      d->status = Idle;
    }
}

// sets the protocol
void KResolver::setProtocol(int protonum, const char *name)
{
  if (isRunning())
    return;			// can't change now

  // we copy the given protocol name. If it isn't an empty string
  // and the protocol number was 0, we will look it up in /etc/protocols
  // we also leave the error reporting to the actual lookup routines, in
  // case the given protocol name doesn't exist

  d->input.protocolName = name;
  if (protonum == 0 && name != 0L && *name != '\0')
    {
      // must look up the protocol number
      d->input.protocol = KResolver::protocolNumber(name);
    }
  else
    d->input.protocol = protonum;
  d->status = Idle;
}

bool KResolver::start()
{
  if (!isRunning())
    {
      d->results.empty();
      d->emitSignal = true;	// reset the variable

      // is there anything to be queued?
      if (d->input.node.isEmpty() && d->input.service.isEmpty())
	{
	  d->status = KResolver::Success;
	  emitFinished();
	}
      else
	KResolverManager::manager()->enqueue(this, 0L);
    }

  return true;
}

bool KResolver::wait(int msec)
{
  if (!isRunning())
    {
      emitFinished();
      return true;
    }

  QMutexLocker locker(&d->mutex);

  if (!isRunning())
    return true;
  else
    {
      QTime t;
      t.start();

      while (!msec || t.elapsed() < msec)
	{
	  // wait on the manager to broadcast completion
	  d->waiting = true;
	  if (msec)
	    KResolverManager::manager()->notifyWaiters.wait(&d->mutex, msec - t.elapsed());
	  else
	    KResolverManager::manager()->notifyWaiters.wait(&d->mutex);

	  // the manager has processed
	  // see if this object is done
	  if (!isRunning())
	    {
	      // it's done
	      d->waiting = false;
	      emitFinished();
	      return true;
	    }
	}

      // if we've got here, we've timed out
      d->waiting = false;
      return false;
    }
}

void KResolver::cancel(bool emitSignal)
{
  d->emitSignal = emitSignal;
  KResolverManager::manager()->dequeue(this);
}

KResolverResults
KResolver::results() const
{
  if (!isRunning())
    return d->results;

  // return a dummy, empty result
  KResolverResults r;
  r.setAddress(d->input.node, d->input.service);
  r.setError(d->errorcode, d->syserror);
  return r;
}

bool KResolver::event(QEvent* e)
{
  if (static_cast<int>(e->type()) == KResolverManager::ResolutionCompleted)
    {
      emitFinished();
      return true;
    }

  return false;
}

void KResolver::emitFinished()
{
  if (isRunning())
    d->status = KResolver::Success;

  QGuardedPtr<QObject> p = this; // guard against deletion

  if (d->emitSignal)
    emit finished(d->results);

  if (p && d->deleteWhenDone)
    deleteLater();		// in QObject
}

QString KResolver::errorString(int errorcode, int syserror)
{
  // no i18n now...
  static const char * const messages[] =
  {
    I18N_NOOP("no error"),	// NoError
    I18N_NOOP("requested family not supported for this host name"), // AddrFamily
    I18N_NOOP("temporary failure in name resolution"),	// TryAgain
    I18N_NOOP("non-recoverable failure in name resolution"), // NonRecoverable
    I18N_NOOP("invalid flags"),			// BadFlags
    I18N_NOOP("memory allocation failure"),	// Memory
    I18N_NOOP("name or service not known"),	// NoName
    I18N_NOOP("requested family not supported"),	// UnsupportedFamily
    I18N_NOOP("requested service not supported for this socket type"), // UnsupportedService
    I18N_NOOP("requested socket type not supported"),	// UnsupportedSocketType
    I18N_NOOP("unknown error"),			// UnknownError
    I18N_NOOP("system error: %1")		// SystemError
  };

  // handle the special value
  if (errorcode == Canceled)
    return i18n("request was canceled");

  if (errorcode > 0 || errorcode < SystemError)
    return QString::null;

  QString msg = i18n(messages[-errorcode]);
  if (errorcode == SystemError)
    msg.arg(QString::fromLocal8Bit(strerror(syserror)));

  return msg;
}

KResolverResults
KResolver::resolve(const QString& host, const QString& service, int flags,
		  int families)
{
  KResolver qres(host, service, qApp, "synchronous KResolver");
  qres.setFlags(flags);
  qres.setFamily(families);
  qres.start();
  qres.wait();
  return qres.results();
}

bool KResolver::resolveAsync(QObject* userObj, const char *userSlot,
			     const QString& host, const QString& service,
			     int flags, int families)
{
  KResolver* qres = new KResolver(host, service, qApp, "asynchronous KResolver");
  QObject::connect(qres, SIGNAL(finished(KResolverResults)), userObj, userSlot);
  qres->setFlags(flags);
  qres->setFamily(families);
  qres->d->deleteWhenDone = true; // this is the only difference from the example code
  return qres->start();
}

#ifdef NEED_MUTEX
QMutex getXXbyYYmutex;
#endif

QStrList KResolver::protocolName(int protonum)
{
  struct protoent *pe;
#ifndef HAVE_GETPROTOBYNAME_R
  QMutexLocker locker(&getXXbyYYmutex);

  pe = getprotobynumber(protonum);

#else
  size_t buflen = 1024;
  struct protoent protobuf;
  char *buf;
  do
    {
      buf = new char[buflen];
      if (getprotobynumber_r(protonum, &protobuf, buf, buflen, &pe) == ERANGE)
	{
	  buflen += 1024;
	  delete [] buf;
	}
      else
	break;
    }
  while (pe == 0L);
#endif

  // Do common processing
  QStrList lst(true);	// use deep copies
  if (pe != NULL)
    {
      lst.append(pe->p_name);
      for (char **p = pe->p_aliases; *p; p++)
	lst.append(*p);
    }

#ifdef HAVE_GETPROTOBYNAME_R
  delete [] buf;
#endif

  return lst;
}

QStrList KResolver::protocolName(const char *protoname)
{
  struct protoent *pe;
#ifndef HAVE_GETPROTOBYNAME_R
  QMutexLocker locker(&getXXbyYYmutex);

  pe = getprotobyname(protoname);

#else
  size_t buflen = 1024;
  struct protoent protobuf;
  char *buf;
  do
    {
      buf = new char[buflen];
      if (getprotobyname_r(protoname, &protobuf, buf, buflen, &pe) == ERANGE)
	{
	  buflen += 1024;
	  delete [] buf;
	}
      else
	break;
    }
  while (pe == 0L);
#endif

  // Do common processing
  QStrList lst(true);	// use deep copies
  if (pe != NULL)
    {
      lst.append(pe->p_name);
      for (char **p = pe->p_aliases; *p; p++)
	lst.append(*p);
    }

#ifdef HAVE_GETPROTOBYNAME_R
  delete [] buf;
#endif

  return lst;
}

int KResolver::protocolNumber(const char *protoname)
{
  struct protoent *pe;
#ifndef HAVE_GETPROTOBYNAME_R
  QMutexLocker locker(&getXXbyYYmutex);

  pe = getprotobyname(protoname);

#else
  size_t buflen = 1024;
  struct protoent protobuf;
  char *buf;
  do
    {
      buf = new char[buflen];
      if (getprotobyname_r(protoname, &protobuf, buf, buflen, &pe) == ERANGE)
	{
	  buflen += 1024;
	  delete [] buf;
	}
      else
	break;
    }
  while (pe == 0L);
#endif

  // Do common processing
  int protonum = -1;
  if (pe != NULL)
    protonum = pe->p_proto;

#ifdef HAVE_GETPROTOBYNAME_R
  delete [] buf;
#endif

  return protonum;
}

int KResolver::servicePort(const char *servname, const char *protoname)
{
  struct servent *se;
#ifndef HAVE_GETSERVBYNAME_R
  QMutexLocker locker(&getXXbyYYmutex);

  se = getservbyname(servname, protoname);

#else
  size_t buflen = 1024;
  struct servent servbuf;
  char *buf;
  do
    {
      buf = new char[buflen];
      if (getservbyname_r(servname, protoname, &servbuf, buf, buflen, &se) == ERANGE)
	{
	  buflen += 1024;
	  delete [] buf;
	}
      else
	break;
    }
  while (se == 0L);
#endif

  // Do common processing
  int servport = -1;
  if (se != NULL)
    servport = ntohs(se->s_port);

#ifdef HAVE_GETSERVBYNAME_R
  delete [] buf;
#endif

  return servport;
}

QStrList KResolver::serviceName(const char* servname, const char *protoname)
{
  struct servent *se;
#ifndef HAVE_GETSERVBYNAME_R
  QMutexLocker locker(&getXXbyYYmutex);

  se = getservbyname(servname, protoname);

#else
  size_t buflen = 1024;
  struct servent servbuf;
  char *buf;
  do
    {
      buf = new char[buflen];
      if (getservbyname_r(servname, protoname, &servbuf, buf, buflen, &se) == ERANGE)
	{
	  buflen += 1024;
	  delete [] buf;
	}
      else
	break;
    }
  while (se == 0L);
#endif

  // Do common processing
  QStrList lst(true);	// use deep copies
  if (se != NULL)
    {
      lst.append(se->s_name);
      for (char **p = se->s_aliases; *p; p++)
	lst.append(*p);
    }

#ifdef HAVE_GETSERVBYNAME_R
  delete [] buf;
#endif

  return lst;
}

QStrList KResolver::serviceName(int port, const char *protoname)
{
  struct servent *se;
#ifndef HAVE_GETSERVBYNAME_R
  QMutexLocker locker(&getXXbyYYmutex);

  se = getservbyport(port, protoname);

#else
  size_t buflen = 1024;
  struct servent servbuf;
  char *buf;
  do
    {
      buf = new char[buflen];
      if (getservbyport_r(port, protoname, &servbuf, buf, buflen, &se) == ERANGE)
	{
	  buflen += 1024;
	  delete [] buf;
	}
      else
	break;
    }
  while (se == 0L);
#endif

  // Do common processing
  QStrList lst(true);	// use deep copies
  if (se != NULL)
    {
      lst.append(se->s_name);
      for (char **p = se->s_aliases; *p; p++)
	lst.append(*p);
    }

#ifdef HAVE_GETSERVBYNAME_R
  delete [] buf;
#endif

  return lst;
}

// forward declaration
static QStringList splitLabels(const QString& unicodeDomain);
static QCString ToASCII(const QString& label);
static QString ToUnicode(const QString& label);
  
// implement the ToAscii function, as described by IDN documents
QCString KResolver::domainToAscii(const QString& unicodeDomain)
{
  QCString retval;
  // RFC 3490, section 4 describes the operation:
  // 1) this is a query, so don't allow unassigned

  // 2) split the domain into individual labels, without
  // separators.
  QStringList input = splitLabels(unicodeDomain);

  // 3) decide whether to enforce the STD3 rules for chars < 0x7F
  // we don't enforce

  // 4) for each label, apply ToASCII
  QStringList::Iterator it = input.begin();
  for ( ; it != input.end(); it++)
    {
      QCString cs = ToASCII(*it);
      if (cs.isNull())
	return QCString();	// error!

      // no, all is Ok.
      if (!retval.isEmpty())
	retval += '.';
      retval += cs;
    }

  return retval;
}

QString KResolver::domainToUnicode(const QCString& asciiDomain)
{
  return domainToUnicode(QString::fromLatin1(asciiDomain));
}

// implement the ToUnicode function, as described by IDN documents
QString KResolver::domainToUnicode(const QString& asciiDomain)
{
  if (asciiDomain.isEmpty())
    return asciiDomain;

  QString retval;

  // draft-idn-idna-14.txt, section 4 describes the operation:
  // 1) this is a query, so don't allow unassigned
  //   besides, input is ASCII

  // 2) split the domain into individual labels, without
  // separators.
  QStringList input = splitLabels(asciiDomain);

  // 3) decide whether to enforce the STD3 rules for chars < 0x7F
  // we don't enforce

  // 4) for each label, apply ToUnicode
  QStringList::Iterator it;
  for (it = input.begin(); it != input.end(); it++)
    {
      QString label = ToUnicode(*it).lower();

      // ToUnicode can't fail
      if (!retval.isEmpty())
	retval += '.';
      retval += label;
    }

  return retval;
}

QString KResolver::normalizeDomain(const QString& domain)
{
  return domainToUnicode(domainToAscii(domain));
}

void KResolver::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }

// here follows IDN functions
// all IDN functions conform to the following documents:
//  RFC 3454 - Preparation of Internationalized Strings
//  RFC 3490 - Internationalizing Domain Names in Applications (IDNA)
//  RFC 3491 - Nameprep: A Stringprep Profile for
//                Internationalized Domain Names (IDN
//  RFC 3492 - Punycode: A Bootstring encoding of Unicode
//          for Internationalized Domain Names in Applications (IDNA)

static QStringList splitLabels(const QString& unicodeDomain)
{
  // From RFC 3490 section 3.1:
  // "Whenever dots are used as label separators, the following characters
  // MUST be recognized as dots: U+002E (full stop), U+3002 (ideographic full
  // stop), U+FF0E (fullwidth full stop), U+FF61 (halfwidth ideographic full
  // stop)."
  static const unsigned int separators[] = { 0x002E, 0x3002, 0xFF0E, 0xFF61 };

  QStringList lst;
  int start = 0;
  uint i;
  for (i = 0; i < unicodeDomain.length(); i++)
    {
      unsigned int c = unicodeDomain[i].unicode();

      if (c == separators[0] ||
	  c == separators[1] ||
	  c == separators[2] ||
	  c == separators[3])
	{
	  // found a separator!
	  lst << unicodeDomain.mid(start, i - start);
	  start = i + 1;
	}
    }
  if ((long)i > start)
    // there is still one left
    lst << unicodeDomain.mid(start, i - start);

  return lst;
}

static QCString ToASCII(const QString& label)
{
#ifdef HAVE_IDNA_H
  // We have idna.h, so we can use the idna_to_ascii
  // function :)

  if (label.length() > 64)
    return (char*)0L;		// invalid label

  QCString retval;
  char buf[65];

  Q_UINT32* ucs4 = new Q_UINT32[label.length() + 1];

  uint i;
  for (i = 0; i < label.length(); i++)
    ucs4[i] = (unsigned long)label[i].unicode();
  ucs4[i] = 0;			// terminate with NUL, just to be on the safe side

  if (idna_to_ascii_4i(ucs4, label.length(), buf, 0) == IDNA_SUCCESS)
    // success!
    retval = buf;

  delete [] ucs4;
  return retval;
#else
  return label.latin1();
#endif
}

static QString ToUnicode(const QString& label)
{
#ifdef HAVE_IDNA_H
  // We have idna.h, so we can use the idna_to_unicode
  // function :)

  Q_UINT32 *ucs4_input, *ucs4_output;
  size_t outlen;

  ucs4_input = new Q_UINT32[label.length() + 1];
  for (uint i = 0; i < label.length(); i++)
    ucs4_input[i] = (unsigned long)label[i].unicode();

  // try the same length for output
  ucs4_output = new Q_UINT32[outlen = label.length()];

  idna_to_unicode_44i(ucs4_input, label.length(),
		      ucs4_output, &outlen,
		      0);

  if (outlen > label.length())
    {
      // it must have failed
      delete [] ucs4_output;
      ucs4_output = new Q_UINT32[outlen];

      idna_to_unicode_44i(ucs4_input, label.length(),
			  ucs4_output, &outlen,
			  0);
    }

  // now set the answer
  QString result;
  result.setLength(outlen);
  for (uint i = 0; i < outlen; i++)
    result[i] = (unsigned int)ucs4_output[i];

  delete [] ucs4_input;
  delete [] ucs4_output;
  
  return result;
#else
  return label;
#endif
}

#include "kresolver.moc"
