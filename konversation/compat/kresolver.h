/*  -*- mode: C++; coding: utf-8; -*-
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

#ifndef KRESOLVER_H
#define KRESOLVER_H

//////////////////
// Needed includes
#include <qvaluelist.h>
#include <qobject.h>
#include "ksocketaddress.h"


////////////////////////
// Forward declarations
struct sockaddr;
class QString;
class QCString;
class QStrList;

//////////////////
// Our definitions

namespace KNetwork {

  namespace Internal { class KResolverManager; }

class KResolverEntryPrivate;
/** @class KResolverEntry kresolver.h kresolver.h
 *  @brief One resolution entry.
 *
 * This class is one element in the resolution results list.
 * It contains the socket address for connecting, as well as
 * a bit more of information: the socket type, address family
 * and protocol numbers.
 *
 * This class contains all the information required for creating,
 * binding and connecting a socket.
 *
 * KResolverEntry objects implicitly share data, so copying them
 * is quite efficient.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KResolverEntry
{
public:
  /**
   * Default constructor
   *
   */
  KResolverEntry();

  /**
   * Constructs a new KResolverEntry from a KSocketAddress
   * and other data.
   *
   * The KSocketAddress @p addr parameter will be deep-copied.
   *
   * @param addr	the address that was resolved
   * @param socktype	the socket type of the resolved address
   * @param protocol	the protocol of the resolved address
   * @param canonName	the canonical name of the resolved hostname
   * @param encodedName	the ASCII-compatible encoding of the hostname
   */
  KResolverEntry(const KSocketAddress& addr, int socktype, int protocol,
		const QString& canonName = QString::null,
		const QCString& encodedName = QCString());

  /**
   * Constructs a new KResolverEntry from raw forms of
   * socket addresses and other data.
   *
   * This constructor instead creates an internal KSocketAddress object.
   *
   * @param sa		the sockaddr structure containing the raw address
   * @param salen	the length of the sockaddr structure
   * @param socktype	the socket type of the resolved address
   * @param protocol	the protocol of the resolved address
   * @param canonName	the canonical name of the resolved hostname
   * @param encodedName	the ASCII-compatible encoding of the hostname
   */
  KResolverEntry(const struct sockaddr *sa, Q_UINT16 salen, int socktype,
		int protocol, const QString& canonName = QString::null,
		const QCString& encodedName = QCString());

  /**
   * Copy constructor.
   *
   * This constructor performs a shallow-copy of the other object.
   */
  KResolverEntry(const KResolverEntry &other);

  /**
   * Destructor.
   *
   * The destructor frees associated resources with this object. It does
   * not destroy shared data.
   */
  ~KResolverEntry();

  /**
   * Retrieves the socket address associated with this entry.
   */
  KSocketAddress address() const;

  /**
   * Retrieves the length of the socket address structure.
   */
  Q_UINT16 length() const;

  /**
   * Retrieves the family associated with this socket address.
   */
  int family() const;

  /**
   * Retrieves the canonical name associated with this entry, if there is any.
   * If the canonical name was not found, this function returns QString::null.
   */
  QString canonicalName() const;

  /**
   * Retrieves the encoded domain name associated with this entry, if there is
   * any. If this domain has been resolved through DNS, this will be the
   * the ACE-encoded hostname.
   *
   * Returns a null QCString if such information is not available.
   *
   * Please note that this information is NOT to be presented to the user,
   * unless requested.
   */
  QCString encodedName() const;

  /**
   * Retrieves the socket type associated with this entry.
   */
  int socketType() const;

  /**
   * Retrieves the protocol associated with this entry.
   */
  int protocol() const;

  /**
   * Assignment operator
   *
   * This function copies the contents of the other object into this one.
   * Data will be shared between the two of them.
   */
  KResolverEntry& operator=(const KResolverEntry& other);

private:
  KResolverEntryPrivate* d;
};

