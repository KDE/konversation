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

#ifndef KSOCKETADDRESS_H
#define KSOCKETADDRESS_H

#include <qstring.h>
#include <qcstring.h>


struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct sockaddr_un;

namespace KNetwork {

class KIpAddress;
class KSocketAddress;
class KInetSocketAddress;
class KUnixSocketAddress;

/** @class KIpAddress ksocketaddress.h ksocketaddress.h
 *  @brief An IP address.
 *
 * This class represents one IP address, version 4 or 6. This is only
 * the address, not including port information or other data.
 *
 * It is not a good programming practice to create address from objects
 * like this. Instead, prefer a more thorough function like
 * @ref KResolver::resolve, which also handle extra information like scope
 * ids.
 *
 * This is a light-weight class. Most of the member functions are inlined and
 * there are no virtual functions. This object's size should be less than 20
 * bytes. Also note that there is no sharing of data.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KIpAddress
{
public:
  /**
   * Default constructor. Creates an empty address.
   * It defaults to IP version 4.
   */
  inline KIpAddress() : m_version(0)
  { }

  /**
   * Copy constructor. Copies the data from the other
   * object.
   *
   * Data is not shared.
   *
   * @param other		the other
   */
  inline KIpAddress(const KIpAddress& other)
  { *this = other; }

  /**
   * Creates an object from the given string representation.
   *
   * The IP version is guessed from the address format.
   *
   * @param addr		the address
   */
  inline KIpAddress(const QString& addr)
  { setAddress(addr); }

  /**
   * Creates an object from the given string representation.
   *
   * The IP version is guessed from the address format.
   *
   * @param addr		the address
   */
  inline KIpAddress(const char* addr)
  { setAddress(addr); }

  /**
   * Creates an object from the given raw data and IP version.
   *
   * @param addr		the raw data
   * @param version		the IP version (4 or 6)
   */
  inline KIpAddress(const void* addr, int version = 4)
  { setAddress(addr, version); }

  /**
   * This is a convenience constructor. Constructs an object
   * from the given IPv4 address in the form of an integer.
   *
   * Note: do not write code to depend on IPv4 addresses being
   * integer types. Instead, treat them as a special type, like
   * a KIpAddress or the system's in_addr.
   *
   * @param ip4addr		the IPv4 address
   */
  inline KIpAddress(Q_UINT32 ip4addr)
  { setAddress(&ip4addr, 4); }

  /**
   * Destructor. This frees resources associated with this object.
   *
   * Note: destructor is non-virtual. The compiler will happily optimise it
   * out of the way.
   */
  inline ~KIpAddress()
  { }

  /**
   * Copy operator.
   *
   * Copies the data from the other object into this one.
   *
   * @param other		the object to copy
   */
  KIpAddress& operator =(const KIpAddress& other);

  /**
   * Returns true if the two addresses match.
   * This function performs a v4-mapped check.
   * @see compare
   */
  inline bool operator ==(const KIpAddress& other) const
  { return compare(other, true); }

  /**
   * Compares this address against the other, supplied one and return
   * true if they match. The @p checkMapped parameter controls whether
   * a check for an IPv6 v4-mapped address will be performed.
   *
   * An IPv6 v4-mapped address is an IPv6 address that is, for all purposes,
   * equivalent to an IPv4 one. The default behaviour of this function
   * is to take that into account. If you want a strict matching,
   * pass @b false to the @p checkMapped parameter.
   *
   * @param other	the other IP address
   * @param checkMapped	whether v4-mapped addresses will be taken into account
   */
  bool compare(const KIpAddress& other, bool checkMapped = true) const;

  /**
   * Retrieves the IP version in this object.
   *
   * @returns the version: 4 or 6
   */
  inline int version() const
  { return m_version; }

  /**
   * Returns true if this is an IPv4 address.
   */
  inline bool isIPv4Addr() const
  { return version() == 4; }

