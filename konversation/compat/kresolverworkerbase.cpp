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

#include <qcstring.h>
#include <qstring.h>

#include "kresolver.h"
#include "kresolver_p.h"
#include "kresolverworkerbase.h"

using namespace KNetwork;
using namespace KNetwork::Internal;

KResolverWorkerBase::KResolverWorkerBase()
  : th(0L), input(0L), m_finished(0), m__reserved(0)
{
}

KResolverWorkerBase::~KResolverWorkerBase()
{
}

QString KResolverWorkerBase::nodeName() const
{
  if (input)
    return input->node;
  return QString::null;
}

QString KResolverWorkerBase::serviceName() const
{
  if (input)
    return input->service;
  return QString::null;
}

int KResolverWorkerBase::flags() const
{
  if (input)
    return input->flags;
  return 0;
}

int KResolverWorkerBase::familyMask() const
{
  if (input)
    return input->familyMask;
  return 0;
}

int KResolverWorkerBase::socketType() const
{
  if (input)
    return input->socktype;
  return 0;
}

int KResolverWorkerBase::protocol() const
{
  if (input)
    return input->protocol;
  return 0;
}

QCString KResolverWorkerBase::protocolName() const
{
  QCString res;
  if (input)
    res = input->protocolName;
  return res;
}

void KResolverWorkerBase::finished()
{
  m_finished = true;
}

bool KResolverWorkerBase::postprocess()
{
  return true;			// no post-processing is a always successful postprocessing
}

bool KResolverWorkerBase::enqueue(KResolverWorkerBase* worker)
{
  RequestData *myself = th->data;
  RequestData *newrequest = new RequestData;
  newrequest->obj = 0;
  newrequest->input = input; // same input
  newrequest->requestor = myself;
  newrequest->nRequests = 0;
  newrequest->worker = worker;
  myself->nRequests++;
  KResolverManager::manager()->dispatch(newrequest);
  return true;
}

void KResolverWorkerFactoryBase::registerNewWorker(KResolverWorkerFactoryBase* factory)
{
  KResolverManager::manager()->registerNewWorker(factory);
}
