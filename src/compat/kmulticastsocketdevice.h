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

#ifndef KMULTICASTSOCKETDEVICE_H
#define KMULTICASTSOCKETDEVICE_H

#include "ksocketdevice.h"
#include "knetworkinterface.h"
#include "ksocketaddress.h"

namespace KNetwork {

class KMulticastSocketImplPrivate;

/**
 * @class KMulticastSocketImpl kmulticastsocketdevice.h kmulticastsocketdevice.h
 * @brief The low-level backend for multicasting sockets.
 *
 * This class is an interface providing methods for handling multicast 
 * operations.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KMulticastSocketImpl: public KSocketDevice
{
public:
  /**
   * Constructor.
   */
  KMulticastSocketImpl(const KSocketBase* = 0L);

  /**
   * Destructor
   */
  virtual ~KMulticastSocketImpl();

  /**
   * Sets our capabilities.
   */
  virtual int capabilities() const;

  /**
   * Overrides the socket creation.
   */
  virtual bool create(int family, int type, int protocol);

  /**
   * Overrides connection. Multicast sockets may not connect.
   */
  virtual bool connect(const KResolverEntry& address);

  /**
   * Retrieves the time-to-live/hop count value on multicast packets being sent.
   */
  virtual int timeToLive() const;

  /**
   * Sets the time-to-live/hop count for outgoing multicast packets.
   *
   * @param ttl		the hop count, from 0 to 255
   * @returns true if setting the value was successful.
   */
  virtual bool setTimeToLive(int ttl);

  /**
   * Retrieves the flag indicating if sent packets will be echoed back
   * to sender.
   */
  virtual bool multicastLoop() const;

  /**
   * Sets the flag indicating the loopback of packets to the sender.
   *
   * @param enable	if true, will echo back
   * @returns true if setting the value was successful.
   */
  virtual bool setMulticastLoop(bool enable);

  /**
   * Retrieves the network interface this socket is associated to.
   */
  virtual KNetworkInterface networkInterface();

  /**
   * Sets the network interface on which this socket should work.
   *
   * @param iface	the interface to associate with
   * @return true if setting the value was successful.
   */
  virtual bool setNetworkInterface(const KNetworkInterface& iface);

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
  KMulticastSocketImplPrivate *d;
};

}				// namespace KNetwork

#endif