  /**
   * Returns true if this is an IPv6 address.
   */
  inline bool isIPv6Addr() const
  { return version() == 6; }

  /**
   * Sets the address to the given string representation.
   *
   * @return true if the address was successfully parsed; otherwise returns
   * false and leaves the object unchanged.
   */
  bool setAddress(const QString& address);

  /**
   * Sets the address to the given string representation.
   *
   * @return true if the address was successfully parsed; otherwise returns
   * false and leaves the object unchanged.
   */
  bool setAddress(const char* address);

  /**
   * Sets the address to the given raw binary representation.
   *
   * @param raw			a pointer to the raw binary data
   * @param version		the IP version
   * @return true if the address was successfully parsed; otherwise returns
   * false and leaves the object unchanged.
   */
  bool setAddress(const void* raw, int version = 4);

  /**
   * Returns the address as a string.
   */
  QString toString() const;

  /**
   * Returns a pointer to binary raw data representing the address.
   */
  inline const void *addr() const
  { return m_data; }

  /**
   * This is a convenience function. Returns the IPv4 address in a
   * 32-bit integer. The result is only valid if @ref isIPv4Addr returns
   * true. Alternatively, if the contained IPv6 address is a v4-mapped one
   * and the @p convertMapped parameter is true, the result will also be
   * valid.
   *
   * Note: you should not treat IP addresses as integers. Instead,
   * use types defined for that purpose, such as KIpAddress or the
   * system's in_addr type.
   *
   * @bug Check if byte ordering is done right
   */
  inline Q_UINT32 IPv4Addr(bool convertMapped = true) const
  {
    return (convertMapped && isV4Mapped()) ? m_data[3] : m_data[0];
  }

  /*-- tests --*/

  /**
   * Returns true if this is the IPv4 or IPv6 unspecified address.
   */
  inline bool isUnspecified() const
  { return version() == 0 ? true : (*this == anyhostV4 || *this == anyhostV6); }

  /**
   *  Returns true if this is either the IPv4 or the IPv6 localhost address.
   */
  inline bool isLocalhost() const
  { return version() == 0 ? false : (*this == localhostV4 || *this == localhostV6); }

  /**
   * This is an alias for @ref isLocalhost.
   */
  inline bool isLoopback() const
  { return isLocalhost(); }

  /**
   * Returns true if this is an IPv4 class A address, i.e., 
   * from 0.0.0.0 to 127.255.255.255.
   *
   * This function does not test for v4-mapped addresses.
   */
  inline bool isClassA() const
  { return version() != 4 ? false : (IPv4Addr() & 0x80000000) == 0; }

  /**
   * Returns true if this is an IPv4 class B address, i.e., one from
   * 128.0.0.0 to 191.255.255.255.
   *
   * This function does not test for v4-mapped addresses.
   */
  inline bool isClassB() const
  { return version() != 4 ? false : (IPv4Addr() & 0xc0000000) == 0x80000000; }

  /**
   * Returns true if this is an IPv4 class C address, i.e., one from
   * 192.0.0.0 to 223.255.255.255.
   *
   * This function does not test for v4-mapped addresses.
   */
  inline bool isClassC() const
  { return version() != 4 ? false : (IPv4Addr() & 0xe0000000) == 0xc0000000; }

  /**
   * Returns true if this is an IPv4 class D (a.k.a. multicast) address.
   *
   * Note: this function is not the same as @ref isMulticast. isMulticast also
   * tests for IPv6 multicast addresses.
   */
  inline bool isClassD() const
  { return version() != 4 ? false : (IPv4Addr() & 0xf0000000) == 0xe0000000; }

  /**
   * Returns true if this is a multicast address, be it IPv4 or IPv6.
   */
  inline bool isMulticast() const
  {
    if (version() == 4) return isClassD();
    if (version() == 6) return ((Q_UINT8*)addr())[0] == 0xff;
    return false;
  }