class KResolverResultsPrivate;
/**
 * @class KResolverResults kresolver.h kresolver.h
 * @brief Name and service resolution results.
 *
 * This object contains the results of a name and service resolution, as
 * those performed by @ref KResolver. It is also a descendant of QValueList, so
 * you may use all its member functions here to access the elements.
 *
 * A KResolverResults object is associated with a resolution, so, in addition
 * to the resolved elements, you can also retrieve information about the 
 * resolution process itself, like the nodename that was resolved or an error
 * code.
 *
 * Note Resolver also uses KResolverResults objects to indicate failure, so
 * you should test for failure.
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KResolverResults: public QValueList<KResolverEntry>
{
public:
  /**
   * Default constructor.
   *
   * Constructs an empty list.
   */
  KResolverResults();

  /**
   * Copy constructor
   *
   * Creates a new object with the contents of the other one. Data will be
   * shared by the two objects, like QValueList
   */
  KResolverResults(const KResolverResults& other);

  /**
   * Destructor
   *
   * Destroys the object and frees associated resources.
   */
  virtual ~KResolverResults();

  /**
   * Assignment operator
   *
   * Copies the contents of the other container into this one, discarding
   * our current values.
   */
  KResolverResults& operator=(const KResolverResults& other);

  /**
   * Retrieves the error code associated with this resolution. The values
   * here are the same as in @ref KResolver::ErrorCodes.
   */
  int error() const;

  /**
   * Retrieves the system error code, if any.
   * @see KResolver::systemError for more information
   */
  int systemError() const;

  /**
   * Sets the error codes
   *
   * @param errorcode		the error code in @ref KResolver::ErrorCodes
   * @param systemerror	the system error code associated, if any
   */
  void setError(int errorcode, int systemerror = 0);

  /**
   * The nodename to which the resolution was performed.
   */
  QString nodeName() const;

  /**
   * The service name to which the resolution was performed.
   */
  QString serviceName() const;

  /**
   * Sets the new nodename and service name
   */
  void setAddress(const QString& host, const QString& service);

protected:
  virtual void virtual_hook( int id, void* data );
private:
  KResolverResultsPrivate* d;
};

class KResolverPrivate;
/**
 * @class KResolver kresolver.h kresolver.h
 * @brief Name and service resolution class.
 *
 * This class provides support for doing name-to-binary resolution
 * for nodenames and service ports. You should use this class if you
 * need specific resolution techniques when creating a socket or if you
 * want to inspect the results before calling the socket functions.
 *
 * You can either create an object and set the options you want in it
 * or you can simply call the static member functions, which will create
 * standard Resolver objects and dispatch the resolution for you. Normally,
 * the static functions will be used, except in cases where specific options
 * must be set.
 *
 * A Resolver object defaults to the following:
 * @li address family: any address family
 * @li socket type: streaming socket
 * @li protocol: implementation-defined. Generally, TCP
 * @li host and service: unset
 *
 * @author Thiago Macieira <thiago.macieira@kdemail.net>
 */
class KResolver: public QObject
{
  Q_OBJECT

public:

  /**
   * Address family selection types
   *
   * These values can be OR-ed together to form a composite family selection.
   *
   * @li UnknownFamily: a family that is unknown to the current implementation
   * @li KnownFamily: a family that is known to the implementation (the exact
   *		opposite of UnknownFamily)
   * @li AnyFamilies: any address family is acceptable
   * @li InternetFamily: an address for connecting to the Internet
   * @li InetFamily: alias for InternetFamily
   * @li IPv6Family: an IPv6 address only
   * @li IPv4Family: an IPv4 address only
   * @li UnixFamily: an address for the local Unix namespace (i.e., Unix sockets)
   * @li LocalFamily: alias for UnixFamily
   */
  enum SocketFamilies
  {
    UnknownFamily = 0x0001,

    UnixFamily = 0x0002,
    LocalFamily = UnixFamily,

    IPv4Family = 0x0004,
    IPv6Family = 0x0008,
    InternetFamily = IPv4Family | IPv6Family,
    InetFamily = InternetFamily,

    KnownFamily = ~UnknownFamily,
    AnyFamily = KnownFamily | UnknownFamily
  };

  /**
   * Flags for the resolution.
   *
   * These flags are used for setting the resolution behaviour for this
   * object:
   * @li Passive: resolve to a passive socket (i.e., one that can be used for
   *		binding to a local interface)
   * @li CanonName: request that the canonical name for the given nodename
   *		be found and recorded
   * @li NoResolve: request that no external resolution be performed. The given
   *		nodename and servicename will be resolved locally only.
   * @li NoSrv: don't try to use SRV-based name-resolution.
   * @li Multiport: the port/service argument is a list of port numbers and
   *		ranges. (future extension)
   *
   * @note SRV-based lookup and Multiport are not implemented yet.
   */
  enum Flags
    {
      Passive = 0x01,
      CanonName = 0x02,
      NoResolve = 0x04,
      NoSrv = 0x08,
      Multiport = 0x10
    };

