/*  -*- C++ -*-
 *  Copyright (C) 2004 Thiago Macieira <thiago.macieira@kdemail.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "ksocks.h"
#include "ksocketaddress.h"
#include "kresolver.h"
#include "ksockssocketdevice.h"

using namespace KNetwork;

// constructor
// nothing to do
KSocksSocketDevice::KSocksSocketDevice(const KSocketBase* obj)
  : KSocketDevice(obj)
{
}

// constructor with argument
// nothing to do
KSocksSocketDevice::KSocksSocketDevice(int fd)
  : KSocketDevice(fd)
{
}

// destructor
// also nothing to do
KSocksSocketDevice::~KSocksSocketDevice()
{
}

// returns the capabilities
int KSocksSocketDevice::capabilities() const
{
  return 0;			// can do everything!
}

// From here on, the code is almost exactly a copy of KSocketDevice
// the differences are the use of KSocks where appropriate

bool KSocksSocketDevice::bind(const KResolverEntry& address)
{
  resetError();

  if (m_sockfd == -1 && !create(address))
    return false;		// failed creating

  // we have a socket, so try and bind
  if (KSocks::self()->bind(m_sockfd, address.address(), address.length()) == -1)
    {
      if (errno == EADDRINUSE)
	setError(IO_BindError, AddressInUse);
      else if (errno == EINVAL)
	setError(IO_BindError, AlreadyBound);
      else
	// assume the address is the cause
	setError(IO_BindError, NotSupported);
      return false;
    }

  return true;
}


bool KSocksSocketDevice::listen(int backlog)
{
  if (m_sockfd != -1)
    {
      if (KSocks::self()->listen(m_sockfd, backlog) == -1)
	{
	  setError(IO_ListenError, NotSupported);
	  return false;
	}

      resetError();
      setFlags(IO_Sequential | IO_Raw | IO_ReadWrite);
      setState(IO_Open);
      return true;
    }

  // we don't have a socket
  // can't listen
  setError(IO_ListenError, NotCreated);
  return false;
}

bool KSocksSocketDevice::connect(const KResolverEntry& address)
{
  resetError();

  if (m_sockfd == -1 && !create(address))
    return false;		// failed creating!

  if (KSocks::self()->connect(m_sockfd, address.address(), address.length()) == -1)
    {
      if (errno == EISCONN)
	return true;		// we're already connected
      else if (errno == EALREADY || errno == EINPROGRESS)
	{
	  setError(IO_ConnectError, InProgress);
	  return true;
	}
      else if (errno == ECONNREFUSED)
	setError(IO_ConnectError, ConnectionRefused);
      else if (errno == ENETDOWN || errno == ENETUNREACH ||
	       errno == ENETRESET || errno == ECONNABORTED ||
	       errno == ECONNRESET || errno == EHOSTDOWN ||
	       errno == EHOSTUNREACH)
	setError(IO_ConnectError, NetFailure);
      else
	setError(IO_ConnectError, NotSupported);

      return false;
    }

  setFlags(IO_Sequential | IO_Raw | IO_ReadWrite);
  setState(IO_Open);
  return true;			// all is well
}

KSocksSocketDevice* KSocksSocketDevice::accept()
{
  if (m_sockfd == -1)
    {
      // can't accept without a socket
      setError(IO_AcceptError, NotCreated);
      return 0L;
    }

  struct sockaddr sa;
  socklen_t len = sizeof(sa);
  int newfd = KSocks::self()->accept(m_sockfd, &sa, &len);
  if (newfd == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	setError(IO_AcceptError, WouldBlock);
      else
	setError(IO_AcceptError, UnknownError);
      return NULL;
    }

  return new KSocksSocketDevice(newfd);
}

static int socks_read_common(int sockfd, char *data, Q_ULONG maxlen, KNetwork::KSocketAddress* from, ssize_t &retval, bool peek = false)
{
  socklen_t len;
  if (from)
    {
      from->setLength(len = 128); // arbitrary length
      retval = KSocks::self()->recvfrom(sockfd, data, maxlen, peek ? MSG_PEEK : 0, from->address(), &len);
    }
  else
    retval = KSocks::self()->recvfrom(sockfd, data, maxlen, peek ? MSG_PEEK : 0, NULL, NULL);

  if (retval == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	return KSocketDevice::WouldBlock;
      else
	return KSocketDevice::UnknownError;
    }

  if (from)
    from->setLength(len);
  return 0;
}

Q_LONG KSocksSocketDevice::readBlock(char *data, Q_ULONG maxlen)
{
  resetError();
  if (m_sockfd == -1)
    return -1;

  if (maxlen == 0 || data == 0L)
    return 0;			// can't read

  ssize_t retval;
  int err = socks_read_common(m_sockfd, data, maxlen, 0L, retval);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocksSocketDevice::readBlock(char *data, Q_ULONG maxlen, KSocketAddress &from)
{
  resetError();
  if (m_sockfd == -1)
    return -1;			// nothing to do here

  if (data == 0L || maxlen == 0)
    return 0;			// user doesn't want to read

  ssize_t retval;
  int err = socks_read_common(m_sockfd, data, maxlen, &from, retval);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocksSocketDevice::peekBlock(char *data, Q_ULONG maxlen)
{
  resetError();
  if (m_sockfd == -1)
    return -1;

  if (maxlen == 0 || data == 0L)
    return 0;			// can't read

  ssize_t retval;
  int err = socks_read_common(m_sockfd, data, maxlen, 0L, retval, true);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocksSocketDevice::peekBlock(char *data, Q_ULONG maxlen, KSocketAddress& from)
{
  resetError();
  if (m_sockfd == -1)
    return -1;			// nothing to do here

  if (data == 0L || maxlen == 0)
    return 0;			// user doesn't want to read

  ssize_t retval;
  int err = socks_read_common(m_sockfd, data, maxlen, &from, retval, true);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocksSocketDevice::writeBlock(const char *data, Q_ULONG len)
{
  return writeBlock(data, len, KSocketAddress());
}

Q_LONG KSocksSocketDevice::writeBlock(const char *data, Q_ULONG len, const KSocketAddress& to)
{
  resetError();
  if (m_sockfd == -1)
    return -1;			// can't write to unopen socket

  if (data == 0L || len == 0)
    return 0;			// nothing to be written

  ssize_t retval = KSocks::self()->sendto(m_sockfd, data, len, 0, to.address(), to.length());
  if (retval == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	setError(IO_WriteError, WouldBlock);
      else
	setError(IO_WriteError, UnknownError);
      return -1;		// nothing written
    }

  return retval;
}

KNetwork::KSocketAddress KSocksSocketDevice::localAddress() const
{
  if (m_sockfd == -1)
    return KSocketAddress();	// not open, empty value

  socklen_t len;
  KSocketAddress localAddress;
  localAddress.setLength(len = 32);	// arbitrary value
  if (KSocks::self()->getsockname(m_sockfd, localAddress.address(), &len) == -1)
    // error!
    return KSocketAddress();

  if (len <= localAddress.length())
    {
      // it has fit already
      localAddress.setLength(len);
      return localAddress;
    }

  // no, the socket address is actually larger than we had anticipated
  // call again
  localAddress.setLength(len);
  if (KSocks::self()->getsockname(m_sockfd, localAddress.address(), &len) == -1)
    // error!
    return KSocketAddress();

  return localAddress;
}

KNetwork::KSocketAddress KSocksSocketDevice::peerAddress() const
{
  if (m_sockfd == -1)
    return KSocketAddress();	// not open, empty value

  socklen_t len;
  KSocketAddress peerAddress;
  peerAddress.setLength(len = 32);	// arbitrary value
  if (KSocks::self()->getpeername(m_sockfd, peerAddress.address(), &len) == -1)
    // error!
    return KSocketAddress();

  if (len <= peerAddress.length())
    {
      // it has fit already
      peerAddress.setLength(len);
      return peerAddress;
    }

  // no, the socket address is actually larger than we had anticipated
  // call again
  peerAddress.setLength(len);
  if (KSocks::self()->getpeername(m_sockfd, peerAddress.address(), &len) == -1)
    // error!
    return KSocketAddress();

  return peerAddress;
}

KNetwork::KSocketAddress KSocksSocketDevice::externalAddress() const
{
  // return empty, indicating unknown external address
  return KSocketAddress();
}

bool KSocksSocketDevice::poll(bool *input, bool *output, bool *exception,
			      int timeout, bool *timedout)
{
  if (m_sockfd == -1)
    {
      setError(IO_UnspecifiedError, NotCreated);
      return false;
    }

  resetError();
  fd_set readfds, writefds, exceptfds;
  fd_set *preadfds = 0L, *pwritefds = 0L, *pexceptfds = 0L;

  if (input)
    {
      preadfds = &readfds;
      FD_ZERO(preadfds);
      FD_SET(m_sockfd, preadfds);
      *input = false;
    }
  if (output)
    {
      pwritefds = &writefds;
      FD_ZERO(pwritefds);
      FD_SET(m_sockfd, pwritefds);
      *output = false;
    }
  if (exception)
    {
      pexceptfds = &exceptfds;
      FD_ZERO(pexceptfds);
      FD_SET(m_sockfd, pexceptfds);
      *exception = false;
    }

  int retval;
  if (timeout < 0)
    retval = KSocks::self()->select(m_sockfd + 1, preadfds, pwritefds, pexceptfds, 0L);
  else
    {
      // convert the milliseconds to timeval
      struct timeval tv;
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000 * 1000;

      retval = select(m_sockfd + 1, preadfds, pwritefds, pexceptfds, &tv);
    }

  if (retval == -1)
    {
      setError(IO_UnspecifiedError, UnknownError);
      return false;
    }
  if (retval == 0)
    {
      // timeout
      if (timedout)
	*timedout = true;
      return true;
    }

  if (input && FD_ISSET(m_sockfd, preadfds))
    *input = true;
  if (output && FD_ISSET(m_sockfd, pwritefds))
    *output = true;
  if (exception && FD_ISSET(m_sockfd, pexceptfds))
    *exception = true;

  return true;
}
