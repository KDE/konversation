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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <qfile.h>
#include <qobject.h>

#include "ksocketaddress.h"

#include "netsupp.h"

using namespace KNetwork;

#if 0
class KIpAddress_localhostV4 : public KIpAddress
{
public:
  KIpAddress_localhostV4()
  {
    *m_data = htonl(0x7f000001);
    m_version = 4;
  }
};

class KIpAddress_localhostV6 : public KIpAddress
{
public:
  KIpAddress_localhostV6()
    : KIpAddress(0L, 6)
  {
    m_data[3] = htonl(1);
  }
};
#endif

static const char localhostV4_data[] = { 127, 0, 0, 1 };
static const char localhostV6_data[] = { 0,0, 0,0,  0,0, 0,0,  0,0, 0,0,  0,0, 0,1 };

const KIpAddress KIpAddress::localhostV4(&localhostV4_data, 4);
const KIpAddress KIpAddress::localhostV6(&localhostV6_data, 6);
const KIpAddress KIpAddress::anyhostV4(0L, 4);
const KIpAddress KIpAddress::anyhostV6(0L, 6);

// helper function to test if an IPv6 v4-mapped address is equal to its IPv4 counterpart
static bool check_v4mapped(const Q_UINT32* v6addr, Q_UINT32 v4addr)
{
  // check that the v6 is a v4-mapped address
  if (!(v6addr[0] == 0 && v6addr[1] == 0 && v6addr[2] == htonl(0x0000ffff)))
    return false;		// not a v4-mapped address

  return v6addr[3] == v4addr;
}

// copy operator
KIpAddress& KIpAddress::operator =(const KIpAddress& other)
{
  m_version = other.m_version;
  if (m_version == 4 || m_version == 6)
    memcpy(m_data, other.m_data, sizeof(m_data));
  return *this;
}

// comparison
bool KIpAddress::compare(const KIpAddress& other, bool checkMapped) const
{
  if (m_version == other.m_version)
    switch (m_version)
      {
      case 0:
	// both objects are empty
	return true;

      case 4:
	// IPv4 address
	return *m_data == *other.m_data;

      case 6:
	// IPv6 address
	// they are 128-bit long, that is, 16 bytes
	return memcmp(m_data, other.m_data, 16) == 0;
      }

  if (checkMapped)
    {
      // check the possibility of a v4-mapped address being compared to an IPv4 one
      if (m_version == 6 && other.m_version == 4 && check_v4mapped(m_data, *other.m_data))
	return true;
      
      if (other.m_version == 6 && m_version == 4 && check_v4mapped(other.m_data, *m_data))
	return true;
    }

  return false;
}

// sets the address to the given address
bool KIpAddress::setAddress(const QString& address)
{
  m_version = 0;

  // try to guess the address version
  if (address.find(':') != -1)
    {
#ifdef AF_INET6
      // guessing IPv6

      Q_UINT32 buf[4];
      if (inet_pton(AF_INET6, address.latin1(), buf))
	{
	  memcpy(m_data, buf, sizeof(m_data));
	  m_version = 6;
	  return true;
	}
#endif

      return false;
    }
  else
    {
      Q_UINT32 buf;
      if (inet_pton(AF_INET, address.latin1(), &buf))
	{
	  *m_data = buf;
	  m_version = 4;
	  return true;
	}

      return false;
    }

  return false;			// can never happen!
}

bool KIpAddress::setAddress(const char* address)
{
  return setAddress(QString::fromLatin1(address));
}

// set from binary data
bool KIpAddress::setAddress(const void* raw, int version)
{
  // this always succeeds
  // except if version is invalid
  if (version != 4 && version != 6)
    return false;

  m_version = version;
  if (raw != 0L)
    memcpy(m_data, raw, version == 4 ? 4 : 16);
  else
    memset(m_data, 0, 16);

  return true;
}

// presentation form
QString KIpAddress::toString() const
{
  char buf[sizeof "1111:2222:3333:4444:5555:6666:255.255.255.255" + 2];
  buf[0] = '\0';
  switch (m_version)
    {
    case 4:
      inet_ntop(AF_INET, m_data, buf, sizeof(buf) - 1);
      return QString::fromLatin1(buf);

    case 6:
#ifdef AF_INET6
      inet_ntop(AF_INET6, m_data, buf, sizeof(buf) - 1);
#endif
      return QString::fromLatin1(buf);
    }

  return QString::null;
}

/*
 * An IPv6 socket address
 * This is taken from RFC 2553.
 */
