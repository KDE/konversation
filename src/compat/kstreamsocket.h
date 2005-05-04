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

#ifndef KSTREAMSOCKET_H
#define KSTREAMSOCKET_H

#include <qstring.h>

#include "kclientsocketbase.h"

namespace KNetwork {

class KResolverEntry;
class KResolverResults;
class KServerSocket;

class KStreamSocketPrivate;
/** @class KStreamSocket kstreamsocket.h kstreamsocket.h
 *  @brief Simple stream socket
 *
 * This class provides functionality to creating unbuffered, stream
 * sockets. In the case of Internet (IP) sockets, this class creates and
 * uses TCP/IP sockets.
 *
 * Objects of this class start, by default, on non-blocking mode. Call
 * setBlocking if you wish to change that.
 *
 * Sample usage:
 * \code
 *   QByteArray httpGet(const QString& hostname)
 *   {
 *     KStreamSocket socket(hostname, "http");
 *     if (!socket.connect())
 *       return QByteArray();
 *     QByteArray data = socket.readAll();
 *     return data;
 *   }
 * \endcode
 *
 * Here's another sample, showing asynchronous operation:
 * \code
 *  DataRetriever::DataRetriever(const QString& hostname, const QString& port)
 *    : socket(hostname, port)
 *  {
 *    // connect signals to our slots
 *    QObject::connect(&socket, SIGNAL(connected(const KResolverEntry&)),
 *                     this, SLOT(slotSocketConnected()));
 *    QObject::connect(&socket, SIGNAL(gotError(int)),
 *                     this, SLOT(slotSocketError(int)));
 *    QObject::connect(&socket, SIGNAL(readyRead()),
 *                     this, SLOT(slotSocketReadyToRead()));
 *    QObject::connect(&socket, SIGNAL(readyWrite()),
 *                     this, SLOT(slotSocketReadyToWrite()));
 *
 *    // set non-blocking mode in order to work asynchronously
 *    socket.setBlocking(false);
 *
 *    // turn on signal emission
 *    socket.enableRead(true);
 *    socket.enableWrite(true);
 *
 *    // start connecting
 *    socket.connect();
 *  }
 * \endcode
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 * @version 0.9
 */
class KStreamSocket: public KClientSocketBase
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   *
   * @param node	destination host
   * @param service	destination service to connect to
   * @param parent	the parent QObject object
   * @param name	the name of this object
   */
  KStreamSocket(const QString& node = QString::null, const QString& service = QString::null,
		QObject* parent = 0L, const char *name = 0L);

  /**
   * Destructor. This closes the socket.
   */
  virtual ~KStreamSocket();

  /**
   * Retrieves the timeout value (in milliseconds).
   */
  int timeout() const;

  /**
   * Retrieves the remaining timeout time (in milliseconds). This value
   * equals @ref timeout() if there's no connection in progress.
   */
  int remainingTimeout() const;

  /**
   * Sets the timeout value. Setting this value while a connection attempt
   * is in progress will reset the timer.
   *
   * Please note that the timeout value is valid for the connection attempt
   * only. No other operations are timed against this value -- including the
   * name lookup associated.
   *
   * @param msecs		the timeout value in milliseconds
   */
  void setTimeout(int msecs);

  /**
   * Binds this socket to the given nodename and service,
   * or use the default ones if none are given. In order to bind to a service
   * and allow the operating system to choose the interface, set @p node to
   * QString::null.
   * 
   * Reimplemented from KClientSocketBase.
   *
   * Upon successful binding, the @ref bound signal will be
   * emitted. If an error is found, the @ref gotError
   * signal will be emitted.
   *
   * @note Due to the internals of the name lookup and binding
   *       mechanism, some (if not most) implementations of this function
   *       do not actually bind the socket until the connection
   *       is requested (see @ref connect). They only set the values
   *       for future reference.
   *
   * This function returns true on success.
   *
   * @param node	the nodename
   * @param service	the service
   */
  virtual bool bind(const QString& node = QString::null,
		    const QString& service = QString::null);

  /**
   * Reimplemented from KClientSocketBase. Connect this socket to this
   * specific address.
   *
   * Unlike @ref bind(const QString&, const QString&) above, this function
   * really does bind the socket. No lookup is performed. The @ref bound
   * signal will be emitted.
   */
  virtual bool bind(const KResolverEntry& entry)
  { return KClientSocketBase::bind(entry); }

  /**
   * Reimplemented from KClientSocketBase.
   *
   * Attempts to connect to the these hostname and service,
   * or use the default ones if none are given. If a connection attempt
   * is already in progress, check on its state and set the error status
   * (NoError, meaning the connection is completed, or InProgress).
   *
   * If the blocking mode for this object is on, this function will only
   * return when all the resolved peer addresses have been tried or when
   * a connection is established.
   *
   * Upon successfully connecting, the @ref connected signal 
   * will be emitted. If an error is found, the @ref gotError
   * signal will be emitted.
   *
   * This function also implements timeout handling.
   *
   * @param node	the remote node to connect to
   * @param service	the service on the remote node to connect to
   */
  virtual bool connect(const QString& node = QString::null,
		       const QString& service = QString::null);

  /**
   * Unshadowing from KClientSocketBase.
   */
  virtual bool connect(const KResolverEntry& entry);

signals:
  /**
   * This signal is emitted when a connection timeout occurs.
   */
  void timedOut();

private slots:
  void hostFoundSlot();
  void connectionEvent();
  void timeoutSlot();

private:
  /**
   * @internal
   * If the user requested local bind before connection, bind the socket to one
   * suitable address and return true. Also sets d->local to the address used.
   *
   * Return false in case of error.
   */
  bool bindLocallyFor(const KResolverEntry& peer);

  /**
   * @internal
   * Finishes the connection process by setting internal values and
   * emitting the proper signals.
   *
   * Note: assumes d->local iterator points to the address that we bound
   * to.
   */
  void connectionSucceeded(const KResolverEntry& peer);

  KStreamSocket(const KStreamSocket&);
  KStreamSocket& operator=(const KStreamSocket&);

  KStreamSocketPrivate *d;

  friend class KServerSocket;
};

} 				// namespace KNetwork

#endif
