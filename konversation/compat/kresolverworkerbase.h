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

#ifndef KRESOLVERWORKERBASE_H
#define KRESOLVERWORKERBASE_H

#include "kresolver.h"

// forward declarations
class QString;
template <class T> class QValueList;

namespace KNetwork {

  namespace Internal
  {
    class KResolverThread;
  }

/** @internal
 * This class is the base functionality for a resolver worker. That is,
 * the class that does the actual work.
 *
 * In the future, this class might be exposed to allow plug-ins. So, try and 
 * make it binary compatible.
 *
 * Note that hostnames are still encoded in Unicode at this point. It's up to
 * the worker class to decide which encoding to use. In the case of DNS, 
 * an ASCII Compatible Encoding (ACE) must be used. 
 * See @ref KResolver::domainToAscii. 
 *
 * Also specially note that the run method in this class is called in a
 * thread that is not the program's main thread. So do not do anything there
 * that you shouldn't!
 *
 * @class KResolverWorkerBase kresolverworkerbase.h kresolverworkerbase.h
 */
class KResolverWorkerBase
{
private:
  // this will be like our d pointer
  KNetwork::Internal::KResolverThread *th;
  const KResolverPrivate::InputData *input;
  friend class KNetwork::Internal::KResolverThread;
  friend class KNetwork::Internal::KResolverManager;

  int m_finished : 1;
  int m__reserved : 31;		// reserved

public:
  /**
   * Derived classes will put their resolved data in this list, or will
   * leave it empty in case of error.
   *
   * Status and error codes should also be stored in this object (the
   * @ref setError function does that).
   */
  KResolverResults results;

public:
  // default constructor
  KResolverWorkerBase();

  // virtual destructor
  virtual ~KResolverWorkerBase();

  /**
   * This is the hostname to be looked for
   */
  QString nodeName() const;

  /**
   * And this is the service name
   */
  QString serviceName() const;

  /**
   * gets the flags
   */
  int flags() const;

  /**
   * gets the family mask
   */
  int familyMask() const;

  /**
   * gets the socket type
   */
  int socketType() const;

  /**
   * gets the protocol number
   */
  int protocol() const;

  /**
   * gets the protocol name, if applicable
   */
  QCString protocolName() const;

  /**
   * Call this function to indicate that processing
   * has finished. This is useful in the preprocessing
   * stage, to indicate that @ref run doesn't have to be
   * called.
   */
  void finished();

protected:
  // like a QThread
  /**
   * This is the function that should be overriden in derived classes.
   * 
   * Derived classes will do their blocking job in this function and return
   * either success or failure to work (not the lookup). That is, whether the
   * lookup result was a domain found or not, if we got our answer, we should
   * indicate success. The error itself must be set with @ref setError.
   *
   * \b Important: this function gets called in a separate thread!
   * 
   * @return true on success
   */
  virtual bool run() = 0;

  /**
   * This function gets called during pre processing for this class and you must
   * override it.
   *
   * \b Important: this function gets called in the main event thread. And it MUST
   * NOT block.
   *
   * This function can be used for an object to determine if it will be able
   * to resolve the given data or not even before launching into a blocking
   * operation. This function should return true if the object is capable of
   * handling this kind of data; false otherwise. Note that the return value
   * of 'true' means that the object's blocking answer will be considered authoritative.
   *
   * This function MUST NOT queue further requests. Leave that to @ref run.
   *
   * This function is pure virtual; you must override it.
   *
   * @return true on success
   */
  virtual bool preprocess() = 0;

  /**
   * This function gets called during post processing for this class.
   *
   * \b Important: this function gets called in the main event thread. And it MUST
   * NOT block.
   *
   * @returns true on success
   */
  virtual bool postprocess();

  /**
   * Sets the error
   */
  inline void setError(int errorcode, int syserror = 0)
  { results.setError(errorcode, syserror); }

  /**
   * Enqueue the given resolver for post-processing.
   *
   * Use this function to make the manager call for another resolution.
   * This is suitable for workers that do post-processing.
   *
   * The manager will make sure that any requests enqueued by this function
   * are done before calling the postprocessing function, which you should 
   * override.
   *
   * \b Important: do use KResolver's own enqueueing functions (i.e., @ref KResolver::start). 
   * Instead, use this function.
   *
   * @returns true on successful queueing or false if a problem ocurred
   */
  bool enqueue(KResolver* other);

  /**
   * @overload
   */
  bool enqueue(KResolverWorkerBase* worker);
};

/** @internal
 * This class provides functionality for creating and registering worker classes.
 *
 * @class KResolverWorkerFactoryBase kresolverworkerbase.h kresolverworkerbase.h
 */
class KResolverWorkerFactoryBase
{
public:
  virtual KResolverWorkerBase* create() const = 0;

  /*
   * Wrapper call to register workers
   *
   * It is NOT thread-safe!
   */
  static void registerNewWorker(KResolverWorkerFactoryBase* factory);
};

/** @internal
 * This class provides functionality for creating and registering worker classes.
 *
 * @class KResolverWorkerFactory kresolverworkerbase.h kresolverworkerbase.h
 */
template<class Worker>
class KResolverWorkerFactory: public KResolverWorkerFactoryBase
{
public:
  virtual KResolverWorkerBase* create() const
  { return new Worker; }
};

}				// namespace KNetwork

#endif