  /**
   * Returns true if this is an IPv6 link-local address.
   */
  inline bool isLinkLocal() const
  { 
    if (version() != 6) return false;
    Q_UINT8* addr = (Q_UINT8*)this->addr();
    return (addr[0] & 0xff) == 0xfe &&
      (addr[1] & 0xc0) == 0x80;
  }

  /**
   * Returns true if this is an IPv6 site-local address.
   */
  inline bool isSiteLocal() const
  {
    if (version() != 6) return false;
    Q_UINT8* addr = (Q_UINT8*)this->addr();
    return (addr[0] & 0xff) == 0xfe &&
      (addr[1] & 0xc0) == 0xc0;
  }

  /**
   * Returns true if this is a global IPv6 address.
   */
  inline bool isGlobal() const
  { return version() != 6 ? false : !(isMulticast() || isLinkLocal() || isSiteLocal()); }

  /**
   * Returns true if this is a v4-mapped IPv6 address.
   */
  inline bool isV4Mapped() const
  {
    if (version() != 6) return false;
    Q_UINT32* addr = (Q_UINT32*)this->addr();
    return addr[0] == 0 && addr[1] == 0 &&
      ((Q_UINT16*)&addr[2])[0] == 0 &&
      ((Q_UINT16*)&addr[2])[1] == 0xffff;
  }

  /**
   * Returns true if this is a v4-compat IPv6 address.
   */
  inline bool isV4Compat() const
  {
    if (version() != 6 || isLocalhost()) return false;
    Q_UINT32* addr = (Q_UINT32*)this->addr();
    return addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] != 0;
  }

  /**
   * Returns true if this is an IPv6 node-local multicast address.
   */
  inline bool isMulticastNodeLocal() const
  { return version() == 6 && isMulticast() && (((Q_UINT32*)addr())[0] & 0xf) == 0x1; }

  /**
   * Returns true if this is an IPv6 link-local multicast address.
   */
  inline bool isMulticastLinkLocal() const
  { return version() == 6 && isMulticast() && (((Q_UINT32*)addr())[0] & 0xf) == 0x2; }
      
  /**
   * Returns true if this is an IPv6 site-local multicast address.
   */
  inline bool isMulticastSiteLocal() const
  { return version() == 6 && isMulticast() && (((Q_UINT32*)addr())[0] & 0xf) == 0x5; }

  /**
   * Returns true if this is an IPv6 organisational-local multicast address.
   */
  inline bool isMulticastOrgLocal() const
  { return version() == 6 && isMulticast() && (((Q_UINT32*)addr())[0] & 0xf) == 0x8; }

  /**
   * Returns true if this is an IPv6 global multicast address.
   */
  inline bool isMulticastGlobal() const
  { return version() == 6 && isMulticast() && (((Q_UINT32*)addr())[0] & 0xf) == 0xe; }

protected:
  Q_UINT32 m_data[4];	       // 16 bytes, needed for an IPv6 address

  char m_version;

public:
  /// localhost in IPv4 (127.0.0.1)
  static const KIpAddress localhostV4;
  /// the any host or undefined address in IPv4 (0.0.0.0)
  static const KIpAddress anyhostV4;

  /// localhost in IPv6 (::1)
  static const KIpAddress localhostV6;
  /// the any host or undefined address in IPv6 (::)
  static const KIpAddress anyhostV6;
};


