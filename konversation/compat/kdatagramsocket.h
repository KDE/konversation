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

#ifndef KDATAGRAMSOCKET_H
#define KDATAGRAMSOCKET_H

#include <qcstring.h>

#include "ksocketaddress.h"
#include "kclientsocketbase.h"

namespace KNetwork {

class KResolverEntry;

/**
 * @class KDatagramPacket kdatagramsocket.h kdatagramsocket.h
 * @brief one datagram
 *
 * This object represents one datagram of data sent or received through
 * a datagram socket (as @ref KDatagramSocket or derived classes). A datagram
 * consists of data as well as a network address associated (whither to send
 * the data or whence it came).
 *
 * This is a lightweight class. Data is stored in a @ref QByteArray, which means
 * that it is explicitly shared.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KDECORE_EXPORT KDatagramPacket
{
  QByteArray m_data;
  KSocketAddress m_address;

public:
  /**
   * Default constructor.
   */
  KDatagramPacket()
  { }

  /**
   * Constructs the datagram with the specified content.
   */
  KDatagramPacket(const QByteArray& content)
    : m_data(content)
  { }

  /**
   * Constructs the datagram with the specified content.
   *
   * @see setData for information on data sharing.
   */
  KDatagramPacket(const char* content, uint length)
  { setData(content, length); }

  /**
   * Constructs the datagram with the specified content and address.
   */
  KDatagramPacket(const QByteArray& content, const KSocketAddress& addr)
    : m_data(content), m_address(addr)
  { }

  /**
   * Constructs the datagram with the specified content and address.
   */
  KDatagramPacket(const char *content, uint length, const KSocketAddress& addr)
    : m_address(addr)
  { setData(content, length); }

  /**
   * Copy constructor. Note that data is explicitly shared.
   */
  KDatagramPacket(const KDatagramPacket& other)
  { *this = other; }

  /**
   * Destructor. Non-virtual.
   */
  ~KDatagramPacket()
  { }

  /**
   * Returns the data.
   */
  const QByteArray& data() const
  { return m_data; }

  /**
   * Returns the data length.
   */
  uint length() const
  { return m_data.size(); }

  /**
   * Returns the data length.
   */
  uint size() const
  { return m_data.size(); }

  /**
   * Returns true if this object is empty.
   */
  bool isEmpty() const
  { return m_data.isEmpty(); }

  /**
   * Returns true if this object is null.
   */
  bool isNull() const
  { return m_data.isNull(); }

  /**
   * Returns the socket address
   */
  const KSocketAddress& address() const
  { return m_address; }

  /**
   * Sets the address stored to the given value.
   */
  void setAddress(const KSocketAddress& addr)
  { m_address = addr; }

  /**
   * Detaches our data from a shared pool.
   * @see QByteArray::detach
   */
  void detach()
  { m_data.detach(); }

  /**
   * Sets the data to the given value. Data is explicitly shared.
   */
  void setData(const QByteArray& data)
  { m_data = data; }

  /**
   * Sets the data to the given buffer and size.
   */
  void setData(const char* data, uint length)
  { m_data.duplicate(data, length); }
};

class KDatagramSocketPrivate;
/**
 * @class KDatagramSocket kdatagramsocket.h kdatagramsocket.h
 * @brief A socket that operates on datagrams.
 *
 * Unlike @ref KStreamSocket, which operates on a connection-based stream
 * socket (generally TCP), this class and its descendants operates on datagrams, 
 * which are normally connectionless.
 *
 * This class in specific provides easy access to the system's connectionless
 * SOCK_DGRAM sockets. 
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KDECORE_EXPORT KDatagramSocket: public KClientSocketBase
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   */
  KDatagramSocket(QObject* parent = 0L, const char *name = 0L);

  /**
   * Destructor. This closes the socket.
   */
  virtual ~KDatagramSocket();

  /**
   * Performs host lookups.
   */
  //  virtual bool lookup();

  /**
   * Binds this socket to the given address. If the socket is blocking,
   * the socket will be bound when this function returns.
   *
   * Note that binding a socket is not necessary to be able to send datagrams.
   * Some protocol families will use anonymous source addresses, while others
   * will allocate an address automatically.
   */
  virtual bool bind(const QString& node = QString::null,
		    const QString& service = QString::null);

  /**
   * @overload
   * Binds this socket to the given address.
   */
  virtual bool bind(const KResolverEntry& entry)
  { return KClientSocketBase::bind(entry); }

  /**
   * "Connects" this socket to the given address. Note that connecting
   * a datagram socket normally does not establish a permanent connection
   * with the peer nor normally returns an error in case of failure.
   *
   * Connecting means only to designate the given address as the default
   * destination address for datagrams sent without destination addresses
   * (@ref writeBlock(const char*, Q_LONG).
   *
   * @note Calling connect will not cause the socket to be bound. You have
   *       to call @ref bind explicitly.
   */
  virtual bool connect(const QString& node = QString::null,
		       const QString& service = QString::null);

  /**
   * @overload
   * "Connects" this socket to the given address.
   */
  virtual bool connect(const KResolverEntry& entry)
  { return KClientSocketBase::connect(entry); }

  /**
   * Receives one datagram from the stream. The reading process is guaranteed
   * to be atomical and not lose data from the packet.
   *
   * If nothing could be read, a null object will be returned.
   */
  virtual KDatagramPacket receive();

  /**
   * Sends one datagram into the stream. The destination address must be
   * set if this socket has not been connected (see @ref connect).
   *   
   * The data in this packet will be sent only in one single datagram. If the
   * system cannot send it like that, this function will fail. So, please take
   * into consideration the datagram size limits.
   *
   * @returns the number of bytes written or -1 in case of error.
   */
  virtual Q_LONG send(const KDatagramPacket& packet);

private slots:
  void lookupFinishedLocal();
  void lookupFinishedPeer();

private:
  bool doBind();
  void setupSignals();

  KDatagramSocketPrivate *d;
};

}				// namespace KNetwork

#endif
