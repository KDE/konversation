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

#ifndef KCLIENTSOCKETBASE_H
#define KCLIENTSOCKETBASE_H

#include <qobject.h>
#include <qstring.h>

#include "ksocketbase.h"
#include "kresolver.h"
#include <kdelibs_export.h>

namespace KNetwork {

class KClientSocketBasePrivate;
/** @class KClientSocketBase kclientsocketbase.h kclientsocketbase.h
 *  @brief Abstract client socket class.
 *
 * This class provides the base functionality for client sockets,
 * such as, and especially, name resolution and signals.
 *
 * @note This class is abstract. If you're looking for a normal,
 *       client socket class, see @ref KStreamSocket and KBufferedSocket
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KDECORE_EXPORT KClientSocketBase : public QObject, public KActiveSocketBase
{
  Q_OBJECT

public:
  /**
   * Socket states.
   *
   * These are the possible states for a KClientSocketBase:
   * - Idle: socket is not connected
   * - HostLookup: socket is doing host lookup prior to connecting
   * - HostFound: name lookup is complete
   * - Bound: the socket is locally bound
   * - Connecting: socket is attempting connection
   * - Open: socket is open
   * - Connected (=Open): socket is connected
   * - Connection (=Open): yet another name for a connected socket
   * - Closing: socket is shutting down
   *
   * Whenever the socket state changes, the @ref stateChanged(int) signal
   * will be emitted.
   */
  enum SocketState
  {
    Idle,
    HostLookup,
    HostFound,
    Bound,
    Connecting,
    Open,
    Closing,

    Unconnected = Bound,
    Connected = Open,
    Connection = Open
  };

public:
  /**
   * Default constructor.
   *
   * @param parent	the parent QObject object
   * @param name	the name of this object
   */
  KClientSocketBase(QObject* parent, const char *name);

  /**
   * Destructor.
   */
  virtual ~KClientSocketBase();

  /**
   * Returns the current state for this socket.
   * @see SocketState
   */
  SocketState state() const;

protected:
  /**
   * Sets the socket options. Reimplemented from KSocketBase.
   */
  virtual bool setSocketOptions(int opts);

public:
  /**
   * Returns the internal KResolver object used for
   * looking up the peer host name and service.
   *
   * This can be used to set extra options to the
   * lookup process other than the default values, as well
   * as obtaining the error codes in case of lookup failure.
   */
  KResolver& peerResolver() const;

  /**
   * Returns the internal list of resolved results for the peer address.
   */
  const KResolverResults& peerResults() const;

  /**
   * Returns the internal KResolver object used for
   * looking up the local host name and service.
   *
   * This can be used to set extra options to the
   * lookup process other than the default values, as well
   * as obtaining the error codes in case of lookup failure.
   */
  KResolver& localResolver() const;

  /**
   * Returns the internal list of resolved results for the local address.
   */
  const KResolverResults& localResults() const;

  /**
   * Enables or disables name resolution. If this flag is set to true,
   * @ref bind and @ref connect operations will trigger name lookup
   * operations (i.e., converting a hostname into its binary form).
   * If the flag is set to false, those operations will instead
   * try to convert a string representation of an address without
   * attempting name resolution.
   *
   * This is useful, for instance, when IP addresses are in
   * their string representation (such as "1.2.3.4") or come
   * from other sources like @ref KSocketAddress.
   *
   * @param enable	whether to enable
   */
  void setResolutionEnabled(bool enable);

  /**
   * Sets the allowed families for the resolutions.
   *
   * @param families		the families that we want/accept
   * @see KResolver::SocketFamilies for possible values
   */
  void setFamily(int families);

  /**
   * Starts the lookup for peer and local hostnames as
   * well as their services.
   *
   * If the blocking mode for this object is on, this function will
   * wait for the lookup results to be available (by calling the 
   * @ref KResolver::wait method on the resolver objects).
   *
   * When the lookup is done, the signal @ref hostFound will be
   * emitted (only once, even if we're doing a double lookup).
   * If the lookup failed (for any of the two lookups) the 
   * @ref gotError signal will be emitted with the appropriate
   * error condition (see @ref KSocketBase::SocketError).
   *
   * This function returns true on success and false on error. Note that
   * this is not the lookup result!
   */
  virtual bool lookup();