struct our_sockaddr_in6
{
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
  Q_UINT8		sin6_len;
  Q_UINT8		sin6_family;
# else  //!HAVE_STRUCT_SOCKADDR_SA_LEN
  Q_UINT16		sin6_family;
# endif
  Q_UINT16       	sin6_port;	/* RFC says in_port_t */
  Q_UINT32		sin6_flowinfo;
  Q_UINT8		sin6_addr[16]; // 24 bytes up to here
  Q_UINT32		sin6_scope_id; // 28 bytes total
};

// useful definitions
#define MIN_SOCKADDR_LEN	sizeof(Q_UINT16)
#define SOCKADDR_IN_LEN		sizeof(sockaddr_in)
#define MIN_SOCKADDR_IN6_LEN	((unsigned) &(((our_sockaddr_in6*)0)->sin6_scope_id))
#define SOCKADDR_IN6_LEN	sizeof(our_sockaddr_in6)
#define MIN_SOCKADDR_UN_LEN	(sizeof(Q_UINT16) + sizeof(char))


class KNetwork::KSocketAddressData
{
public:
  /*
   * Note: maybe this should be virtual
   * But since the data is shared via the d pointer, it doesn't really matter
   * what one class sees, so will the other
   */
  class QMixSocketAddressRef : public KInetSocketAddress, public KUnixSocketAddress
  {
  public:
    QMixSocketAddressRef(KSocketAddressData* d)
      : KInetSocketAddress(d), KUnixSocketAddress(d)
    {
    }
  };
  QMixSocketAddressRef ref;

  union
  {
    struct sockaddr         *generic;
    struct sockaddr_in      *in;
    struct our_sockaddr_in6 *in6;
    struct sockaddr_un      *un;
  } addr;
  Q_UINT16 curlen, reallen;

  KSocketAddressData()
    : ref(this)
  {
    addr.generic = 0L;
    curlen = 0;
    invalidate();
  }

  ~KSocketAddressData()
  {
    if (addr.generic != 0L)
      free(addr.generic);
  }

  inline bool invalid() const
  { return reallen == 0; }

  inline void invalidate()
  { reallen = 0; }

  void dup(const sockaddr* sa, Q_UINT16 len, bool clear = true);

  void makeipv4()
  {
    short oldport = 0;
    if (!invalid())
      switch (addr.generic->sa_family)
	{
	case AF_INET:
	  return;		// nothing to do here
#ifdef AF_INET6
	case AF_INET6:
	  oldport = addr.in6->sin6_port;
	  break;
#endif
	}

    // create new space
    dup(0L, SOCKADDR_IN_LEN);

    addr.in->sin_family = AF_INET;
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    addr.in->sin_len = SOCKADDR_IN_LEN;
#endif
    addr.in->sin_port = oldport;
  }

  void makeipv6()
  {
    short oldport = 0;
    if (!invalid())
      switch (addr.generic->sa_family)
	{
	case AF_INET:
	  oldport = addr.in->sin_port;
	  break;

#ifdef AF_INET6
	case AF_INET6:
	  return;		// nothing to do here
#endif
	}

    // make room
    dup(0L, SOCKADDR_IN6_LEN);
#ifdef AF_INET6
    addr.in6->sin6_family = AF_INET6;
#endif
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    addr.in6->sin6_len = SOCKADDR_IN6_LEN;
#endif
    addr.in6->sin6_port = oldport;
    // sin6_scope_id and sin6_flowid are zero
  }

};

// create duplicates of
void KSocketAddressData::dup(const sockaddr* sa, Q_UINT16 len, bool clear)
{
  if (len < MIN_SOCKADDR_LEN)
    {
      // certainly invalid
      invalidate();
      return;
    }

  if (sa && ((sa->sa_family == AF_INET && len < SOCKADDR_IN_LEN) ||
#ifdef AF_INET6
	     (sa->sa_family == AF_INET6 && len < MIN_SOCKADDR_IN6_LEN) ||
#endif
	     (sa->sa_family == AF_UNIX && len < MIN_SOCKADDR_UN_LEN)))
    {
      // also invalid
      invalidate();
      return;
    }

  // good
  reallen = len;
  if (len > curlen)
    {
      if (len < 32)
	curlen = 32;		// big enough for sockaddr_in and sockaddr_in6
      else
	curlen = len;
      addr.generic = (sockaddr*)realloc(addr.generic, curlen);
    }

  if (sa != 0L)
    {
      memcpy(addr.generic, sa, len); // copy

      // now, normalise the data
      if (addr.generic->sa_family == AF_INET)
	reallen = SOCKADDR_IN_LEN; // no need to be larger
#ifdef AF_INET6
      else if (addr.generic->sa_family == AF_INET6)
	{
	  // set the extra field (sin6_scope_id)
	  
	  // the buffer is never smaller than 32 bytes, so this is always
	  // allowed
	  if (reallen < SOCKADDR_IN6_LEN)
	    addr.in6->sin6_scope_id = 0;
	  
	  reallen = SOCKADDR_IN6_LEN;
	}
#endif
      else if (addr.generic->sa_family == AF_UNIX)
	reallen = MIN_SOCKADDR_UN_LEN + strlen(addr.un->sun_path);
    }
  else if (clear)
    {
      memset(addr.generic, 0, len);
      addr.generic->sa_family = AF_UNSPEC;
    }
}

