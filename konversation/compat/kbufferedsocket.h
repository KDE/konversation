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

#ifndef KBUFFEREDSOCKET_H
#define KBUFFEREDSOCKET_H

#include <qobject.h>
#include <qcstring.h>
#include <qvaluelist.h>
#include "kstreamsocket.h"

class KIOBufferBase;

namespace KNetwork {

class KBufferedSocketPrivate;
/** @class KBufferedSocket kbufferedsocket.h kbufferedsocket.h
 *  @brief Buffered stream sockets.
 *
 * This class allows the user to create and operate buffered stream sockets
 * such as those used in most Internet connections. This class is
 * also the one that resembles the most to the old @ref QSocket implementation.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KBufferedSocket: public KStreamSocket
{
  Q_OBJECT
public:
  /**
   * Default constructor.
   *
   * @param node	destination host
   * @param service	destination service to connect to
   */
  KBufferedSocket(const QString& host = QString::null, const QString& service = QString::null,
		  QObject* parent = 0L, const char *name = 0L);

  /**
   * Destructor.
   */
  virtual ~KBufferedSocket();

  /**
   * Be sure to catch new devices.
   */
  virtual void setSocketDevice(KSocketDevice* device);

protected:
  /**
   * Buffered sockets can only operate in blocking mode.
   */
  virtual bool setSocketOptions(int opts);

public:
  /**
   * Closes the socket for new data, but allow data that had been buffered
   * for output with @ref writeBlock to be still be written.
   *
   * @sa closeNow
   */
  virtual void close();

  /**
   * Make use of the buffers.
   */
  virtual Q_LONG bytesAvailable() const;

  /**
   * Make use of buffers.
   */
  virtual Q_LONG waitForMore(int msecs, bool *timeout = 0L);

  /**
   * Reads data from the socket. Make use of buffers.
   */
  virtual Q_LONG readBlock(char *data, Q_ULONG maxlen);

  /**
   * @overload
   * Reads data from a socket.
   *
   * The @p from parameter is always set to @ref peerAddress()
   */
  virtual Q_LONG readBlock(char *data, Q_ULONG maxlen, KSocketAddress& from);

  /**
   * Peeks data from the socket.
   */
  virtual Q_LONG peekBlock(char *data, Q_ULONG maxlen);

  /**
   * @overload
   * Peeks data from the socket.
   *
   * The @p from parameter is always set to @ref peerAddress()
   */
  virtual Q_LONG peekBlock(char *data, Q_ULONG maxlen, KSocketAddress &from);

  /**
   * Writes data to the socket.
   */
  virtual Q_LONG writeBlock(const char *data, Q_ULONG len);

  /**
   * @overload
   * Writes data to the socket.
   *
   * The @p to parameter is discarded.
   */
  virtual Q_LONG writeBlock(const char *data, Q_ULONG len, const KSocketAddress& to);

  /**
   * Catch changes.
   */
  virtual void enableRead(bool enable);

  /**
   * Catch changes.
   */
  virtual void enableWrite(bool enable);

  /**
   * Sets the use of input buffering.
   */
  void setInputBuffering(bool enable);

  /**
   * Retrieves the input buffer object.
   */
  KIOBufferBase* inputBuffer();

  /**
   * Sets the use of output buffering.
   */
  void setOutputBuffering(bool enable);

  /**
   * Retrieves the output buffer object.
   */
  KIOBufferBase* outputBuffer();

  /**
   * Returns the length of the output buffer.
   */
  virtual Q_ULONG bytesToWrite() const;

  /**
   * Closes the socket and discards any output data that had been buffered
   * with @ref writeBlock but that had not yet been written.
   *
   * @sa close
   */
  virtual void closeNow();

  /**
   * Returns true if a line can be read with @ref readLine
   */
  bool canReadLine() const;

  /**
   * Reads a line of data from the socket buffers.
   */
  QCString readLine();

protected:
  /**
   * Catch connection to clear the buffers
   */
  virtual void stateChanging(SocketState newState);

protected slots:
  /**
   * Slot called when there's read activity.
   */
  virtual void slotReadActivity();

  /**
   * Slot called when there's write activity.
   */
  virtual void slotWriteActivity();

signals:
  /**
   * This signal is emitted whenever data is written.
   */
  void bytesWritten(int bytes);

private:
  KBufferedSocket(const KBufferedSocket&);
  KBufferedSocket& operator=(const KBufferedSocket&);

  KBufferedSocketPrivate *d;

public:
  // KDE4: remove this function
  /**
   * @deprecated
   * Closes the socket.
   *
   * This function is provided to ease porting from KExtendedSocket,
   * which required a call to reset() in order to be able to connect again
   * using the same device. This is not necessary in KBufferedSocket any more.
   */
  inline void reset()
  { closeNow(); }
};

}				// namespace KNetwork

#endif
