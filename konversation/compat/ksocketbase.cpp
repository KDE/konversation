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
#include <qmutex.h>
#include "klocale.h"

#include "ksocketbase.h"
#include "ksocketdevice.h"

using namespace KNetwork;

class KNetwork::KSocketBasePrivate
{
public:
  int socketOptions;
  int socketError;
  int capabilities;

  mutable KSocketDevice* device;

  QMutex mutex;

  KSocketBasePrivate()
    : mutex(true)		// create recursive
  { }
};

KSocketBase::KSocketBase()
  : d(new KSocketBasePrivate)
{
  d->socketOptions = Blocking;
  d->socketError = 0;
  d->device = 0L;
  d->capabilities = 0;
}

KSocketBase::~KSocketBase()
{
  delete d->device;
  delete d;
}

bool KSocketBase::setSocketOptions(int opts)
{
  d->socketOptions = opts;
  return true;
}

int KSocketBase::socketOptions() const
{
  return d->socketOptions;
}

bool KSocketBase::setBlocking(bool enable)
{
  return setSocketOptions((socketOptions() & ~Blocking) | (enable ? Blocking : 0));
}

bool KSocketBase::blocking() const
{
  return socketOptions() & Blocking;
}

bool KSocketBase::setAddressReuseable(bool enable)
{
  return setSocketOptions((socketOptions() & ~AddressReuseable) | (enable ? AddressReuseable : 0));
}

bool KSocketBase::addressReuseable() const
{
  return socketOptions() & AddressReuseable;
}

bool KSocketBase::setIPv6Only(bool enable)
{
  return setSocketOptions((socketOptions() & ~IPv6Only) | (enable ? IPv6Only : 0));
}

bool KSocketBase::isIPv6Only() const
{
  return socketOptions() & IPv6Only;
}

bool KSocketBase::setBroadcast(bool enable)
{
  return setSocketOptions((socketOptions() & ~Broadcast) | (enable ? Broadcast : 0));
}

bool KSocketBase::broadcast() const
{
  return socketOptions() & Broadcast;
}

KSocketDevice* KSocketBase::socketDevice() const
{
  if (d->device)
    return d->device;

  // it doesn't exist, so create it
  QMutexLocker locker(mutex());
  if (d->device)
    return d->device;

  KSocketBase* that = const_cast<KSocketBase*>(this);
  KSocketDevice* dev = 0;
  if (d->capabilities)
    dev = KSocketDevice::createDefault(that, d->capabilities);
  if (!dev)
    dev = KSocketDevice::createDefault(that);
  that->setSocketDevice(dev);
  return d->device;
}

void KSocketBase::setSocketDevice(KSocketDevice* device)
{
  QMutexLocker locker(mutex());
  if (d->device == 0L)
    d->device = device;
}

int KSocketBase::setRequestedCapabilities(int add, int remove)
{
  d->capabilities |= add;
  d->capabilities &= ~remove;
  return d->capabilities;
}

bool KSocketBase::hasDevice() const
{
  return d->device != 0L;
}

void KSocketBase::setError(SocketError error)
{
  d->socketError = error;
}

KSocketBase::SocketError KSocketBase::error() const
{
  return static_cast<KSocketBase::SocketError>(d->socketError);
}

// static
QString KSocketBase::errorString(KSocketBase::SocketError code)
{
  QString reason;
  switch (code)
    {
    case NoError:
      reason = i18n("Socket error code NoError", "no error");
      break;

    case LookupFailure:
      reason = i18n("Socket error code LookupFailure",
		    "name lookup has failed");
      break;

    case AddressInUse:
      reason = i18n("Socket error code AddressInUse",
		    "address already in use");
      break;

    case AlreadyBound:
      reason = i18n("Socket error code AlreadyBound",
		    "socket is already bound");
      break;

    case AlreadyCreated:
      reason = i18n("Socket error code AlreadyCreated",
		    "socket is already created");
      break;
      
    case NotBound:
      reason = i18n("Socket error code NotBound",
		    "socket is not bound");
      break;

    case NotCreated:
      reason = i18n("Socket error code NotCreated",
		    "socket has not been created");
      break;

    case WouldBlock:
      reason = i18n("Socket error code WouldBlock",
		    "operation would block");
      break;

    case ConnectionRefused:
      reason = i18n("Socket error code ConnectionRefused",
		    "connection actively refused");
      break;

    case ConnectionTimedOut:
      reason = i18n("Socket error code ConnectionTimedOut",
		    "connection timed out");
      break;

    case InProgress:
      reason = i18n("Socket error code InProgress",
		    "operation is already in progress");
      break;

    case NetFailure:
      reason = i18n("Socket error code NetFailure",
		    "network failure occurred");
      break;

    case NotSupported:
      reason = i18n("Socket error code NotSupported",
		    "operation is not supported");
      break;

    case Timeout:
      reason = i18n("Socket error code Timeout",
		    "timed operation timed out");
      break;

    case UnknownError:
      reason = i18n("Socket error code UnknownError",
		    "an unknown/unexpected error has happened");
      break;

    default:
      reason = QString::null;
      break;
    }

  return reason;
}

// static
bool KSocketBase::isFatalError(int code)
{
  switch (code)
    {
    case WouldBlock:
    case InProgress:
      return false;
    }

  return true;
}

void KSocketBase::unsetSocketDevice()
{
  d->device = 0L;
}

QMutex* KSocketBase::mutex() const
{
  return &d->mutex;
}

KActiveSocketBase::KActiveSocketBase()
{
}

KActiveSocketBase::~KActiveSocketBase()
{
}

int KActiveSocketBase::getch()
{
  unsigned char c;
  if (readBlock((char*)&c, 1) != 1)
    return -1;

  return c;
}

int KActiveSocketBase::putch(int ch)
{
  unsigned char c = (unsigned char)ch;
  if (writeBlock((char*)&c, 1) != 1)
    return -1;

  return c;
}

void KActiveSocketBase::setError(int status, SocketError error)
{
  KSocketBase::setError(error);
  setStatus(status);
}

void KActiveSocketBase::resetError()
{
  KSocketBase::setError(NoError);
  resetStatus();
}

KPassiveSocketBase::KPassiveSocketBase()
{
}

KPassiveSocketBase::~KPassiveSocketBase()
{
}