  /**
   * Error codes
   *
   * These are the possible error values that objects of this class
   * may return. See @ref strError for getting a string representation
   * for these errors.
   *
   * @li AddrFamily: Address family for the given nodename is not supported.
   * @li TryAgain: Temporary failure in name resolution. You should try again.
   * @li NonRecoverable: Non-recoverable failure in name resolution.
   * @li BadFlags: Invalid flags were given.
   * @li Memory: Memory allocation failure.
   * @li NoName: The specified name or service doesn't exist.
   * @li UnsupportedFamily: The requested socket family is not supported.
   * @li UnsupportedService: The requested service is not supported for this
   *		socket type (i.e., a datagram service in a streaming socket).
   * @li UnsupportedSocketType: The requested socket type is not supported.
   * @li UnknownError: An unknown, unexpected error occurred.
   * @li SystemError: A system error occurred. See @ref systemError.
   * @li Canceled: This request was cancelled by the user.
   */
  enum ErrorCodes
    {
      // note: if you change this enum, take a look at KResolver::strError
      NoError = 0,
      AddrFamily = -1,
      TryAgain = -2,
      NonRecoverable = -3,
      BadFlags = -4,
      Memory = -5,
      NoName = -6,
      UnsupportedFamily = -7,
      UnsupportedService = -8,
      UnsupportedSocketType = -9,
      UnknownError = -10,
      SystemError = -11,
      Canceled = -100
    };

  /**
   * Status codes.
   *
   * These are the possible status for a Resolver object. A value
   * greater than zero indicates normal behaviour, while negative
   * values either indicate failure or error.
   *
   * @li Idle: resolution has not yet been started.
   * @li Queued: resolution is queued but not yet in progress.
   * @li InProgress: resolution is in progress.
   * @li PostProcessing: resolution is in progress.
   * @li Success: resolution is done; you can retrieve the results.
   * @li Canceled: request cancelled by the user.
   * @li Failed: resolution is done, but failed.
   *
   * Note: the status Canceled and the error code Canceled are the same.
   *
   * Note 2: the status Queued and InProgress might not be distinguishable.
   * Some implementations might not differentiate one from the other.
   */
  enum StatusCodes
    {
      Idle = 0,
      Queued = 1,
      InProgress = 5,
      PostProcessing = 6,
      Success = 10,
      //Canceled = -100,	// already defined above
      Failed = -101
    };

  /**
   * Default constructor.
   *
   * Creates an empty Resolver object. You should set the wanted
   * names and flags using the member functions before starting
   * the name resolution.
   */
  KResolver(QObject * = 0L, const char * = 0L);

  /**
   * Constructor with host and service names.
   *
   * Creates a Resolver object with the given host and
   * service names. Flags are initialised to 0 and any address family
   * will be accepted.
   *
   * @param nodename	The host name we want resolved.
   * @param servicename	The service name associated, like "http".
   */
  KResolver(const QString& nodename, const QString& servicename = QString::null,
	    QObject * = 0L, const char * = 0L);

  /**
   * Destructor.
   *
   * When this object is deleted, it'll destroy all associated
   * resources. If the resolution is still in progress, it will be
   * cancelled and the signal will \b not be emitted.
   */
  virtual ~KResolver();

  /**
   * Retrieve the current status of this object.
   *
   * @see StatusCodes for the possible status codes.
   */
  int status() const;

  /**
   * Retrieve the error code in this object.
   *
   * This function will return NoError if we are not in
   * an error condition. See @ref status and @ref StatusCodes to
   * find out what the current status is.
   *
   * @see errorString for getting a textual representation of
   * this error
   */
  int error() const;

  /**
   * Retrieve the associated system error code in this object.
   *
   * Many resolution operations may generate an extra error code
   * as given by the C errno variable. That value is stored in the
   * object and can be retrieved by this function.
   */
  int systemError() const;

  /**
   * Returns the textual representation of the error in this object.
   */
  inline QString errorString() const
  { return errorString(error(), systemError()); }

  /**
   * Returns true if this object is currently running
   */
  bool isRunning() const;

  /**
   * The nodename to which the resolution was/is to be performed.
   */
  QString nodeName() const;

