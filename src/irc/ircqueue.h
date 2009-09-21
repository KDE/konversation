/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.
*/

/*
  Copyright (C) 2008 Eli J. MacKenzie <argonel at gmail.com>
*/


#ifndef IRCQUEUE_H
#define IRCQUEUE_H

#include <QObject>
#include <QList>
#include <QTime>

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
    IRCMessage() : t(QTime::currentTime()) //, codec(QTextCodec::codecForName("utf8"))
    {} ///< this constructor required for QValueList, do not use

    /**
        Make a new IRCMessage with timestamp of QTime::currentTime().

        Note the constructor takes a QString, not a const QString& or a QString *. If you want to modify the
        contained text, put it back with setText.
    */
    IRCMessage(QString i) : s(i), t(QTime::currentTime()) //, codec(QTextCodec::codecForName("utf8"))
    {}

    QString text() { return s; }
    int age() { return t.elapsed(); }
    QTime time() { return t; }
    void setText(QString text) { s=text; }
private:
    QString s;
    QTime t;

    //FIXME wire this up
    //QTextCodec* codec;
    //operator const char * () const { return codec->fromUnicode(text()); }

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
        bool isValid()  { return m_rate > 0; }
        bool operator==(const EmptyingRate& o) const
        {
            return (m_rate == o.m_rate && m_interval == o.m_interval && m_type == o.m_type)? true : false;
        }
    };

    IRCQueue(Server *server, EmptyingRate& rate, int myindex=0);
    ~IRCQueue();

    void enqueue(QString line);
    void reset();
    EmptyingRate& getRate();// { return &m_rate; }

    bool isValid() { return m_rate.isValid(); }
    bool isEmpty() { return m_pending.isEmpty(); }

    //WTF? why are there two of these.
    //These are decoupled for a reason, what is it?
    int currentWait(); ///< Time in ms that the front has been waiting
    int elapsed(); ///< How long has the queue been running since it was last started?

    int nextSize(); ///< Current size of front
    int pendingMessages() { return m_pending.count(); }
    int linesSent() const; ///< count of lines sent by this queue
    int bytesSent() const; ///< count of bytes sent by this queue

    ///Time in milliseconds that the previous message waited
    int lastWait() { return m_lastWait; }

public slots:
    void sent(int bytes, int encodedBytes, IRCQueue *); ///< feedback statistics
    void sendNow(); ///< dumps a line to the socket
    void serverOnline(bool on); ///< server tells us that the socket is ready

protected:
    QString pop(); ///< pops front, sets statistics
    void adjustTimer(); ///< sets the next timer interval
    bool doSend(); ///< pops front and tells the server to send it. returns true if we sent something
    EmptyingRate& m_rate;

private:
    QList<IRCMessage> m_pending;
    QTimer *m_timer;
    bool m_blocked;
    bool m_online;
    Server *m_server;

    QTime m_startedAt;
    QTime m_lastSent, m_globalLastSent;
    int m_linesSent, m_globalLinesSent;
    int m_bytesSent, m_globalBytesSent;
    int m_lastWait;
    int m_myIndex;
};

#endif
