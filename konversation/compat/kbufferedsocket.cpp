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
#include <qtimer.h>

#include "ksocketdevice.h"
#include "ksocketaddress.h"
#include "ksocketbuffer_p.h"
#include "kbufferedsocket.h"

using namespace KNetwork;
using namespace KNetwork::Internal;

class KNetwork::KBufferedSocketPrivate
{
public:
  mutable KSocketBuffer *input, *output;

  KBufferedSocketPrivate()
  {
    input = 0L;
    output = 0L;
  }
};

KBufferedSocket::KBufferedSocket(const QString& host, const QString& service,
				 QObject *parent, const char *name)
  : KStreamSocket(host, service, parent, name),
    d(new KBufferedSocketPrivate)
{
  setBlocking(false);
  setInputBuffering(true);
  setOutputBuffering(true);
}

KBufferedSocket::~KBufferedSocket()
{
  closeNow();
  delete d->input;
  delete d->output;
  delete d;
}

void KBufferedSocket::setSocketDevice(KSocketDevice* device)
{
  KStreamSocket::setSocketDevice(device);
  device->setBlocking(false);
}

bool KBufferedSocket::setSocketOptions(int opts)
{
  if (opts == Blocking)
    return false;

  opts &= ~Blocking;
  return KStreamSocket::setSocketOptions(opts);
}

void KBufferedSocket::close()
{
  if (!d->output || d->output->isEmpty())
    closeNow();
  else
    {
      setState(Closing);
      QSocketNotifier *n = socketDevice()->readNotifier();
      if (n)
	n->setEnabled(false);
      emit stateChanged(Closing);
    }
}

Q_LONG KBufferedSocket::bytesAvailable() const
{
  if (!d->input)
    return KStreamSocket::bytesAvailable();

  return d->input->length();
}

Q_LONG KBufferedSocket::waitForMore(int msecs, bool *timeout)
{
  Q_LONG retval = KStreamSocket::waitForMore(msecs, timeout);
  if (d->input)
    {
      resetError();
      slotReadActivity();
      return bytesAvailable();
    }
  return retval;
}

Q_LONG KBufferedSocket::readBlock(char *data, Q_ULONG maxlen)
{
  if (d->input)
    {
      if (d->input->isEmpty())
	{
	  setError(IO_ReadError, WouldBlock);
	  emit gotError(WouldBlock);
	  return -1;
	}
      resetError();
      return d->input->consumeBuffer(data, maxlen);
    }
  return KStreamSocket::readBlock(data, maxlen);
}

Q_LONG KBufferedSocket::readBlock(char *data, Q_ULONG maxlen, KSocketAddress& from)
{
  from = peerAddress();
  return readBlock(data, maxlen);
}

Q_LONG KBufferedSocket::peekBlock(char *data, Q_ULONG maxlen)
{
  if (d->input)
    {
      if (d->input->isEmpty())
	{
	  setError(IO_ReadError, WouldBlock);
	  emit gotError(WouldBlock);
	  return -1;
	}
      resetError();
      return d->input->consumeBuffer(data, maxlen, false);
    }
  return KStreamSocket::peekBlock(data, maxlen);
}

Q_LONG KBufferedSocket::peekBlock(char *data, Q_ULONG maxlen, KSocketAddress& from)
{
  from = peerAddress();
  return peekBlock(data, maxlen);
}

Q_LONG KBufferedSocket::writeBlock(const char *data, Q_ULONG len)
{
  if (state() != Connected)
    {
      // cannot write now!
      setError(IO_WriteError, NotConnected);
      return -1;
    }

  if (d->output)
    {
      if (d->output->isFull())
	{
	  setError(IO_WriteError, WouldBlock);
	  emit gotError(WouldBlock);
	  return -1;
	}
      resetError();

      // enable notifier to send data
      QSocketNotifier *n = socketDevice()->writeNotifier();
      if (n)
	n->setEnabled(true);

      return d->output->feedBuffer(data, len);
    }

  return KStreamSocket::writeBlock(data, len);
}

Q_LONG KBufferedSocket::writeBlock(const char *data, Q_ULONG maxlen,
				   const KSocketAddress&)
{
  // ignore the third parameter
  return writeBlock(data, maxlen);
}