  /**
   * The service name to which the resolution was/is to be performed.
   */
  QString serviceName() const;

  /**
   * Sets the nodename for the resolution.
   *
   * Set the nodename to QString::null to unset it.
   * @param nodename		The nodename to be resolved.
   */
  void setNodeName(const QString& nodename);

  /**
   * Sets the service name to be resolved.
   *
   * Set it to QString::null to unset it.
   * @param service		The service to be resolved.
   */
  void setServiceName(const QString& service);

  /**
   * Sets both the host and the service names.
   *
   * Setting either value to QString::null will unset them.
   * @param node		The nodename
   * @param service		The service name
   */
  void setAddress(const QString& node, const QString& service);

  /**
   * Retrieves the flags set for the resolution.
   *
   * @see Flags for an explanation on what flags are possible
   */
  int flags() const;

  /**
   * Sets the flags.
   *
   * @param flags		the new flags
   * @return			the old flags
   * @see Flags for an explanation on the flags
   */
  int setFlags(int flags);

  /**
   * Sets the allowed socket families.
   *
   * @param families		the families that we want/accept
   * @see SocketFamilies for possible values
   */
  void setFamily(int families);

  /**
   * Sets the socket type we want.
   *
   * The values for the @p type parameter are the SOCK_*
   * constants, defined in <sys/socket.h>. The most common
   * values are:
   *  @li SOCK_STREAM		streaming socket (= reliable, sequenced,
   *				connection-based)
   *  @li SOCK_DGRAM		datagram socket (= unreliable, connectionless)
   *  @li SOCK_RAW		raw socket, with direct access to the
   *				container protocol (such as IP)
   *
   * These three are the only values to which it is guaranteed that
   * resolution will work. Some systems may define other constants (such as
   * SOCK_RDM for reliable datagrams), but support is implementation-defined.
   * 
   * @param type		the wanted socket type (SOCK_* constants). Set
   *				0 to use the default.
   */
  void setSocketType(int type);

  /**
   * Sets the protocol we want.
   *
   * Protocols are dependant on the selected address family, so you should know
   * what you are doing if you use this function. Besides, protocols generally
   * are either stream-based or datagram-based, so the value of the socket
   * type is also important. The resolution will fail if these values don't match.
   *
   * When using an Internet socket, the values for the protocol are the
   * IPPROTO_* constants, defined in <netinet/in.h>.
   *
   * You may choose to set the protocol either by its number or by its name, or
   * by both. If you set:
   * @li the number and the name: both values will be stored internally; you
   *		may set the name to an empty value, if wanted
   * @li the number only (name = NULL): the name will be searched in the 
   *		protocols database
   * @li the name only (number = 0): the number will be searched in the
   *		database
   * @li neither name nor number: reset to default behaviour
   *
   * @param protonum		the protocol number we want
   * @param name		the protocol name
   */
  void setProtocol(int protonum, const char *name = 0L);

  /**
   * Starts the name resolution asynchronously.
   *
   * This function will queue this object for resolution
   * and will return immediately. The status upon exit will either be
   * Queued or InProgress or Failed.
   *
   * This function does nothing if the object is already queued. But if
   * it had already succeeded or failed, this function will re-start it.
   *
   * Note: if both the nodename and the servicename are unset, this function
   * will not queue, but will set a success state and emit the signal. Also
   * note that in this case and maybe others, the signal @ref finished might
   * be emitted before this function returns.
   *
   * @return true if this request was successfully queued for asynchronous
   *		resolution
   */
  bool start();

  /**
   * Waits for a request to finish resolving.
   *
   * This function will wait on a running request for its termination. The
   * status upon exit will either be Success or Failed or Canceled.
   *
   * This function may be called from any thread, even one that is not the
   * GUI thread or the one that started the resolution process. But note this
   * function is not thread-safe nor reentrant: i.e., only one thread can be
   * waiting on one given object.
   *
   * Also note that this function ensures that the @ref finished signal is
   * emitted before it returns. That means that, as a side-effect, whenever
   * wait() is called, the signal is emitted on the thread calling wait().
   *
   * @param msec		the time to wait, in milliseconds or 0 to
   *				wait forever
   * @return true if the resolution has finished processing, even when it
   *         failed or was canceled. False means the wait timed out and
   *         the resolution is still running.
   */
  bool wait(int msec = 0);

