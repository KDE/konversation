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

#ifndef KREVERSERESOLVER_H
#define KREVERSERESOLVER_H

//////////////////
// Needed includes
#include <qobject.h>
#include <qstring.h>

#include "ksocketaddress.h"

namespace KNetwork {

class KReverseResolverPrivate;
/** @class KReverseResolver kreverseresolver.h kreverseresolver.h
 *  @brief Run a reverse-resolution on a socket address.
 *
 * This class is provided as a counterpart to KResolver in such a way
 * as it produces a reverse resolution: it resolves a socket address
 * from its binary representations into a textual representation.
 *
 * Most users will use the static functions @ref resolve, which work
 * both synchronously (blocking) and asynchronously (non-blocking).
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KReverseResolver: public QObject
{
  Q_OBJECT

public:
  /**
   * Flags for the reverse resolution.
   *
   * These flags are used by the reverse resolution functions for
   * setting resolution parameters. The possible values are:
   * @li NumericHost: don't try to resolve the host address to a text form.
   *		Instead, convert the address to its numeric textual representation.
   * @li NumericService: the same as NumericHost, but for the service name
   * @li NodeNameOnly: returns the node name only (i.e., not the Fully
   *		Qualified Domain Name)
   * @li Datagram: in case of ambiguity in the service name, prefer the
   *		name associated with the datagram protocol
   * @li NumericScope: for those addresses which have the concept of scope,
   *            resolve using the numeric value instead of the proper scope name.
   * @li ResolutionRequired: normally, when resolving, if the name resolution
   *            fails, the process normally converts the numeric address into its
   *            presentation forms. This flag causes the function to return
   *            with error instead.
   */
  enum Flags
    {
      NumericHost = 0x01,
      NumericService = 0x02,
      NodeNameOnly = 0x04,
      Datagram = 0x08,
      NumericScope = 0x10,
      ResolutionRequired = 0x20
    };

  /**
   * Constructs this object to resolve the given socket address.
   *
   * @param addr	the address to resolve
   * @param flags	the flags to use, see @ref Flags
   */
  KReverseResolver(const KSocketAddress& addr, int flags = 0,
		   QObject * = 0L, const char * = 0L);

  /**
   * Destructor.
   */
  virtual ~KReverseResolver();

  /**
   * This function returns 'true' if the processing is still running.
   */
  bool isRunning() const;

  /**
   * This function returns true if the processing has finished with
   * success, false if it's still running or failed.
   */
  bool success() const;

  /**
   * This function returns true if the processing has finished with
   * failure, false if it's still running or succeeded.
   */
  bool failure() const;

  /**
   * Returns the resolved node name, if the resolution has finished 
   * successfully, or QString::null otherwise.
   */
  QString node() const;

  /**
   * Returns the resolved service name, if the resolution has finished
   * successfully, or QString::null otherwise.
   */
  QString service() const;

  /**
   * Returns the socket address which was subject to resolution.
   */
  const KSocketAddress& address() const;

  /**
   * Starts the resolution. This function returns 'true'
   * if the resolution has started successfully.
   */
  bool start();

  /**
   * Overrides event handling
   */
  virtual bool event(QEvent* );

signals:
  /**
   * This signal is emitted when the resolution has finished.
   *
   * @param obj		this class, which contains the results
   */
  void finished(const KReverseResolver& obj);

public:
  /**
   * Resolves a socket address to its textual representation
   *
   * FIXME!! How can we do this in a non-blocking manner!?
   *
   * This function is used to resolve a socket address from its
   * binary representation to a textual form, even if numeric only.
   *
   * @param addr	the socket address to be resolved
   * @param node	the QString where we will store the resolved node
   * @param serv	the QString where we will store the resolved service
   * @param flags	flags to be used for this resolution.
   * @return true if the resolution succeeded, false if not
   * @see ReverseFlags for the possible values for @p flags
   */
  static bool resolve(const KSocketAddress& addr, QString& node, 
		      QString& serv, int flags = 0);

  /**
   * Resolves a socket address to its textual representation
   *
   * FIXME!! How can we do this in a non-blocking manner!?
   *
   * This function behaves just like the above one, except it takes
   * a sockaddr structure and its size as parameters.
   *
   * @param sa	the sockaddr structure containing the address to be resolved
   * @param salen	the length of the sockaddr structure
   * @param node	the QString where we will store the resolved node
   * @param serv	the QString where we will store the resolved service
   * @param flags	flags to be used for this resolution.
   * @return true if the resolution succeeded, false if not
   * @see ReverseFlags for the possible values for @p flags
   */
  static bool resolve(const struct sockaddr* sa, Q_UINT16 salen, 
		      QString& node, QString& serv, int flags = 0);

private:
  KReverseResolverPrivate* d;
};

}				// namespace KNetwork

#endif