  /**
   * Binds this socket to the given nodename and service,
   * or use the default ones if none are given.
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
		    const QString& service = QString::null) = 0;

  /**
   * Reimplemented from KSocketBase. Connect this socket to this
   * specific address.
   *
   * Unlike @ref bind(const QString&, const QString&) above, this function
   * really does bind the socket. No lookup is performed. The @ref bound
   * signal will be emitted.
   */
  virtual bool bind(const KResolverEntry& address);

  /**
   * Attempts to connect to the these hostname and service,
   * or use the default ones if none are given. If a connection attempt
   * is already in progress, check on its state and set the error status
   * (NoError or InProgress).
   *
   * If the blocking mode for this object is on, this function will only
   * return when all the resolved peer addresses have been tried or when
   * a connection is established.
   *
   * Upon successfully connecting, the @ref connected signal 
   * will be emitted. If an error is found, the @ref gotError
   * signal will be emitted.
   *
   * @par Note for derived classes:
   *      Derived classes must implement this function. The implementation
   *      will set the parameters for the lookup (using the peer KResolver
   *      object) and call @ref lookup to start it.
   *
   * @par
   *      The implementation should use the @ref hostFound
   *      signal to be notified of the completion of the lookup process and
   *      then proceed to start the connection itself. Care should be taken
   *      regarding the value of @ref blocking flag.
   *
   * @param node	the nodename
   * @param service	the service
   */
  virtual bool connect(const QString& node = QString::null,
		       const QString& service = QString::null) = 0;

  /**
   * @overload
   * Reimplemented from KSocketBase.
   */
  virtual bool connect(const KResolverEntry& address);

  /**
   * @deprecated
   * This is a convenience function provided to ease migrating from
   * Qt 3.x's QSocket class.
   */
  inline void connectToHost(const QString& host, Q_UINT16 port)
  { connect(host, QString::number(port)); }

  /**
   * Disconnects the socket.
   * Note that not all socket types can disconnect.
   */
  virtual bool disconnect();

  /**
   * Opens the socket. Reimplemented from QIODevice.
   *
   * You should not call this function; instead, use @ref connect
   */
  virtual inline bool open(int)
  { return connect(); }

  /**
   * Closes the socket. Reimplemented from QIODevice.
   *
   * The closing of the socket causes the emission of the
   * signal @ref closed.
   */
  virtual void close();

  /**
   * This call is not supported on sockets. Reimplemented from QIODevice.
   */
  virtual void flush()
  { }

  /**
   * Returns the number of bytes available on this socket.
   * Reimplemented from KSocketBase.
   */
  virtual Q_LONG bytesAvailable() const;

  /**
   * Waits for more data. Reimplemented from KSocketBase.
   */
  virtual Q_LONG waitForMore(int msecs, bool *timeout = 0L);

  /**
   * Reads data from a socket. Reimplemented from KSocketBase.
   */
  virtual Q_LONG readBlock(char *data, Q_ULONG maxlen);

  /**
   * @overload
   * Reads data from a socket. Reimplemented from KSocketBase.
   */
  virtual Q_LONG readBlock(char *data, Q_ULONG maxlen, KSocketAddress& from);

  /**
   * Peeks data from the socket. Reimplemented from KSocketBase.
   */
  virtual Q_LONG peekBlock(char *data, Q_ULONG maxlen);

  /**
   * @overload
   * Peeks data from the socket. Reimplemented from KSocketBase.
   */
  virtual Q_LONG peekBlock(char *data, Q_ULONG maxlen, KSocketAddress &from);

  /**
   * Writes data to the socket. Reimplemented from KSocketBase.
   */
  virtual Q_LONG writeBlock(const char *data, Q_ULONG len);

  /**
   * @overload
   * Writes data to the socket. Reimplemented from KSocketBase.
   */
  virtual Q_LONG writeBlock(const char *data, Q_ULONG len, const KSocketAddress& to);

  /**
   * Returns the local socket address. Reimplemented from KSocketBase.
   */
  virtual KSocketAddress localAddress() const;

  /**
   * Returns the peer socket address. Reimplemented from KSocketBase.
   */
  virtual KSocketAddress peerAddress() const;