// default constructor
KSocketAddress::KSocketAddress()
  : d(new KSocketAddressData)
{
}

// constructor from binary data
KSocketAddress::KSocketAddress(const sockaddr *sa, Q_UINT16 len)
  : d(new KSocketAddressData)
{
  setAddress(sa, len);
}

KSocketAddress::KSocketAddress(const KSocketAddress& other)
  : d(new(KSocketAddressData))
{
  *this = other;
}

KSocketAddress::KSocketAddress(KSocketAddressData *d2)
  : d(d2)
{
}

KSocketAddress::~KSocketAddress()
{
  // prevent double-deletion, since we're already being deleted
  if (d)
    {
      d->ref.KInetSocketAddress::d = 0L;
      d->ref.KUnixSocketAddress::d = 0L;
      delete d;
    }
}

KSocketAddress& KSocketAddress::operator =(const KSocketAddress& other)
{
  if (other.d && !other.d->invalid())
    d->dup(other.d->addr.generic, other.d->reallen);
  else
    d->invalidate();
  return *this;
}

const sockaddr* KSocketAddress::address() const
{
  if (d->invalid())
    return 0L;
  return d->addr.generic;
}

sockaddr* KSocketAddress::address()
{
  if (d->invalid())
    return 0L;
  return d->addr.generic;
}

KSocketAddress& KSocketAddress::setAddress(const sockaddr* sa, Q_UINT16 len)
{
  if (sa != 0L && len >= MIN_SOCKADDR_LEN)
    d->dup(sa, len);
  else
    d->invalidate();

  return *this;
}

Q_UINT16 KSocketAddress::length() const
{
  if (d->invalid())
    return 0;
  return d->reallen;
}

KSocketAddress& KSocketAddress::setLength(Q_UINT16 len)
{
  d->dup((sockaddr*)0L, len, false);

  return *this;
}

int KSocketAddress::family() const
{
  if (d->invalid())
    return AF_UNSPEC;
  return d->addr.generic->sa_family;
}

KSocketAddress& KSocketAddress::setFamily(int family)
{
  if (d->invalid())
    d->dup((sockaddr*)0L, MIN_SOCKADDR_LEN);
  d->addr.generic->sa_family = family;

  return *this;
}

bool KSocketAddress::operator ==(const KSocketAddress& other) const
{
  // if this is invalid, it's only equal if the other one is invalid as well
  if (d->invalid())
    return other.d->invalid();

  // check the family to make sure we don't do unnecessary comparison
  if (d->addr.generic->sa_family != other.d->addr.generic->sa_family)
    return false;		// not the same family, not equal

  // same family then
  // check the ones we know already
  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
      Q_ASSERT(d->reallen == SOCKADDR_IN_LEN);
      Q_ASSERT(other.d->reallen == SOCKADDR_IN_LEN);
      return memcmp(d->addr.in, other.d->addr.in, SOCKADDR_IN_LEN) == 0;

#ifdef AF_INET6
    case AF_INET6:
      Q_ASSERT(d->reallen >= MIN_SOCKADDR_IN6_LEN);
      Q_ASSERT(other.d->reallen >= MIN_SOCKADDR_IN6_LEN);

# if !defined(HAVE_STRUCT_SOCKADDR_IN6) || defined(HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID)
      // check for the case where sin6_scope_id isn't present
      if (d->reallen != other.d->reallen)
	{
	  if (memcmp(d->addr.in6, other.d->addr.in6, MIN_SOCKADDR_IN6_LEN) != 0)
	    return false;	// not equal
	  if (d->reallen > other.d->reallen)
	    return d->addr.in6->sin6_scope_id == 0;
	  else
	    return other.d->addr.in6->sin6_scope_id == 0;
	}
# endif

	return memcmp(d->addr.in6, other.d->addr.in6, d->reallen) == 0;
#endif

    case AF_UNIX:
      Q_ASSERT(d->reallen >= MIN_SOCKADDR_UN_LEN);
      Q_ASSERT(other.d->reallen >= MIN_SOCKADDR_UN_LEN);

      // do a string comparison here
      return strcmp(d->addr.un->sun_path, other.d->addr.un->sun_path) == 0;

    default:
      // something else we don't know about
      // they are equal if and only if they are exactly equal
      if (d->reallen == other.d->reallen)
	return memcmp(d->addr.generic, other.d->addr.generic, d->reallen) == 0;
    }

  return false;		// not equal in any other case
}

