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

#include <config.h>

#include <qsocketnotifier.h>
#include <qmutex.h>

#include "ksocketaddress.h"
#include "kresolver.h"
#include "ksocketbase.h"
#include "ksocketdevice.h"
#include "kstreamsocket.h"
#include "kbufferedsocket.h"
#include "kserversocket.h"

using namespace KNetwork;

class KNetwork::KServerSocketPrivate
{
public:
  KResolver resolver;
  KResolverResults resolverResults;

  enum { None, LookupDone, Bound, Listening } state;
  int backlog;
  int timeout;

  bool bindWhenFound : 1, listenWhenBound : 1, useKBufferedSocket : 1;

  KServerSocketPrivate()
    : state(None), timeout(0), bindWhenFound(false), listenWhenBound(false),
      useKBufferedSocket(true)
  { 
    resolver.setFlags(KResolver::Passive);
    resolver.setFamily(KResolver::KnownFamily);
  }
};

KServerSocket::KServerSocket(QObject* parent, const char *name)
  : QObject(parent, name), d(new KServerSocketPrivate)
{
  QObject::connect(&d->resolver, SIGNAL(finished(KResolverResults)), 
		   this, SLOT(lookupFinishedSlot()));
}

KServerSocket::KServerSocket(const QString& service, QObject* parent, const char *name)
  : QObject(parent, name), d(new KServerSocketPrivate)
{
  QObject::connect(&d->resolver, SIGNAL(finished(KResolverResults)), 
		   this, SLOT(lookupFinishedSlot()));
  d->resolver.setServiceName(service);
}

KServerSocket::KServerSocket(const QString& node, const QString& service,
			     QObject* parent, const char* name)
  : QObject(parent, name), d(new KServerSocketPrivate)
{
  QObject::connect(&d->resolver, SIGNAL(finished(KResolverResults)), 
		   this, SLOT(lookupFinishedSlot()));
  setAddress(node, service);
}

KServerSocket::~KServerSocket()
{
  close();
  delete d;
}

bool KServerSocket::setSocketOptions(int opts)
{
  QMutexLocker locker(mutex());
  KSocketBase::setSocketOptions(opts); // call parent
  bool result = socketDevice()->setSocketOptions(opts); // and set the implementation
  copyError();
  return result;
}

KResolver& KServerSocket::resolver() const
{
  return d->resolver;
}

const KResolverResults& KServerSocket::resolverResults() const
{
  return d->resolverResults;
}

void KServerSocket::setResolutionEnabled(bool enable)
{
  if (enable)
    d->resolver.setFlags(d->resolver.flags() & ~KResolver::NoResolve);
  else
    d->resolver.setFlags(d->resolver.flags() | KResolver::NoResolve);
}

void KServerSocket::setFamily(int families)
{
  d->resolver.setFamily(families);
}

void KServerSocket::setAddress(const QString& service)
{
  d->resolver.setNodeName(QString::null);
  d->resolver.setServiceName(service);
  d->resolverResults.empty();
  if (d->state <= KServerSocketPrivate::LookupDone)
    d->state = KServerSocketPrivate::None;
}

void KServerSocket::setAddress(const QString& node, const QString& service)
{
  d->resolver.setNodeName(node);
  d->resolver.setServiceName(service);
  d->resolverResults.empty();
  if (d->state <= KServerSocketPrivate::LookupDone)
    d->state = KServerSocketPrivate::None;
}

void KServerSocket::setTimeout(int msec)
{
  d->timeout = msec;
}

bool KServerSocket::lookup()
{
  setError(NoError);
  if (d->resolver.isRunning() && !blocking())
    return true;		// already doing lookup

  if (d->state >= KServerSocketPrivate::LookupDone)
    return true;		// results are already available

  // make sure we have at least one parameter for lookup
  if (d->resolver.serviceName().isNull() &&
      !d->resolver.nodeName().isNull())
    d->resolver.setServiceName(QString::fromLatin1(""));

  // don't restart the lookups if they had succeeded and
  // the input values weren't changed

  // reset results
  d->resolverResults = KResolverResults();

  if (d->resolver.status() <= 0)
    // if it's already running, there's no harm in calling again
    d->resolver.start();	// signal may emit

  if (blocking())
    {
      // we're in blocking mode operation
      // wait for the results

      d->resolver.wait();	// signal may be emitted again
      // lookupFinishedSlot has been called
    }

  return true;
}

bool KServerSocket::bind(const KResolverEntry& address)
{
  if (socketDevice()->bind(address))
    {
      setError(NoError);

      d->state = KServerSocketPrivate::Bound;
      emit bound(address);
      return true;
    }
  copyError();
  return false;
}

bool KServerSocket::bind(const QString& node, const QString& service)
{
  setAddress(node, service);
  return bind();
}