  /**
   * Returns true if the readyRead signal is set to be emitted.
   */
  bool emitsReadyRead() const;

  /**
   * Enables the emission of the readyRead signal.
   * By default, this signal is enabled.
   *
   * @param enable	whether to enable the signal
   */
  virtual void enableRead(bool enable);

  /**
   * Returns true if the readyWrite signal is set to be emitted.
   */
  bool emitsReadyWrite() const;

  /**
   * Enables the emission of the readyWrite signal.
   * By default, this signal is disabled.
   *
   * @param enable	whether to enable the signal
   */
  virtual void enableWrite(bool enable);

protected slots:
  // protected slots

  /**
   * This slot is connected to the read notifier's signal meaning
   * the socket can read more data. 
   *
   * The default implementation only emits the readyRead signal.
   *
   * Override if your class requires processing of incoming
   * data.
   */
  virtual void slotReadActivity();

  /**
   * This slot is connected to the write notifier's signal
   * meaning the socket can write more data.
   *
   * The default implementation only emits the readyWrite signal.
   *
   * Override if your class writes data from another source
   * (like a buffer).
   */
  virtual void slotWriteActivity();

private slots:
  void lookupFinishedSlot();

signals:
  /**
   * This signal is emitted whenever the socket state changes.
   *
   * Note: do not delete this object inside the slot called by this
   * signal.
   *
   * @param newstate	the new state of the socket object
   */
  void stateChanged(int newstate);

  /**
   * This signal is emitted when this object finds an error.
   * The @p code parameter contains the error code that can
   * also be found by calling @ref error.
   */
  void gotError(int code);

  /**
   * This signal is emitted when the lookup is successfully completed.
   */
  void hostFound();

  /**
   * This signal is emitted when the socket successfully binds
   * to an address.
   *
   * @param local	the local address we bound to
   */
  void bound(const KResolverEntry& local);

  /**
   * This signal is emitted when the socket is about to connect
   * to an address (but before doing so).
   * 
   * The @p skip parameter can be used to make the loop skip this address.
   * Its value is initially false: change it to true if you want to
   * skip the current address (as given by @p remote).
   *
   * This function is also useful if one wants to reset the timeout.
   *
   * @param remote	the address we're about to connect to
   * @param skip	set to true if you want to skip this address
   * @note if the connection is successful, the @ref connected signal will be
   *       emitted.
   */
  void aboutToConnect(const KResolverEntry& remote, bool& skip);

  /**
   * This socket is emitted when the socket successfully connects
   * to a remote address.
   *
   * @param remote	the remote address we did connect to
   */
  void connected(const KResolverEntry& remote);

  /**
   * This signal is emitted when the socket completes the
   * closing/shut down process.
   */
  void closed();

  /**
   * This signal is emitted whenever the socket is ready for
   * reading -- i.e., there is data to be read in the buffers.
   * The subsequent read operation is guaranteed to be non-blocking.
   *
   * You can toggle the emission of this signal with the @ref enableRead
   * function. This signal is by default enabled.
   */
  void readyRead();

  /**
   * This signal is emitted whenever the socket is ready for 
   * writing -- i.e., whenever there's space available in the buffers
   * to receive more data. The subsequent write operation is
   * guaranteed to be non-blocking.
   *
   * You can toggle the emission of this signal with the @ref enableWrite
   * function. This signal is by default disabled. You will
   * want to disable this signal after the first reception, since
   * it'll probably fire at every event loop.
   */
  void readyWrite();

protected:
  /**
   * Sets the socket state to @p state. This function does not
   * emit the @ref stateChanged signal.
   */
  void setState(SocketState state);

  /**
   * This function is called by @ref setState whenever the state
   * changes. You should override it if you need to specify any
   * actions to be done when the state changes.
   *
   * The default implementation acts for these states only:
   *  - Connected: it sets up the socket notifiers to fire readyRead and 
   *               readyWrite signals.
   */
  virtual void stateChanging(SocketState newState);

  /**
   * Convenience function to set this object's error code to match
   * that of the socket device.
   */
  void copyError();

private:
  KClientSocketBase(const KClientSocketBase&);
  KClientSocketBase& operator=(const KClientSocketBase&);

  KClientSocketBasePrivate *d;
};

}				// namespace KNetwork

#endif