QString KSocketAddress::nodeName() const
{
  if (d->invalid())
    return QString::null;

  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
#ifdef AF_INET6
    case AF_INET6:

      QString scopeid("%");
      if (d->addr.generic->sa_family == AF_INET6 && d->addr.in6->sin6_scope_id)
	scopeid += QString::number(d->addr.in6->sin6_scope_id);
      else
	scopeid.truncate(0);
      return d->ref.ipAddress().toString() + scopeid;
#else
      return d->ref.ipAddress().toString();
#endif
    }

  // any other case, including AF_UNIX
  return QString::null;
}

QString KSocketAddress::serviceName() const
{
  if (d->invalid())
    return QString::null;

  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
#ifdef AF_INET6
    case AF_INET6:
#endif
      return QString::number(d->ref.port());

    case AF_UNIX:
      return d->ref.pathname();
    }

  return QString::null;
}

QString KSocketAddress::toString() const
{
  if (d->invalid())
    return QString::null;

  QString fmt;

  if (d->addr.generic->sa_family == AF_INET)
    fmt = "%1:%2";
#ifdef AF_INET6
  else if (d->addr.generic->sa_family == AF_INET6)
    fmt = "[%1]:%2";
#endif
  else if (d->addr.generic->sa_family == AF_UNIX)
    return QString::fromLatin1("unix:%1").arg(serviceName());
  else
    return QObject::tr("Unknown family %1").arg(d->addr.generic->sa_family);

  return fmt.arg(nodeName()).arg(serviceName());
}

KInetSocketAddress& KSocketAddress::asInet()
{
  return d->ref;
}

KInetSocketAddress KSocketAddress::asInet() const
{
  return d->ref;
}

KUnixSocketAddress& KSocketAddress::asUnix()
{
  return d->ref;
}

KUnixSocketAddress KSocketAddress::asUnix() const
{
  return d->ref;
}

int KSocketAddress::ianaFamily(int af)
{
  switch (af)
    {
    case AF_INET:
      return 1;

#ifdef AF_INET6
    case AF_INET6:
      return 2;
#endif

    default:
      return 0;
    }
}

int KSocketAddress::fromIanaFamily(int iana)
{
  switch (iana)
    {
    case 1:
      return AF_INET;

#ifdef AF_INET6
    case 2:
      return AF_INET6;
#endif

    default:
      return AF_UNSPEC;
    }
}

// default constructor
KInetSocketAddress::KInetSocketAddress()
{
}

// binary data constructor
KInetSocketAddress::KInetSocketAddress(const sockaddr* sa, Q_UINT16 len)
  : KSocketAddress(sa, len)
{
  if (!d->invalid())
    update();
}

// create from IP and port
KInetSocketAddress::KInetSocketAddress(const KIpAddress& host, Q_UINT16 port)
{
  setHost(host);
  setPort(port);
}

// copy constructor
KInetSocketAddress::KInetSocketAddress(const KInetSocketAddress& other)
  : KSocketAddress(other)
{
}

// special copy constructor
KInetSocketAddress::KInetSocketAddress(const KSocketAddress& other)
  : KSocketAddress(other)
{
  if (!d->invalid())
    update();
}

// special constructor
KInetSocketAddress::KInetSocketAddress(KSocketAddressData *d)
  : KSocketAddress(d)
{
}

// destructor
KInetSocketAddress::~KInetSocketAddress()
{
  /* nothing to do */
}

// copy operator
KInetSocketAddress& KInetSocketAddress::operator =(const KInetSocketAddress& other)
{
  KSocketAddress::operator =(other);
  return *this;
}

// IP version
int KInetSocketAddress::ipVersion() const
{
  if (d->invalid())
    return 0;

  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
      return 4;

#ifdef AF_INET6
    case AF_INET6:
      return 6;
#endif
    }

  return 0;			// for all other cases
}

KIpAddress KInetSocketAddress::ipAddress() const
{
  if (d->invalid())
    return KIpAddress();	// return an empty address as well

  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
      return KIpAddress(&d->addr.in->sin_addr, 4);
#ifdef AF_INET6
    case AF_INET6:
      return KIpAddress(&d->addr.in6->sin6_addr, 6);
#endif
    }

  return KIpAddress();		// empty in all other cases
}

