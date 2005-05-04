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

#ifndef KHTTPPROXYSOCKETDEVICE_H
#define KHTTPPROXYSOCKETDEVICE_H

#include "ksocketdevice.h"

namespace KNetwork {

class KHttpProxySocketDevicePrivate;

/**
 * @class KHttpProxySocketDevice khttpproxysocketdevice.h khttproxysocketdevice.h
 * @brief The low-level backend for HTTP proxying.
 *
 * This class derives from @ref KSocketDevice and implements the necessary
 * calls to make a connection through an HTTP proxy.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KHttpProxySocketDevice: public KSocketDevice
{
public:
  /**
   * Constructor.
   */
  KHttpProxySocketDevice(const KSocketBase* = 0L);

  /**
   * Constructor with proxy server's address.
   */
  KHttpProxySocketDevice(const KResolverEntry& proxy);

  /**
   * Destructor
   */
  virtual ~KHttpProxySocketDevice();

  /**
   * Sets our capabilities.
   */
  virtual int capabilities() const;

  /**
   * Retrieves the proxy server address.
   */
  const KResolverEntry& proxyServer() const;

  /**
   * Sets the proxy server address.
   */
  void setProxyServer(const KResolverEntry& proxy);

  /**
   * Closes the socket.
   */
  virtual void close();

  /**
   * Overrides connection.
   */
  virtual bool connect(const KResolverEntry& address);

  /**
   * Name-based connection.
   * We can tell the HTTP proxy server the full name.
   */
  virtual bool connect(const QString& name, const QString& service);

  /**
   * Return the peer address.
   */
  virtual KSocketAddress peerAddress() const;

  /**
   * Return the externally visible address. We can't tell what that address is,
   * so this function always returns an empty object.
   */
  virtual KSocketAddress externalAddress() const;

private:
  /**
   * Parses the server reply after sending the connect command.
   * Returns true on success and false on failure.
   */
  bool parseServerReply();
  KHttpProxySocketDevicePrivate *d;

public:
  /**
   * This is the default proxy server to be used.
   * Applications may want to set this value so that calling @ref setProxyServer
   * is unnecessary.
   */
  static KResolverEntry defaultProxy;
};

}				// namespace KNetwork

#endif
