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

#include "config.h"

// System includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

// Qt
#include <qevent.h>
#include <qmutex.h>
#include <qapplication.h>

// Us
#include "kreverseresolver.h"
#include "kresolver_p.h"
#include "kresolverworkerbase.h"
#include "ksocketaddress.h"

using namespace KNetwork;
using namespace KNetwork::Internal;

namespace
{
  class ReverseThread: public KResolverWorkerBase
  {
  public:
    ReverseThread(const KSocketAddress& addr, int flags)
      : m_addr(addr), m_flags(flags), m_parent(0L)
    { }

    virtual ~ReverseThread()
    { }

    virtual bool preprocess()
    { return true; }
    virtual bool run();
    virtual bool postprocess();

    // input:
    KSocketAddress m_addr;
    int m_flags;
    KReverseResolver *m_parent;

    // output:
    QString node;
    QString service;
    bool success;
  };

  class KReverseResolverEvent: public QEvent
  {
  public:
    static const int myType = QEvent::User + 63; // arbitrary value
    QString node;
    QString service;
    bool success;

    KReverseResolverEvent(const QString& _node, const QString& _service,
			  bool _success)
      : QEvent((Type)myType), node(_node), 
	service(_service), success(_success)
    { }
  };
}

class KNetwork::KReverseResolverPrivate
{
public:
  QString node;
  QString service;
  KSocketAddress addr;
  int flags;

  ReverseThread* worker;
  bool success;

  inline KReverseResolverPrivate(const KSocketAddress& _addr)
    : addr(_addr), worker(0L), success(false)
  { }
};

KReverseResolver::KReverseResolver(const KSocketAddress& addr, int flags,
				   QObject *parent, const char* name)
  : QObject(parent, name), d(new KReverseResolverPrivate(addr))
{
  d->flags = flags;
}

KReverseResolver::~KReverseResolver()
{
  if (d->worker)
    d->worker->m_parent = 0L;
}

bool KReverseResolver::isRunning() const
{
  return d->worker != 0L;
}

bool KReverseResolver::success() const
{
  return !isRunning() && d->success;
}

bool KReverseResolver::failure() const
{
  return !isRunning() && !d->success;
}

QString KReverseResolver::node() const
{
  return d->node;
}

QString KReverseResolver::service() const
{
  return d->service;
}

const KSocketAddress& KReverseResolver::address() const
{
  return d->addr;
}

bool KReverseResolver::start()
{
  if (d->worker != 0L)
    return true;		// already started

  d->worker = new ReverseThread(d->addr, d->flags);
  d->worker->m_parent = this;

  RequestData *req = new RequestData;
  req->obj = 0L;
  req->input = 0L;
  req->requestor = 0L;
  req->worker = d->worker;
  KResolverManager::manager()->dispatch(req);
  return true;
}

bool KReverseResolver::event(QEvent *e)
{
  if (e->type() != KReverseResolverEvent::myType)
    return QObject::event(e);	// call parent

  KReverseResolverEvent *re = static_cast<KReverseResolverEvent*>(e);
  d->node = re->node;
  d->service = re->service;
  d->success = re->success;

  // don't delete d->worker!
  // KResolverManager::doNotifying takes care of that, if it hasn't already
  d->worker = 0L;

  // emit signal
  emit finished(*this); 

  return true;
}

bool KReverseResolver::resolve(const KSocketAddress& addr, QString& node,
			       QString& serv, int flags)
{
  ReverseThread th(addr, flags);
  if (th.run())
    {
      node = th.node;
      serv = th.service;
      return true;
    }
  return false;
}

bool KReverseResolver::resolve(const struct sockaddr* sa, Q_UINT16 salen,
			       QString& node, QString& serv, int flags)
{
  return resolve(KSocketAddress(sa, salen), node, serv, flags);
}

bool ReverseThread::run()
{
  int err;
  char h[NI_MAXHOST], s[NI_MAXSERV];
  int niflags = 0;

  h[0] = s[0] = '\0';

  if (m_flags & KReverseResolver::NumericHost)
    niflags |= NI_NUMERICHOST;
  if (m_flags & KReverseResolver::NumericService)
    niflags |= NI_NUMERICSERV;
  if (m_flags & KReverseResolver::NodeNameOnly)
    niflags |= NI_NOFQDN;
  if (m_flags & KReverseResolver::Datagram)
    niflags |= NI_DGRAM;
  if (m_flags & KReverseResolver::ResolutionRequired)
    niflags |= NI_NAMEREQD;

  {
#ifdef NEED_MUTEX
    QMutexLocker locker(&getXXbyYYmutex);
#endif
    err = ::getnameinfo(m_addr, m_addr.length(),
			h, sizeof(h) - 1, s, sizeof(s) - 1, niflags);
  }

  if (err == 0)
    {
      node = KResolver::domainToUnicode(QString::fromLatin1(h));
      service = QString::fromLatin1(s);
      success = true;
    }
  else
    {
      node = service = QString::null;
      success = false;
    }

  return success;
}

bool ReverseThread::postprocess()
{
  // post an event
  if (m_parent)
    QApplication::postEvent(m_parent,
			    new KReverseResolverEvent(node, service, success));
  return true;
}

#include "kreverseresolver.moc"