KInetSocketAddress& KInetSocketAddress::setHost(const KIpAddress& ip)
{
  switch (ip.version())
    {
    case 4:
      makeIPv4();
      memcpy(&d->addr.in->sin_addr, ip.addr(), sizeof(d->addr.in->sin_addr));
      break;

    case 6:
      makeIPv6();
      memcpy(&d->addr.in6->sin6_addr, ip.addr(), sizeof(d->addr.in6->sin6_addr));
      break;

    default:
      // empty
      d->invalidate();
    }

  return *this;
}

// returns the port
Q_UINT16 KInetSocketAddress::port() const
{
  if (d->invalid())
    return 0;

  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
      return ntohs(d->addr.in->sin_port);

#ifdef AF_INET6
    case AF_INET6:
      return ntohs(d->addr.in6->sin6_port);
#endif
    }

  return 0;
}

KInetSocketAddress& KInetSocketAddress::setPort(Q_UINT16 port)
{
  if (d->invalid())
    makeIPv4();

  switch (d->addr.generic->sa_family)
    {
    case AF_INET:
      d->addr.in->sin_port = htons(port);
      break;
      
#ifdef AF_INET6
    case AF_INET6:
      d->addr.in6->sin6_port = htons(port);
      break;
#endif
      
    default:
      d->invalidate();		// setting the port on something else
    }

  return *this;
}

KInetSocketAddress& KInetSocketAddress::makeIPv4()
{
  d->makeipv4();
  return *this;
}

KInetSocketAddress& KInetSocketAddress::makeIPv6()
{
  d->makeipv6();
  return *this;
}

Q_UINT32 KInetSocketAddress::flowinfo() const
{
#ifndef AF_INET6
  return 0;
#else

  if (!d->invalid() && d->addr.in6->sin6_family == AF_INET6)
    return d->addr.in6->sin6_flowinfo;
  return 0;
#endif
}

KInetSocketAddress& KInetSocketAddress::setFlowinfo(Q_UINT32 flowinfo)
{
  makeIPv6();			// must set here
  d->addr.in6->sin6_flowinfo = flowinfo;
  return *this;
}

int KInetSocketAddress::scopeId() const
{
#ifndef AF_INET6
  return 0;
#else

  if (!d->invalid() && d->addr.in6->sin6_family == AF_INET6)
    return d->addr.in6->sin6_scope_id;
  return 0;
#endif
}

KInetSocketAddress& KInetSocketAddress::setScopeId(int scopeid)
{
  makeIPv6();			// must set here
  d->addr.in6->sin6_scope_id = scopeid;
  return *this;
}

void KInetSocketAddress::update()
{
  if (d->addr.generic->sa_family == AF_INET)
    return;
#ifdef AF_INET6
  else if (d->addr.generic->sa_family == AF_INET6)
    return;
#endif
  else
    d->invalidate();
}

KUnixSocketAddress::KUnixSocketAddress()
{
}

KUnixSocketAddress::KUnixSocketAddress(const sockaddr* sa, Q_UINT16 len)
  : KSocketAddress(sa, len)
{
  if (!d->invalid() && d->addr.un->sun_family != AF_UNIX)
    d->invalidate();
}

KUnixSocketAddress::KUnixSocketAddress(const KUnixSocketAddress& other)
  : KSocketAddress(other)
{
}

KUnixSocketAddress::KUnixSocketAddress(const QString& pathname)
{
  setPathname(pathname);
}

KUnixSocketAddress::KUnixSocketAddress(KSocketAddressData* d)
  : KSocketAddress(d)
{
}

KUnixSocketAddress::~KUnixSocketAddress()
{
}

KUnixSocketAddress& KUnixSocketAddress::operator =(const KUnixSocketAddress& other)
{
  KSocketAddress::operator =(other);
  return *this;
}

QString KUnixSocketAddress::pathname() const
{
  if (!d->invalid() && d->addr.un->sun_family == AF_UNIX)
    return QFile::decodeName(d->addr.un->sun_path);
  return QString::null;
}

KUnixSocketAddress& KUnixSocketAddress::setPathname(const QString& path)
{
  d->dup(0L, MIN_SOCKADDR_UN_LEN + path.length());
  d->addr.un->sun_family = AF_UNIX;
  strcpy(d->addr.un->sun_path, QFile::encodeName(path));

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
  d->addr.un->sun_len = d->reallen;
#endif

  return *this;
}
