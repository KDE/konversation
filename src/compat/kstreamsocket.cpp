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
#include <qdatetime.h>
#include <qtimer.h>

#include "ksocketaddress.h"
#include "kresolver.h"
#include "ksocketdevice.h"
#include "kstreamsocket.h"

using namespace KNetwork;

class KNetwork::KStreamSocketPrivate
{
public:
  KResolverResults::ConstIterator local, peer;
  QTime startTime;
  QTimer timer;

  int timeout;

  inline KStreamSocketPrivate()
    : timeout(0)
  { }
};

KStreamSocket::KStreamSocket(const QString& node, const QString& service,
			     QObject* parent, const char *name)
  : KClientSocketBase(parent, name), d(new KStreamSocketPrivate)
{
  peerResolver().setNodeName(node);
  peerResolver().setServiceName(service);
  peerResolver().setFamily(KResolver::KnownFamily);
  localResolver().setFamily(KResolver::KnownFamily);

  setSocketOptions(socketOptions() & ~Blocking);

  QObject::connect(&d->timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));
}

KStreamSocket::~KStreamSocket()
{
  delete d;
  // KClientSocketBase's destructor closes the socket
}

int KStreamSocket::timeout() const
{
  return d->timeout;
}

int KStreamSocket::remainingTimeout() const
{
  if (state() != Connecting)
    return timeout();
  if (timeout() <= 0)
    return 0;

  return timeout() - d->startTime.elapsed();
}

void KStreamSocket::setTimeout(int msecs)
{
  d->timeout = msecs;

  if (state() == Connecting)
    d->timer.changeInterval(msecs);
}

bool KStreamSocket::bind(const QString& node, const QString& service)
{
  if (state() != Idle)
    return false;

  if (!node.isNull())
    localResolver().setNodeName(node);
  if (!service.isNull())
    localResolver().setServiceName(service);
  return true;
}

bool KStreamSocket::connect(const QString& node, const QString& service)
{
  if (state() == Connected)
    return true;		// already connected

  if (state() > Connected)
    return false;		// can't do much here

  if (!node.isNull())
    peerResolver().setNodeName(node);
  if (!service.isNull())
    peerResolver().setServiceName(service);

  if (state() == Connecting && !blocking())
    {
      setError(IO_ConnectError, InProgress);
      emit gotError(InProgress);
      return true;		// we're already connecting
    }

  if (state() < HostFound)
    {
      // connection hasn't started yet
      if (!blocking())
	{
	  QObject::connect(this, SIGNAL(hostFound()), SLOT(hostFoundSlot()));
	  return lookup();
	}

      // blocking mode
      if (!lookup())
	return false;		// lookup failure
    }

  /*
   * lookup results are available here
   */

  if (timeout() > 0)
    {
      if (!blocking() && !d->timer.isActive())
	d->timer.start(timeout(), true);
      else
	{
	  // blocking connection with timeout
	  // this must be handled as a special case because it requires a
	  // non-blocking socket

	  d->timer.stop();	// no need for a timer here

	  socketDevice()->setBlocking(false);
	  while (true)
	    {
	      connectionEvent();
	      if (state() < Connecting)
		return false;	// error connecting
	      if (remainingTimeout() <= 0)
		{
		  // we've timed out
		  timeoutSlot();
		  return false;
		}

	      if (socketDevice()->error() == InProgress)
		{
		  bool timedout;
		  socketDevice()->poll(remainingTimeout(), &timedout);
		  if (timedout)
		    {
		      timeoutSlot();
		      return false;
		    }
		}
	    }
	}
    }

  connectionEvent();
  return error() == NoError;
}

bool KStreamSocket::connect(const KResolverEntry& entry)
{
  return KClientSocketBase::connect(entry);
}

void KStreamSocket::hostFoundSlot()
{
  QObject::disconnect(this, SLOT(hostFoundSlot()));
  if (timeout() > 0)
    d->timer.start(timeout(), true);
  QTimer::singleShot(0, this, SLOT(connectionEvent()));
}