  /**
   * Cancels a running request
   *
   * This function will cancel a running request. If the request is not
   * currently running or queued, this function does nothing.
   *
   * Note: if you tell the signal to be emitted, be aware that it might
   * or might not be emitted before this function returns.
   *
   * @param emitSignal	whether to emit the @ref finished signal or not
   */
  void cancel(bool emitSignal = true);

  /**
   * Retrieves the results of this resolution
   *
   * Use this function to retrieve the results of the resolution. If no
   * data was resolved (yet) or if we failed, this function will return
   * an empty object.
   *
   * @return the resolved data
   * @see status for information on finding out if the resolution was successful
   */
  KResolverResults results() const;

  /**
   * Handles events. Reimplemented from QObject.
   *
   * This function handles the events generated by the manager indicating that
   * this object has finished processing.
   *
   * Do not post events to this object.
   */
  virtual bool event(QEvent*);

signals:
  // signals

  /**
   * This signal is emitted whenever the resolution is finished, one
   * way or another (success or failure). The @p results parameter
   * will contain the resolved data.
   *
   * Note: if you are doing multiple resolutions, you can use the 
   * QObject::sender() function to distinguish one Resolver object from
   * another.
   *
   * @param results		the resolved data; might be empty if the resolution
   *			failed
   * @see results for information on what the results are
   *
   * @note This signal is @b always delivered in the GUI event thread, even for
   *       resolutions that were started in secondary threads.
   */
  void finished(KResolverResults results);

private:
  void emitFinished();

public:
  // Static functions

  /**
   * Returns the string representation of this error code.
   *
   * @param errorcode	the error code. See @ref ErrorCodes.
   * @param syserror	the system error code associated.
   * @return		the string representation. This is already
   *			i18n'ed.
   */
  static QString errorString(int errorcode, int syserror = 0);

  /**
   * Resolve the nodename and service name synchronously
   *
   * This static function is provided as convenience for simplifying
   * name resolution. It resolves the given host and service names synchronously
   * and returns the results it found. It is equivalent to the following code:
   *
   * \code
   *   KResolver qres(host, service);
   *   qres.setFlags(flags);
   *   qres.setFamily(families)
   *   qres.start();
   *   qres.wait();
   *   return qres.results();
   * \endcode
   *
   * @param host		the nodename to resolve
   * @param service		the service to resolve
   * @param flags		flags to be used
   * @param families		the families to be searched
   * @return a KResolverResults object containing the results
   * @see KResolverResults for information on how to obtain the error code
   */
  static KResolverResults resolve(const QString& host, const QString& service,
				 int flags = 0, int families = KResolver::InternetFamily);

  /**
   * Start an asynchronous name resolution
   *
   * This function is provided as a convenience to simplify the resolution
   * process. It creates an internal KResolver object, connects the
   * @ref finished signal to the given slot and starts the resolution
   * asynchronously. It is more or less equivalent to the following code:
   *
   * \b Note: this function may trigger the signal before it returns, so
   * your code must be prepared for this situation.
   *
   * \code
   *   KResolver* qres = new KResolver(host, service);
   *   QObject::connect(qres, SIGNAL(finished(KResolverResults)),
   *			  userObj, userSlot);
   *   qres->setFlags(flags);
   *   qres->setFamily(families);
   *   return qres->start();
   * \endcode
   *
   * You should use it like this in your code:
   * \code
   *   KResolver::resolveAsync(myObj, SLOT(mySlot(KResolverResults)), host, service);
   * \endcode
   *
   * @param userObj		the object whose slot @p userSlot we will connect
   * @param userSlot		the slot to which we'll connect
   * @param host		the nodename to resolve
   * @param service		the service to resolve
   * @param flags		flags to be used
   * @param families		families to be searcheed
   * @return true if the queueing was successful, false if not
   * @see KResolverResults for information on how to obtain the error code
   */
  static bool resolveAsync(QObject* userObj, const char *userSlot,
			   const QString& host, const QString& service,
			   int flags = 0, int families = KResolver::InternetFamily);