class KSocketAddressData;
/** @class KSocketAddress ksocketaddress.h ksocketaddress.h
 *  @brief A generic socket address.
 *
 * This class holds one generic socket address.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KSocketAddress
{
public:
  /**
   * Default constructor.
   *
   * Creates an empty object
   */
  KSocketAddress();

  /**
   * Creates this object with the given data.
   * The raw socket address is copied into this object.
   *
   * @param sa		the socket address structure
   * @param len		the socket address length
   */
  KSocketAddress(const sockaddr* sa, Q_UINT16 len);

  /**
   * Copy constructor. This creates a copy of the other
   * object.
   *
   * Data is not shared.
   *
   * @param other	the object to copy from
   */
  KSocketAddress(const KSocketAddress& other);

  /**
   * Destructor. Frees any associated resources.
   */
  virtual ~KSocketAddress();

  /**
   * Performs a shallow copy of the other object into this one.
   * Data will be copied.
   *
   * @param other	the object to copy from
   */
  KSocketAddress& operator =(const KSocketAddress& other);

  /**
   * Returns the socket address structure, to be passed down to
   * low level functions.
   *
   * Note that this function returns NULL for invalid or empty sockets,
   * so you may use to to test for validity.
   */
  const sockaddr* address() const;

  /**
   * Returns the socket address structure, to be passed down to
   * low level functions.
   *
   * Note that this function returns NULL for invalid or empty sockets,
   * so you may use to to test for validity.
   *
   * The returned value, if not NULL, is an internal buffer which is guaranteed
   * to be at least @ref length() bytes long.
   */
  sockaddr* address();

  /**
   * Sets the address to the given address.
   * The raw socket address is copied into this object.
   *
   * @param sa		the socket address structure
   * @param len		the socket address length
   */
  KSocketAddress& setAddress(const sockaddr *sa, Q_UINT16 len);

  /**
   * Returns the socket address structure, to be passed down to
   * low level functions.
   */
  inline operator const sockaddr*() const
  { return address(); }

  /**
   * Returns the length of this socket address structure.
   */
  Q_UINT16 length() const;

  /**
   * Sets the length of this socket structure.
   *
   * Use this function with care. It allows you to resize the internal
   * buffer to fit needs. This function should not be used except for handling
   * unknown socket address structures.
   *
   * Also note that this function may invalidate the socket if a known
   * family is set (Internet or Unix socket) and the new length would be
   * too small to hold the system's sockaddr_* structure. If unsure, reset
   * the family:
   *
   * \code
   *   KSocketAddress qsa;
   *   [...]
   *   qsa.setFamily(AF_UNSPEC).setLength(newlen);
   * \endcode
   *
   * @param len		the new length
   */
  KSocketAddress& setLength(Q_UINT16 len);

  /**
   * Returns the family of this address.
   * @return the family of this address, AF_UNSPEC if it's undefined
   */
  int family() const;

  /**
   * Sets the family of this object.
   *
   * Note: setting the family will probably invalidate any address data
   * contained in this object. Use this function with care.
   *
   * @param family	the new family to set
   */
  virtual KSocketAddress& setFamily(int family);

  /**
   * Returns the IANA family number of this address.
   * @return the IANA family number of this address (1 for AF_INET.
   *         2 for AF_INET6, otherwise 0)
   */
  inline int ianaFamily() const
  { return ianaFamily(family()); }
  
  /**
   * Returns true if this equals the other socket.
   *
   * Socket addresses are considered matching if and only if all data is the same.
   *
   * @param other	the other socket
   * @return true if both sockets are equal
   */
  bool operator ==(const KSocketAddress& other) const;

  /**
   * Returns the node name of this socket.
   *
   * In the case of Internet sockets, this is string representation of the IP address.
   * The default implementation returns QString::null.
   *
   * @return the node name, can be QString::null
   * @bug use KResolver to resolve unknown families
   */
  virtual QString nodeName() const;

  /**
   * Returns the service name for this socket.
   *
   * In the case of Internet sockets, this is the port number.
   * The default implementation returns QString::null.
   *
   * @return the service name, can be QString::null
   * @bug use KResolver to resolve unknown families
   */
  virtual QString serviceName() const;

  /**
   * Returns this socket address as a string suitable for
   * printing. Family, node and service are part of this address.
   *
   * @bug use KResolver to resolve unknown families
   */
  virtual QString toString() const;

  /**
   * Returns an object reference that can be used to manipulate this socket
   * as an Internet socket address. Both objects share the same data.
   */
  KInetSocketAddress& asInet();

  /**
   * Returns an object is equal to this object's data, but they don't share it.
   */
  KInetSocketAddress asInet() const;

  /**
   * Returns an object reference that can be used to manipulate this socket
   * as a Unix socket address. Both objects share the same data.
   */
  KUnixSocketAddress& asUnix();

  /**
   * Returns an object is equal to this object's data, but they don't share it.
   */
  KUnixSocketAddress asUnix() const;