void KStreamSocket::connectionEvent()
{
  if (state() != HostFound && state() != Connecting)
    return;			// nothing to do

  const KResolverResults& peer = peerResults();
  if (state() == HostFound)
    {
      d->startTime.start();

      setState(Connecting);
      emit stateChanged(Connecting);
      d->peer = peer.begin();
      d->local = localResults().begin(); // just to be on the safe side
    }

  while (d->peer != peer.end())
    {
      const KResolverEntry &r = *d->peer;

      if (socketDevice()->socket() != -1)
	{
	  // we have an existing file descriptor
	  // this means that we've got activity in it (connection result)
	  if (socketDevice()->connect(r) && socketDevice()->error() == NoError)
	    {
	      // yes, it did connect!
	      connectionSucceeded(r);
	      return;
	    }
	  else if (socketDevice()->error() == InProgress)
	    // nope, still in progress
	    return;

	  // no, the socket failed to connect
	  copyError();
	  socketDevice()->close();
	  ++d->peer;
	  continue;
	}

      // try to bind
      if (!bindLocallyFor(r))
	{
	  // could not find a matching family
	  ++d->peer;
	  continue;
	}

      {
	bool skip = false;
	emit aboutToConnect(r, skip);
	if (skip)
	  {
	    ++d->peer;
	    continue;
	  }
      }

      if (socketDevice()->connect(r) || socketDevice()->error() == InProgress)
	{
	  // socket is attempting to connect
	  if (socketDevice()->error() == InProgress)
	    {
	      QSocketNotifier *n = socketDevice()->readNotifier();
	      QObject::connect(n, SIGNAL(activated(int)),
			       this, SLOT(connectionEvent()));
	      n->setEnabled(true);

	      n = socketDevice()->writeNotifier();
	      QObject::connect(n, SIGNAL(activated(int)),
			       this, SLOT(connectionEvent()));
	      n->setEnabled(true);

	      return;		// wait for activity
	    }

	  // socket has connected
	  continue;		// let the beginning of the loop handle success
	}

      // connection failed
      // try next
      copyError();
      socketDevice()->close();
      ++d->peer;
    }

  // that was the last item
  setState(Idle);
  emit stateChanged(Idle);
  emit gotError(error());
  return;
}

void KStreamSocket::timeoutSlot()
{
  if (state() != Connecting)
    return;

  // halt the connections
  socketDevice()->close();	// this also kills the notifiers

  setError(IO_TimeOutError, Timeout);
  setState(HostFound);
  emit stateChanged(HostFound);
  emit gotError(Timeout);
  emit timedOut();
}

bool KStreamSocket::bindLocallyFor(const KResolverEntry& peer)
{
  const KResolverResults& local = localResults();

  if (local.isEmpty())
    // user doesn't want to bind to any specific local address
    return true;

  bool foundone = false;
  // scan the local resolution for a matching family
  for (d->local = local.begin(); d->local != local.end(); ++d->local)
    if ((*d->local).family() == peer.family())
      {
	// found a suitable address!
	foundone = true;

	if (socketDevice()->bind(*d->local))
	  return true;
      }
  
  if (!foundone)
    {
      // found nothing
      setError(IO_BindError, NotSupported);
      emit gotError(NotSupported);
    }
  else
    copyError();
  return false;
}

void KStreamSocket::connectionSucceeded(const KResolverEntry& peer)
{
  QObject::disconnect(socketDevice()->readNotifier(), 0, this, SLOT(connectionEvent()));
  QObject::disconnect(socketDevice()->writeNotifier(), 0, this, SLOT(connectionEvent()));

  resetError();
  setFlags(IO_Sequential | IO_Raw | IO_ReadWrite | IO_Open | IO_Async);
  setState(Connected);
  socketDevice()->setSocketOptions(socketOptions());
  d->timer.stop();
  emit stateChanged(Connected);
  
  if (!localResults().isEmpty())
    emit bound(*d->local);
  emit connected(peer);
}

#include "kstreamsocket.moc"
