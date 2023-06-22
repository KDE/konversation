/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2008 Eli J. MacKenzie <argonel at gmail.com>
*/

#include "ircqueue.h"

#include "server.h"
#include "konversation_log.h"

#include <QTimer>
#include <QString>

IRCMessage::IRCMessage(const QString &str)
    : s(str) //, codec(QTextCodec::codecForName("utf8"))
{
    t.start();
}

////

int IRCQueue::EmptyingRate::nextInterval(int, int elapsed)
{
    if (!isValid())
        return 0;
    //KX << _S(m_interval) << endl;
    if (m_type == Lines)
    {
        int i = m_interval/m_rate;
        //KX << _S(i) << endl;
        if (i<elapsed) {
            //KX << _S(i) << _S(elapsed) << endl;
            return 0;
        }
        else
        {
            //KX << _S(i) << endl;
            return i;
        }
    }
    else
    {
        //TODO write this...
        return 0;
    }
return 0;
}

IRCQueue::EmptyingRate& IRCQueue::getRate()
{
    return m_rate;
}


IRCQueue::IRCQueue(Server *server, EmptyingRate& rate) :
        m_rate(rate), m_blocked(true), m_server(server),
        m_linesSent(0), m_globalLinesSent(0),
        m_bytesSent(0), m_globalBytesSent(0), m_lastWait(0)
{
    //KX << _S(m_rate.m_rate) << _S(m_rate.m_interval) << _S(m_rate.m_type) << endl;
    m_timer=new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &IRCQueue::sendNow);
    if (server)
    {
        connect(server, &Server::serverOnline, this, &IRCQueue::serverOnline);
        connect(server, &Server::sentStat, this, &IRCQueue::sent);
        m_blocked=!(m_server->isConnected());
    }
}

IRCQueue::~IRCQueue()
{
    qCDebug(KONVERSATION_LOG) << __FUNCTION__;
}

QString IRCQueue::pop()
{
    if (m_pending.isEmpty())
        return QString();

    IRCMessage msg=m_pending.first();
    m_pending.pop_front();
    m_lastWait=msg.age();
    m_lastSent=QTime::currentTime();
    return msg.text();
}

int IRCQueue::nextSize() const
{
    if (m_pending.isEmpty())
        return 0;
    return m_pending.first().text().length();
}

int IRCQueue::currentWait() const
{
    if (m_pending.isEmpty())
        return 0;
    return m_pending.first().age();
}

int IRCQueue::elapsed() const
{
    if (!m_startedAt.isValid())
        return 0;
    else
        return m_startedAt.elapsed(); //FIXME if its been more than a day since this queue was used, this breaks
}

int IRCQueue::linesSent() const
{
    return m_linesSent;
}

int IRCQueue::bytesSent() const
{
    return m_bytesSent;
}

///Feedback indicating size of data sent to update statistics. Not necessarily data from this queue!!!
void IRCQueue::sent(int, int e, IRCQueue *wq)
{
    //KX << Q_FUNC_INFO << _S(m_mine) << endl;
    m_globalLinesSent++;
    m_globalBytesSent+=e; // we don't care about the unencoded bytes, we want what went to the server
    if (wq == this) {
        m_linesSent++;
        m_bytesSent+=e;
    }
}

void IRCQueue::enqueue(const QString& line)
{
    m_pending.append(IRCMessage(line));
    if (!m_timer->isActive())
        adjustTimer();
}

//starts timer if stopped, adjusts interval if necessary
void IRCQueue::adjustTimer()
{
    int msec;
    msec=getRate().nextInterval(nextSize(), elapsed());
    //if (m_myIndex == 0)
    //    KX << _S(msec) << endl;
    m_timer->start(msec);
    m_startedAt.start();
    return;
}

bool IRCQueue::doSend()
{
    bool p=!m_pending.isEmpty();
    if (p)
    {
        QString s=pop();
        if (s.isEmpty())
            return doSend(); //can't send empty strings, but no point in losing the timeslot
        m_server->toServer(s, this);
        m_startedAt.start();
    }
    return p;//if we sent something, fire the timer again
}

///it would probably be better to delete and recreate the queue.
void IRCQueue::reset()
{
    // KX << Q_FUNC_INFO << endl;
    m_timer->stop();
    m_lastWait=0;
    if (m_server)
        m_blocked=!(m_server->isConnected()); //FIXME  (maybe) "we can't do this anymore because blocked can't correspond to whether the server is online, instead must correspond to whether the socket has become writable (readyWrite)"

    m_startedAt.invalidate();
    m_globalLastSent=m_lastSent=QTime();
    m_pending.clear();
    m_linesSent=m_bytesSent=m_globalBytesSent=m_globalLinesSent=0;
}

//called when the timer fires.
void  IRCQueue::sendNow()
{
    if (doSend())
        adjustTimer();
    //else //its a single-shot timer so if we don't adjust it, it won't run :)
}

///lets us know we should block output
void IRCQueue::serverOnline(bool on)
{
    if (m_blocked!=on)
        return;
    m_blocked=!on;
    if (m_blocked && m_timer->isActive())
        reset();
    else if (!m_blocked && !m_timer->isActive() && nextSize())
    {
        adjustTimer();
    }
}

#include "moc_ircqueue.cpp"