protected:
  /// @internal
  /// private data
  KSocketAddressData *d;

  /// @internal
  /// extra constructor
  KSocketAddress(KSocketAddressData* d);

public:				// static
  /**
   * Returns the IANA family number of the given address family.
   * Returns 0 if there is no corresponding IANA family number.
   * @param af		the address family, in AF_* constants
   * @return the IANA family number of this address (1 for AF_INET.
   *         2 for AF_INET6, otherwise 0)
   */
  static int ianaFamily(int af);

  /**
   * Returns the address family of the given IANA family number.
   * @return the address family, AF_UNSPEC for unknown IANA family numbers
   */
  static int fromIanaFamily(int iana);
};


/** @class KInetSocketAddress ksocketaddress.h ksocketaddress.h
 *  @brief an Internet socket address
 *
 * An Inet (IPv4 or IPv6) socket address
 *
 * This is an IPv4 or IPv6 address of the Internet.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KInetSocketAddress: public KSocketAddress
{
  friend class KSocketAddress;
public:
  /**
   * Public constructor. Creates an empty object.
   */
  KInetSocketAddress();

  /**
   * Creates an object from raw data.
   *
   * Note: if the socket address @p sa does not contain a valid Internet
   * socket (IPv4 or IPv6), this object will be empty.
   *
   * @param sa		the sockaddr structure
   * @param len		the structure's length
   */
  KInetSocketAddress(const sockaddr* sa, Q_UINT16 len);

  /**
   * Creates an object from an IP address and port.
   *
   * @param host	the IP address
   * @param port	the port number
   */
  KInetSocketAddress(const KIpAddress& host, Q_UINT16 port);

  /**
   * Copy constructor.
   *
   * Data is not shared.
   *
   * @param other	the other object
   */
  KInetSocketAddress(const KInetSocketAddress& other);

  /**
   * Copy constructor.
   *
   * If the other, generic socket address contains an Internet address,
   * it will be copied. Otherwise, this object will be empty.
   *
   * @param other	the other object
   */
  KInetSocketAddress(const KSocketAddress& other);

  /**
   * Destroys this object.
   */
  virtual ~KInetSocketAddress();

  /**
   * Copy operator.
   *
   * Copies the other object into this one.
   *
   * @param other	the other object
   */
  KInetSocketAddress& operator =(const KInetSocketAddress& other);

  /**
   * Cast operator to sockaddr_in.
   */
  inline operator const sockaddr_in*() const
  { return (const sockaddr_in*)address(); }

  /**
   * Cast operator to sockaddr_in6.
   */
  inline operator const sockaddr_in6*() const
  { return (const sockaddr_in6*)address(); }

  /**
   * Returns the IP version of the address this object holds.
   *
   * @return 4 or 6, if IPv4 or IPv6, respectively; 0 if this object is empty
   */
  int ipVersion() const;

  /**
   * Returns the IP address component.
   */
  KIpAddress ipAddress() const;

  /**
   * Sets the IP address to the given raw address.
   *
   * This call will preserve port numbers accross IP versions, but will lose
   * IPv6 specific data if the address is set to IPv4.
   *
   * @param addr	the address to set to
   * @return a reference to itself
   */
  KInetSocketAddress& setHost(const KIpAddress& addr);

  /**
   * Retrieves the port number stored in this object.
   *
   * @return a port number in the range 0 to 65535, inclusive. An empty or 
   * invalid object will have a port number of 0.
   */
  Q_UINT16 port() const;

  /**
   * Sets the port number. If this object is empty, this function will default to
   * creating an IPv4 address.
   *
   * @param port	the port number to set
   * @return a reference to itself
   */
  KInetSocketAddress& setPort(Q_UINT16 port);

  /**
   * Converts this object to an IPv4 socket address. It has no effect if the object
   * is already an IPv4 socket address.
   *
   * If this object is an IPv6 address, the port number is preserved. All other information
   * is lost.
   *
   * @return a reference to itself
   */
  KInetSocketAddress& makeIPv4();

  /**
   * Converts this object to an IPv6 socket address. It has no effect if the object
   * is already an IPv6 socket address.
   *
   * If this object is an IPv4 address, the port number is preserved.
   *
   * @return a reference to itself
   */
  KInetSocketAddress& makeIPv6();

  /**
   * Returns the flowinfo information from the IPv6 socket address.
   *
   * @return the flowinfo information or 0 if this object is empty or IPv4
   */
  Q_UINT32 flowinfo() const;

  /**
   * Sets the flowinfo information for an IPv6 socket address. If this is not
   * an IPv6 socket address, this function converts it to one. @see makeIPv6.
   *
   * @param flowinfo		the flowinfo to set
   * @return a reference to itself
   */
  KInetSocketAddress& setFlowinfo(Q_UINT32 flowinfo);

  /**
   * Returns the scope id this IPv6 socket is bound to.
   *
   * @return the scope id, or 0 if this is not an IPv6 object
   */
  int scopeId() const;

  /**
   * Sets the scope id for this IPv6 object. If this is not an IPv6 socket
   * address, this function converts it to one. @see makeIPv6
   *
   * @param scopeid		the scopeid to set
   * @return a reference to itself
   */
  KInetSocketAddress& setScopeId(int scopeid);