bool KServerSocket::bind(const QString& service)
{
  setAddress(service);
  return bind();
}

bool KServerSocket::bind()
{
  if (d->state >= KServerSocketPrivate::Bound)
    return true;

  if (d->state < KServerSocketPrivate::LookupDone)
    {
      if (!blocking())
	{
	  d->bindWhenFound = true;
	  bool ok = lookup();	// will call doBind
	  if (d->state >= KServerSocketPrivate::Bound)
	    d->bindWhenFound = false;
	  return ok;
	}

      // not blocking
      if (!lookup())
	return false;
    }

  return doBind();
}

bool KServerSocket::listen(int backlog)
{
  // WARNING
  // this function has to be reentrant
  // due to the mechanisms used for binding, this function might
  // end up calling itself

  if (d->state == KServerSocketPrivate::Listening)
    return true;		// already listening

  d->backlog = backlog;

  if (d->state < KServerSocketPrivate::Bound)
    {
      // we must bind
      // note that we can end up calling ourselves here
      d->listenWhenBound = true;
      if (!bind())
	{
	  d->listenWhenBound = false;
	  return false;
	}

      if (d->state < KServerSocketPrivate::Bound)
	// asynchronous lookup in progress...
	// we can't be blocking here anyways
	return true;

      d->listenWhenBound = false;
    }

  if (d->state < KServerSocketPrivate::Listening)
    return doListen();

  return true;
}

void KServerSocket::close()
{
  socketDevice()->close();
  if (d->resolver.isRunning())
    d->resolver.cancel(false);
  d->state = KServerSocketPrivate::None;
  emit closed();
}

void KServerSocket::setAcceptBuffered(bool enable)
{
  d->useKBufferedSocket = enable;
}

KActiveSocketBase* KServerSocket::accept()
{
  if (d->state < KServerSocketPrivate::Listening)
    {
      if (!blocking())
	{
	  listen();
	  setError(WouldBlock);
	  return NULL;
	}
      else if (!listen())
	// error happened during listen
	return false;
    }

  // check to see if we're doing a timeout
  if (blocking() && d->timeout > 0)
    {
      bool timedout;
      if (!socketDevice()->poll(d->timeout, &timedout))
	{
	  copyError();
	  return NULL;
	}

      if (timedout)
	return 0L;
    }

  // we're listening here
  KSocketDevice* accepted = socketDevice()->accept();
  if (!accepted)
    {
      // error happened during accept
      copyError();
      return NULL;
    }

  KStreamSocket* streamsocket;
  if (d->useKBufferedSocket)
    streamsocket = new KBufferedSocket();
  else
    streamsocket = new KStreamSocket();
  streamsocket->setSocketDevice(accepted);

  // FIXME!
  // when KStreamSocket can find out the state of the socket passed through
  // setSocketDevice, this will probably be unnecessary:
  streamsocket->setState(KStreamSocket::Connected);
  streamsocket->setFlags(IO_Sequential | IO_Raw | IO_ReadWrite | IO_Open | IO_Async);

  return streamsocket;
}

KSocketAddress KServerSocket::localAddress() const
{
  return socketDevice()->localAddress();
}

KSocketAddress KServerSocket::externalAddress() const
{
  return socketDevice()->externalAddress();
}

void KServerSocket::lookupFinishedSlot()
{
  if (d->resolver.isRunning() || d->state > KServerSocketPrivate::LookupDone)
    return;

  if (d->resolver.status() < 0)
    {
      setError(LookupFailure);
      emit gotError(LookupFailure);
      d->bindWhenFound = d->listenWhenBound = false;
      d->state = KServerSocketPrivate::None;
      return;
    }

  // lookup succeeded
  d->resolverResults = d->resolver.results();
  d->state = KServerSocketPrivate::LookupDone;
  emit hostFound();

  if (d->bindWhenFound)
    doBind();
}

void KServerSocket::copyError()
{
  setError(socketDevice()->error());
}

bool KServerSocket::doBind()
{
  d->bindWhenFound = false;
  // loop through the results and bind to the first that works

  KResolverResults::ConstIterator it = d->resolverResults.begin();
  for ( ; it != d->resolverResults.end(); ++it)
    if (bind(*it))
      {
	if (d->listenWhenBound)
	  return doListen();
	return true;
      }
    else
      socketDevice()->close();	// didn't work, try again

  // failed to bind
  emit gotError(error());
  return false;
}

bool KServerSocket::doListen()
{
  if (!socketDevice()->listen(d->backlog))
    {
      copyError();
      emit gotError(error());
      return false;		// failed to listen
    }
  
  // set up ready accept signal
  QObject::connect(socketDevice()->readNotifier(), SIGNAL(activated(int)),
		   this, SIGNAL(readyAccept()));
  d->state = KServerSocketPrivate::Listening;
  return true;
}


#include "kserversocket.moc"