void KBufferedSocket::enableRead(bool enable)
{
  KStreamSocket::enableRead(enable);
  if (!enable && d->input)
    {
      // reenable it
      QSocketNotifier *n = socketDevice()->readNotifier();
      if (n)
	n->setEnabled(true);
    }

  if (enable && state() != Connected && d->input && !d->input->isEmpty())
    // this means the buffer is still dirty
    // allow the signal to be emitted
    QTimer::singleShot(0, this, SLOT(slotReadActivity()));
}

void KBufferedSocket::enableWrite(bool enable)
{
  KStreamSocket::enableWrite(enable);
  if (!enable && d->output && !d->output->isEmpty())
    {
      // reenable it
      QSocketNotifier *n = socketDevice()->writeNotifier();
      if (n)
	n->setEnabled(true);
    }
}

void KBufferedSocket::stateChanging(SocketState newState)
{
  if (newState == Connecting || newState == Connected)
    {
      // we're going to connect
      // make sure the buffers are clean
      if (d->input)
	d->input->clear();
      if (d->output)
	d->output->clear();

      // also, turn on notifiers
      enableRead(emitsReadyRead());
      enableWrite(emitsReadyWrite());
    }
  KStreamSocket::stateChanging(newState);
}

void KBufferedSocket::setInputBuffering(bool enable)
{
  QMutexLocker locker(mutex());
  if (!enable)
    {
      delete d->input;
      d->input = 0L;
    }
  else if (d->input == 0L)
    {
      d->input = new KSocketBuffer;
    }
}

KIOBufferBase* KBufferedSocket::inputBuffer()
{
  return d->input;
}

void KBufferedSocket::setOutputBuffering(bool enable)
{
  QMutexLocker locker(mutex());
  if (!enable)
    {
      delete d->output;
      d->output = 0L;
    }
  else if (d->output == 0L)
    {
      d->output = new KSocketBuffer;
    }
}

KIOBufferBase* KBufferedSocket::outputBuffer()
{
  return d->output;
}

Q_ULONG KBufferedSocket::bytesToWrite() const
{
  if (!d->output)
    return 0;

  return d->output->length();
}

void KBufferedSocket::closeNow()
{
  KStreamSocket::close();
}

bool KBufferedSocket::canReadLine() const
{
  if (!d->input)
    return false;

  return d->input->canReadLine();
}

QCString KBufferedSocket::readLine()
{
  return d->input->readLine();
}

void KBufferedSocket::slotReadActivity()
{
  if (d->input && state() == Connected)
    {
      mutex()->lock();
      Q_LONG len = d->input->receiveFrom(socketDevice());

      if (len == -1)
	{
	  if (socketDevice()->error() != WouldBlock)
	    {
	      // nope, another error!
	      copyError();
	      mutex()->unlock();
	      closeNow();	// emits closed
	      return;
	    }
	}
      else if (len == 0)
	{
	  // remotely closed
	  resetError();
	  mutex()->unlock();
	  closeNow();		// emits closed
	  return;
	}

      // no error
      mutex()->unlock();
    }

  if (state() == Connected)
    KStreamSocket::slotReadActivity(); // this emits readyRead
  else if (emitsReadyRead())	// state() != Connected
    {
      if (d->input && !d->input->isEmpty())
	{
	  // buffer isn't empty
	  // keep emitting signals till it is
	  QTimer::singleShot(0, this, SLOT(slotReadActivity()));
	  emit readyRead();
	}
    }
}

void KBufferedSocket::slotWriteActivity()
{
  if (d->output && !d->output->isEmpty() &&
      (state() == Connected || state() == Closing))
    {
      mutex()->lock();
      Q_LONG len = d->output->sendTo(socketDevice());

      if (len == -1)
	{
	  if (socketDevice()->error() != WouldBlock)
	    {
	      // nope, another error!
	      copyError();
	      mutex()->unlock();
	      closeNow();
	      return;
	    }
	}
      else if (len == 0)
	{
	  // remotely closed
	  resetError();
	  mutex()->unlock();
	  closeNow();
	  return;
	}

      if (d->output->isEmpty())
	// deactivate the notifier until we have something to send
	// writeNotifier can't return NULL here
	socketDevice()->writeNotifier()->setEnabled(false);

      mutex()->unlock();
      emit bytesWritten(len);
    }

  if (state() != Closing)
    KStreamSocket::slotWriteActivity();
  else if (d->output && d->output->isEmpty() && state() == Closing)
    {
      KStreamSocket::close();	// finished sending data
    }
}

#include "kbufferedsocket.moc"