protected:
  /// @internal
  /// extra constructor
  KInetSocketAddress(KSocketAddressData* d);

private:
  void update();
};

/*
 * External definition
 */

/** @class KUnixSocketAddress ksocketaddress.h ksocketaddress.h
 *  @brief A Unix (local) socket address.
 *
 * This is a Unix socket address.
 *
 * Note that this class uses QStrings to represent filenames, which means
 * the proper encoding is used to translate into valid filesystem file names.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KUnixSocketAddress: public KSocketAddress
{
  friend class KSocketAddress;
public:
  /**
   * Default constructor. Creates an empty object.
   */
  KUnixSocketAddress();

  /**
   * Creates this object with the given raw data. If
   * the sockaddr structure does not contain a Local namespace
   * (Unix) socket, this object will be created empty.
   *
   * @param sa		the socket address structure
   * @param len		the structure's length
   */
  KUnixSocketAddress(const sockaddr* sa, Q_UINT16 len);

  /**
   * Copy constructor. Creates a copy of the other object,
   * sharing the data explicitly.
   *
   * @param other	the other object
   */
  KUnixSocketAddress(const KUnixSocketAddress& other);

  /**
   * Constructs an object from the given pathname.
   */
  KUnixSocketAddress(const QString& pathname);

  /**
   * Destructor.
   */
  virtual ~KUnixSocketAddress();

  /**
   * Copy operator. Copies the contents of the other object into
   * this one. Data is explicitly shared.
   *
   * @param other		the other
   */
  KUnixSocketAddress& operator =(const KUnixSocketAddress& other);

  /**
   * Cast operator to sockaddr_un.
   */
  inline operator const sockaddr_un*() const
  { return (const sockaddr_un*)address(); }

  /**
   * Returns the pathname associated with this object. Will return
   * QString::null if this object is empty.
   */
  QString pathname() const;

  /**
   * Sets the pathname for the object.
   *
   * @return a reference to itself
   */
  KUnixSocketAddress& setPathname(const QString& path);

protected:
  /// @internal
  /// extra constructor
  KUnixSocketAddress(KSocketAddressData* d);
};

}				// namespace KNetwork

#endif
