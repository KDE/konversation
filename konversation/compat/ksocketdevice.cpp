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

#include <qmap.h>

#ifdef USE_SOLARIS
# include <sys/filio.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

#ifdef HAVE_POLL
# include <sys/poll.h>
#else
# ifdef HAVE_SYS_SELECT
#  include <sys/select.h>
# endif
#endif

// Include syssocket before our local includes
#include "syssocket.h"

#include <qmutex.h>
#include <qsocketnotifier.h>

#include "kresolver.h"
#include "ksocketaddress.h"
#include "ksocketbase.h"
#include "ksocketdevice.h"
#include "ksockssocketdevice.h"

using namespace KNetwork;

class KNetwork::KSocketDevicePrivate
{
public:
  mutable QSocketNotifier *input, *output, *exception;
  int af;

  inline KSocketDevicePrivate()
  {
    input = output = exception = 0L;
  }
};


KSocketDevice::KSocketDevice(const KSocketBase* parent)
  : m_sockfd(-1), d(new KSocketDevicePrivate)
{
  setSocketDevice(this);
  if (parent)
    setSocketOptions(parent->socketOptions());
}

KSocketDevice::KSocketDevice(int fd)
  : m_sockfd(fd), d(new KSocketDevicePrivate)
{
  setState(IO_Open);
  setFlags(IO_Sequential | IO_Raw | IO_ReadWrite);
  setSocketDevice(this);
}

KSocketDevice::KSocketDevice(bool, const KSocketBase* parent)
  : m_sockfd(-1), d(new KSocketDevicePrivate)
{
  // do not set parent
  if (parent)
    setSocketOptions(parent->socketOptions());
}

KSocketDevice::~KSocketDevice()
{
  close();			// deletes the notifiers
  unsetSocketDevice(); 		// prevent double deletion
  delete d;
}

bool KSocketDevice::setSocketOptions(int opts)
{
  // must call parent
  QMutexLocker locker(mutex());
  KSocketBase::setSocketOptions(opts);

  if (m_sockfd == -1)
    return true;		// flags are stored

    {
      int fdflags = fcntl(m_sockfd, F_GETFL, 0);
      if (fdflags == -1)
	{
	  setError(IO_UnspecifiedError, UnknownError);
	  return false;		// error
	}

      if (opts & Blocking)
	fdflags &= ~O_NONBLOCK;
      else
	fdflags |= O_NONBLOCK;

      if (fcntl(m_sockfd, F_SETFL, fdflags) == -1)
	{
	  setError(IO_UnspecifiedError, UnknownError);
	  return false;		// error
	}
    }

    {
      int on = opts & AddressReuseable ? 1 : 0;
      if (setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) == -1)
	{
	  setError(IO_UnspecifiedError, UnknownError);
	  return false;		// error
	}
    }

#if defined(IPV6_V6ONLY) && defined(AF_INET6)
  if (d->af == AF_INET6)
    {
      // don't try this on non-IPv6 sockets, or we'll get an error

      int on = opts & IPv6Only ? 1 : 0;
      if (setsockopt(m_sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&on, sizeof(on)) == -1)
	{
	  setError(IO_UnspecifiedError, UnknownError);
	  return false;		// error
	}
    }
#endif

   {
     int on = opts & Broadcast ? 1 : 0;
     if (setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on)) == -1)
       {
	 setError(IO_UnspecifiedError, UnknownError);
	 return false;		// error
       }
   }

  return true;			// all went well
}

bool KSocketDevice::open(int)
{
  resetError();
  return false;
}

void KSocketDevice::close()
{
  resetError();
  if (m_sockfd != -1)
    {
      delete d->input;
      delete d->output;
      delete d->exception;

      d->input = d->output = d->exception = 0L;

      ::close(m_sockfd);
    }
  setState(0);

  m_sockfd = -1;
}

