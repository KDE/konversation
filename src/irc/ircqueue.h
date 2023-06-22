/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2008 Eli J. MacKenzie <argonel at gmail.com>
*/


#ifndef IRCQUEUE_H
#define IRCQUEUE_H

#include <QObject>
#include <QList>
#include <QTime>
#include <QElapsedTimer>

class QTimer;
class Server;

//channel.cpp, outputfilter.cpp, query.cpp, server.cpp, statuspanel.cpp

/**
 * A message from or to an IRC server.
 *
 * Contains all we know about the message, which currently consists of the text, the time it was created,
 * and its original encoding. (Since currently these objects are only used internally, we know the message
 * is Unicode.)
 */
struct IRCMessage
{
    /**
        Make a new IRCMessage with timestamp of QTime::currentTime().

        Note the constructor takes a QString, not a const QString& or a QString *. If you want to modify the
        contained text, put it back with setText.
    */
    IRCMessage(const QString &str);

    QString text() const { return s; }
    int age() const { return t.elapsed(); } // in milliseconds
    void setText(const QString &text) { s=text; }
private:
    QString s;
    QElapsedTimer t;

    //FIXME wire this up
    //QTextCodec* codec;
    //operator QByteArray () const { return codec->fromUnicode(text()); }

};

/**
* Provides a self-sending queue of IRCMessages.
*
* Messages enqueued in this server can only be erased via reset() or sent to the attached server.
* The server and the emptying rates cannot be changed, if you want to do that construct a new queue.

*/
class IRCQueue: public QObject
{
    Q_OBJECT

public:
    struct EmptyingRate
    {
        enum RateType {
            Lines, ///< Lines per interval.
            Bytes  ///< Bytes per interval. Not implemented. FIXME
        };
        EmptyingRate(int rate=39, int msec_interval=59000, RateType type=Lines):
                m_rate(rate), m_interval(msec_interval), m_type(type)
        {
        }

        int nextInterval(int byte_size, int msec_since_last);

        int m_rate;
        int m_interval;
        RateType m_type;
        bool isValid() const { return m_rate > 0; }
        bool operator==(const EmptyingRate& o) const
        {
            return (m_rate == o.m_rate && m_interval == o.m_interval && m_type == o.m_type)? true : false;
        }
    };

    IRCQueue(Server *server, EmptyingRate& rate);
    ~IRCQueue() override;

    void enqueue(const QString& line);
    void reset();
    EmptyingRate& getRate();// { return &m_rate; }

    bool isValid() const { return m_rate.isValid(); }
    bool isEmpty() const { return m_pending.isEmpty(); }

    //WTF? why are there two of these.
    //These are decoupled for a reason, what is it?
    int currentWait() const; ///< Time in ms that the front has been waiting
    int elapsed() const; ///< How long has the queue been running since it was last started?

    int nextSize() const; ///< Current size of front
    int pendingMessages() const { return m_pending.count(); }
    int linesSent() const; ///< count of lines sent by this queue
    int bytesSent() const; ///< count of bytes sent by this queue

    ///Time in milliseconds that the previous message waited
    int lastWait() const { return m_lastWait; }

public Q_SLOTS:
    void sent(int bytes, int encodedBytes, IRCQueue *); ///< feedback statistics
    void sendNow(); ///< dumps a line to the socket
    void serverOnline(bool on); ///< server tells us that the socket is ready

private:
    QString pop(); ///< pops front, sets statistics
    void adjustTimer(); ///< sets the next timer interval
    bool doSend(); ///< pops front and tells the server to send it. returns true if we sent something

private:
    EmptyingRate& m_rate;

    QList<IRCMessage> m_pending;
    QTimer *m_timer;
    bool m_blocked;
    Server *m_server;

    QElapsedTimer m_startedAt;
    QTime m_lastSent, m_globalLastSent;
    int m_linesSent, m_globalLinesSent;
    int m_bytesSent, m_globalBytesSent;
    int m_lastWait;

    Q_DISABLE_COPY(IRCQueue)
};

#endif