  /**
   * Returns the domain name in an ASCII Compatible Encoding form, suitable
   * for DNS lookups. This is the base for International Domain Name support
   * over the Internet.
   *
   * Note this function may fail, in which case it'll return a null 
   * QCString. Reasons for failure include use of unknown code
   * points (Unicode characters).
   *
   * Note that the encoding is illegible and, thus, should not be presented
   * to the user, except if requested.
   *
   * @param unicodeDomain	the domain name to be encoded
   * @return the ACE-encoded suitable for DNS queries if successful, a null
   *	     QCString if failure.
   */
  static QCString domainToAscii(const QString& unicodeDomain);

  /**
   * Does the inverse of @ref domainToAscii and return an Unicode domain
   * name from the given ACE-encoded domain.
   *
   * This function may fail if the given domain cannot be successfully
   * converted back to Unicode. Reasons for failure include a malformed
   * domain name or good ones whose reencoding back to ACE don't match
   * the form given here (e.g., ACE-encoding of an already
   * ASCII-compatible domain).
   *
   * It is, however, guaranteed that domains returned
   * by @ref domainToAscii will work.
   *
   * @param asciiDomain	the ACE-encoded domain name to be decoded
   * @return the Unicode representation of the given domain name
   * if successful, the original string if not
   * @note ACE = ASCII-Compatible Encoding, i.e., 7-bit
   */
  static QString domainToUnicode(const QCString& asciiDomain);

  /**
   * The same as above, but taking a QString argument.
   *
   * @param asciiDomain	the ACE-encoded domain name to be decoded
   * @return the Unicode representation of the given domain name
   * if successful, QString::null if not.
   */
  static QString domainToUnicode(const QString& asciiDomain);

  /**
   * Normalise a domain name.
   *
   * In order to prevent simple mistakes in International Domain
   * Names (IDN), it has been decided that certain code points
   * (characters in Unicode) would be instead converted to others.
   * This includes turning them all to lower case, as well certain
   * other specific operations, as specified in the documents.
   *
   * For instance, the German 'ß' will be changed into 'ss', while
   * the micro symbol 'µ' will be changed to the Greek mu 'μ'.
   *
   * Two equivalent domains have the same normalised form. And the
   * normalised form of a normalised domain is itself (i.e., if 
   * d is normalised, the following is true: d == normalizeDomain(d) )
   *
   * This operation is equivalent to encoding and the decoding a Unicode
   * hostname.
   *
   * @param domain		a domain to be normalised
   * @return the normalised domain, or QString::null if the domain is
   * invalid.
   */
  static QString normalizeDomain(const QString& domain);

  /**
   * Resolves a protocol number to its names
   *
   * Note: the returned QStrList operates on deep-copies.
   *
   * @param protonum	the protocol number to be looked for
   * @return all the protocol names in a list. The first is the "proper"
   *		name.
   */
  static QStrList protocolName(int protonum);

  /**
   * Finds all aliases for a given protocol name
   *
   * @param protoname	the protocol name to be looked for
   * @return all the protocol names in a list. The first is the "proper"
   *		name.
   */
  static QStrList protocolName(const char *protoname);

  /**
   * Resolves a protocol name to its number
   *
   * @param protoname	the protocol name to be looked for
   * @return the protocol number or -1 if we couldn't locate it
   */
  static int protocolNumber(const char *protoname);

  /**
   * Resolves a service name to its port number
   *
   * @param servname		the service name to be looked for
   * @param protoname		the protocol it is associated with
   * @return the port number in host byte-order or -1 in case of error
   */
  static int servicePort(const char *servname, const char *protoname);

  /**
   * Finds all the aliases for a given service name
   *
   * Note: the returned QStrList operates on deep-copies.
   *
   * @param servname		the service alias to be looked for
   * @param protoname		the protocol it is associated with
   * @return all the service names in a list. The first is the "proper"
   *		name.
   */
  static QStrList serviceName(const char *servname, const char *protoname);

  /**
   * Resolves a port number to its names
   *
   * Note: the returned QStrList operates on deep copies.
   *
   * @param port		the port number, in host byte-order
   * @param protoname		the protocol it is associated with
   * @return all the service names in a list. The first is the "proper"
   *		name.
   */
  static QStrList serviceName(int port, const char *protoname);

protected:

  /**
   * Sets the error codes
   */
  void setError(int errorcode, int systemerror = 0);

  virtual void virtual_hook( int id, void* data );
private:
  KResolverPrivate* d;
  friend class KResolverResults;
  friend class ::KNetwork::Internal::KResolverManager;
};

}				// namespace KNetwork

#endif
