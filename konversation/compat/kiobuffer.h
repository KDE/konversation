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

#ifndef KIOBUFFER_H
#define KIOBUFFER_H

#include <qglobal.h>

class QIODevice;

/**
 * @class KIOBufferBase kiobuffer.h kiobuffer.h
 * @brief base for I/O buffer implementation
 *
 * This class declares the base methods to interface with an I/O buffer.
 * Most applications will not need to access this class directly, since
 * it is all handled by @ref KBufferedSocket and other buffering classes.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KIOBufferBase
{
public:
  /**
   * Default constructor. Does nothing.
   */
  KIOBufferBase()
  { }

  /**
   * Copy constructor. Does nothing here.
   */
  KIOBufferBase(const KIOBufferBase& )
  { }

  /**
   * Virtual destructor. Does nothing.
   */
  virtual ~KIOBufferBase()
  { }

  /**
   * Assignment operator. Does nothing.
   */
  KIOBufferBase& operator=(const KIOBufferBase& )
  { return *this; }

  /**
   * Returns true if a line can be read from the buffer.
   */
  virtual bool canReadLine() const = 0;

  /**
   * Reads a line from the buffer and discards it.
   */
  virtual QCString readLine() = 0;

  /**
   * Returns the number of bytes in the buffer. Note that this is not
   * the size of the buffer.
   *
   * @sa size
   */
  virtual Q_LONG length() const = 0;

  /**
   * Returns true if the buffer is empty of data.
   */
  inline bool isEmpty() const
  { return length() == 0; }

  /**
   * Retrieves the buffer size. The value of -1 indicates that
   * the buffer has no defined upper limit.
   *
   * @sa length for the length of the data stored
   */
  virtual Q_LONG size() const = 0;

  /**
   * Returns true if the buffer is full (i.e., cannot receive more data)
   */
  inline bool isFull() const
  { return size() != -1 && size() == length(); }

  /**
   * Sets the size of the buffer, if allowed.
   *
   * @param size	the maximum size, use -1 for unlimited.
   * @returns true on success, false if an error occurred.
   * @note if the new size is less than length(), the buffer will be truncated
   */
  virtual bool setSize(Q_LONG size) = 0;

  /**
   * Adds data to the end of the buffer.
   *
   * @param data	the data to be added
   * @param len		the data length, in bytes
   * @returns the number of bytes added to the end of the buffer.
   */
  virtual Q_LONG feedBuffer(const char *data, Q_LONG len) = 0;

  /**
   * Consumes data from the beginning of the buffer.
   *
   * @param data	where to copy the data to
   * @param maxlen	the maximum length to copy, in bytes
   * @param discard	if true, the bytes copied will be discarded
   * @returns the number of bytes copied from the buffer
   */
  virtual Q_LONG consumeBuffer(char *data, Q_LONG maxlen, bool discard = true) = 0;

  /**
   * Clears the buffer.
   */
  virtual void clear() = 0;
};

#endif
