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

#ifndef KMULTICASTSOCKET_H
#define KMULTICASTSOCKET_H

#include "kdatagramsocket.h"
#include "kmulticastsocketdevice.h"

namespace KNetwork {

class KMulticastSocketPrivate;
/**
 * @class KMulticastSocket kmulticastsocket.h kmulticastsocket.h
 * @brief A multicast-capable datagram socket class
 *
 * This class derives from @ref KDatagramSocket adding methods to it to
 * allow better control over the multicast functionality. In special,
 * the join and leave group functions are added.
 *
 * Other more low-level options on multicast sockets can be accessed
 * directly with the @ref KMulticastSocketImpl class returned by 
 * @ref multicastSocketDevice.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KMulticastSocket: public KDatagramSocket
{
  // Q_add-it-here_OBJECT
public:
  /**
   * Constructor.
   */
  KMulticastSocket(QObject* parent = 0L, const char *name = 0L);

  /**
   * Destructor.
   */
  ~KMulticastSocket();

  /**
   * Returns the multicast socket device in use by this object.
   *
   * @note The returned object can be null.
   */
  KMulticastSocketImpl* multicastSocketDevice();

  /**
   * @overload
   */
  const KMulticastSocketImpl* multicastSocketDevice() const;

  /**
   * Joins a multicast group. The group to be joined is identified by the 
   * @p group parameter.
   *
   * @param group	the multicast group to join
   * @returns true on success
   */
  virtual bool joinGroup(const KSocketAddress& group);

  /**
   * @overload
   * Joins a multicast group. This function also specifies the network interface
   * to be used.
   */
  virtual bool joinGroup(const KSocketAddress& group, 
			 const KNetworkInterface& iface);

  /**
   * Leaves a multicast group. The group being left is given by its address in the
   * @p group parameter.
   *
   * @param group	the group to leave
   * @returns true on successful leaving the group
   */
  virtual bool leaveGroup(const KSocketAddress& group);

  /**
   * @overload
   * Leaves a multicast group.
   */
  virtual bool leaveGroup(const KSocketAddress& group,
			  const KNetworkInterface& iface);

private:
  KMulticastSocketPrivate *d;
};

}				// namespace KNetwork

#endif