bool KSocketDevice::create(int family, int type, int protocol)
{
  resetError();

  if (m_sockfd != -1)
    {
      // it's already created!
      setError(IO_SocketCreateError, AlreadyCreated);
      return false;
    }

  // no socket yet; we have to create it
  m_sockfd = kde_socket(family, type, protocol);

  if (m_sockfd == -1)
    {
      setError(IO_SocketCreateError, NotSupported);
      return false;
    }

  d->af = family;
  setSocketOptions(socketOptions());
  return true;		// successfully created
}

bool KSocketDevice::create(const KResolverEntry& address)
{
  return create(address.family(), address.socketType(), address.protocol());
}

bool KSocketDevice::bind(const KResolverEntry& address)
{
  resetError();

  if (m_sockfd == -1 && !create(address))
    return false;		// failed creating

  // we have a socket, so try and bind
  if (kde_bind(m_sockfd, address.address(), address.length()) == -1)
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

bool KSocketDevice::listen(int backlog)
{
  if (m_sockfd != -1)
    {
      if (kde_listen(m_sockfd, backlog) == -1)
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

bool KSocketDevice::connect(const KResolverEntry& address)
{
  resetError();

  if (m_sockfd == -1 && !create(address))
    return false;		// failed creating!

  if (kde_connect(m_sockfd, address.address(), address.length()) == -1)
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

KSocketDevice* KSocketDevice::accept()
{
  if (m_sockfd == -1)
    {
      // can't accept without a socket
      setError(IO_AcceptError, NotCreated);
      return 0L;
    }

  struct sockaddr sa;
  socklen_t len = sizeof(sa);
  int newfd = kde_accept(m_sockfd, &sa, &len);
  if (newfd == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	setError(IO_AcceptError, WouldBlock);
      else
	setError(IO_AcceptError, UnknownError);
      return NULL;
    }

  return new KSocketDevice(newfd);
}

bool KSocketDevice::disconnect()
{
  resetError();

  if (m_sockfd == -1)
    return false;		// can't create

  KSocketAddress address;
  address.setFamily(AF_UNSPEC);
  if (kde_connect(m_sockfd, address.address(), address.length()) == -1)
    {
      if (errno == EALREADY || errno == EINPROGRESS)
	{
	  setError(IO_ConnectError, InProgress);
	  return false;
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

Q_LONG KSocketDevice::bytesAvailable() const
{
  if (m_sockfd == -1)
    return -1;			// there's nothing to read in a closed socket

  int nchars;
  if (ioctl(m_sockfd, FIONREAD, &nchars) == -1)
    return -1;			// error!

  return nchars;
}

Q_LONG KSocketDevice::waitForMore(int msecs, bool *timeout)
{
  if (m_sockfd == -1)
    return -1;			// there won't ever be anything to read...

  bool input;
  if (!poll(&input, 0, 0, msecs, timeout))
    return -1;			// failed polling

  return bytesAvailable();
}

static int do_read_common(int sockfd, char *data, Q_ULONG maxlen, KSocketAddress* from, ssize_t &retval, bool peek = false)
{
  socklen_t len;
  if (from)
    {
      from->setLength(len = 128); // arbitrary length
      retval = ::recvfrom(sockfd, data, maxlen, peek ? MSG_PEEK : 0, from->address(), &len);
    }
  else
    retval = ::recvfrom(sockfd, data, maxlen, peek ? MSG_PEEK : 0, NULL, NULL);

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

Q_LONG KSocketDevice::readBlock(char *data, Q_ULONG maxlen)
{
  resetError();
  if (m_sockfd == -1)
    return -1;

  if (maxlen == 0 || data == 0L)
    return 0;			// can't read

  ssize_t retval;
  int err = do_read_common(m_sockfd, data, maxlen, 0L, retval);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocketDevice::readBlock(char *data, Q_ULONG maxlen, KSocketAddress &from)
{
  resetError();
  if (m_sockfd == -1)
    return -1;			// nothing to do here

  if (data == 0L || maxlen == 0)
    return 0;			// user doesn't want to read

  ssize_t retval;
  int err = do_read_common(m_sockfd, data, maxlen, &from, retval);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocketDevice::peekBlock(char *data, Q_ULONG maxlen)
{
  resetError();
  if (m_sockfd == -1)
    return -1;

  if (maxlen == 0 || data == 0L)
    return 0;			// can't read

  ssize_t retval;
  int err = do_read_common(m_sockfd, data, maxlen, 0L, retval, true);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocketDevice::peekBlock(char *data, Q_ULONG maxlen, KSocketAddress& from)
{
  resetError();
  if (m_sockfd == -1)
    return -1;			// nothing to do here

  if (data == 0L || maxlen == 0)
    return 0;			// user doesn't want to read

  ssize_t retval;
  int err = do_read_common(m_sockfd, data, maxlen, &from, retval, true);

  if (err)
    {
      setError(IO_ReadError, static_cast<SocketError>(err));
      return -1;
    }

  return retval;
}

Q_LONG KSocketDevice::writeBlock(const char *data, Q_ULONG len)
{
  return writeBlock(data, len, KSocketAddress());
}

Q_LONG KSocketDevice::writeBlock(const char *data, Q_ULONG len, const KSocketAddress& to)
{
  resetError();
  if (m_sockfd == -1)
    return -1;			// can't write to unopen socket

  if (data == 0L || len == 0)
    return 0;			// nothing to be written

  ssize_t retval = ::sendto(m_sockfd, data, len, 0, to.address(), to.length());
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

KSocketAddress KSocketDevice::localAddress() const
{
  if (m_sockfd == -1)
    return KSocketAddress();	// not open, empty value

  socklen_t len;
  KSocketAddress localAddress;
  localAddress.setLength(len = 32);	// arbitrary value
  if (kde_getsockname(m_sockfd, localAddress.address(), &len) == -1)
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
  if (kde_getsockname(m_sockfd, localAddress.address(), &len) == -1)
    // error!
    return KSocketAddress();

  return localAddress;
}

KSocketAddress KSocketDevice::peerAddress() const
{
  if (m_sockfd == -1)
    return KSocketAddress();	// not open, empty value

  socklen_t len;
  KSocketAddress peerAddress;
  peerAddress.setLength(len = 32);	// arbitrary value
  if (kde_getpeername(m_sockfd, peerAddress.address(), &len) == -1)
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
  if (kde_getpeername(m_sockfd, peerAddress.address(), &len) == -1)
    // error!
    return KSocketAddress();

  return peerAddress;
}

KSocketAddress KSocketDevice::externalAddress() const
{
  // for normal sockets, the externally visible address is the same
  // as the local address
  return localAddress();
}

QSocketNotifier* KSocketDevice::readNotifier() const
{
  if (d->input)
    return d->input;

  QMutexLocker locker(mutex());
  if (d->input)
    return d->input;

  if (m_sockfd == -1)
    {
      // socket doesn't exist; can't create notifier
      return 0L;
    }

  return d->input = createNotifier(QSocketNotifier::Read);
}

QSocketNotifier* KSocketDevice::writeNotifier() const
{
  if (d->output)
    return d->output;

  QMutexLocker locker(mutex());
  if (d->output)
    return d->output;

  if (m_sockfd == -1)
    {
      // socket doesn't exist; can't create notifier
      return 0L;
    }

  return d->output = createNotifier(QSocketNotifier::Write);
}

QSocketNotifier* KSocketDevice::exceptionNotifier() const
{
  if (d->exception)
    return d->exception;

  QMutexLocker locker(mutex());
  if (d->exception)
    return d->exception;

  if (m_sockfd == -1)
    {
      // socket doesn't exist; can't create notifier
      return 0L;
    }

  return d->exception = createNotifier(QSocketNotifier::Exception);
}

bool KSocketDevice::poll(bool *input, bool *output, bool *exception,
			 int timeout, bool* timedout)
{
  if (m_sockfd == -1)
    {
      setError(IO_UnspecifiedError, NotCreated);
      return false;
    }

  resetError();
#ifdef HAVE_POLL
  struct pollfd fds;
  fds.fd = m_sockfd;
  fds.events = 0;

  if (input)
    {
      fds.events |= POLLIN;
      *input = false;
    }
  if (output)
    {
      fds.events |= POLLOUT;
      *output = false;
    }
  if (exception)
    {
      fds.events |= POLLPRI;
      *exception = false;
    }

  int retval = ::poll(&fds, 1, timeout);
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

  if (input && fds.revents & POLLIN)
    *input = true;
  if (output && fds.revents & POLLOUT)
    *output = true;
  if (exception && fds.revents & POLLPRI)
    *exception = true;

  return true;
#else
  /*
   * We don't have poll(2). We'll have to make do with select(2).
   */

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
    retval = select(m_sockfd + 1, preadfds, pwritefds, pexceptfds, 0L);
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
#endif
}

bool KSocketDevice::poll(int timeout, bool *timedout)
{
  bool input, output, exception;
  return poll(&input, &output, &exception, timeout, timedout);
}

QSocketNotifier* KSocketDevice::createNotifier(QSocketNotifier::Type type) const
{
  if (m_sockfd == -1)
    return 0L;

  return new QSocketNotifier(m_sockfd, type);
}

namespace
{
  // simple class to avoid pointer stuff
  template<class T> class ptr
  {
    typedef T type;
    type* obj;
  public:
    ptr() : obj(0)
    { }

    ptr(const ptr<T>& other) : obj(other.obj)
    { }

    ptr(type* _obj) : obj(_obj)
    { }

    ~ptr()
    { }

    ptr<T>& operator=(const ptr<T>& other)
    { obj = other.obj; return *this; }

    ptr<T>& operator=(T* _obj)
    { obj = _obj; return  *this; }

    type* operator->() const { return obj; }

    operator T*() const { return obj; }

    bool isNull() const
    { return obj == 0; }
  };

  static KSocketDeviceFactoryBase* defaultImplFactory;
  static QMutex defaultImplFactoryMutex;
  typedef QMap<int, KSocketDeviceFactoryBase* > factoryMap;
  static factoryMap factories;
 
  KSocketDevice* KSocketDevice::createDefault(KSocketBase* parent)
  {
    KSocketDevice* device = dynamic_cast<KSocketDevice*>(parent);
    if (device != 0L)
      return device;

    KSocksSocketDevice::initSocks();

    if (defaultImplFactory)
      return defaultImplFactory->create(parent);

    // the really default
    return new KSocketDevice(parent);
  }

  KSocketDevice* KSocketDevice::createDefault(KSocketBase* parent, int capabilities)
  {
    KSocketDevice* device = dynamic_cast<KSocketDevice*>(parent);
    if (device != 0L)
      return device;

    QMutexLocker locker(&defaultImplFactoryMutex);
    factoryMap::ConstIterator it = factories.constBegin();
    for ( ; it != factories.constEnd(); ++it)
      if ((it.key() & capabilities) == capabilities)
	// found a match
	return it.data()->create(parent);

    return 0L;			// no default
  }

  KSocketDeviceFactoryBase*
  KSocketDevice::setDefaultImpl(KSocketDeviceFactoryBase* factory)
  {
    QMutexLocker locker(&defaultImplFactoryMutex);
    KSocketDeviceFactoryBase* old = defaultImplFactory;
    defaultImplFactory = factory;
    return old;
  }

  void KSocketDevice::addNewImpl(KSocketDeviceFactoryBase* factory, int capabilities)
  {
    QMutexLocker locker(&defaultImplFactoryMutex);
    if (factories.contains(capabilities))
      delete factories[capabilities];
    factories.insert(capabilities, factory);
  }

}
