// -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#ifndef CONNECTION_H
#define CONNECTION_H
/*
    Copyright (c) 2005      by İsmail Dönmez <ismail@kde.org.tr>
    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kbufferedsocket.h>
#include <kresolver.h>
using namespace KNetwork;

class Connection : public QObject
{
    Q_OBJECT;

public:

    Connection(const QString& server,
               const QString& port,
               const QString& password,
               const QString& interface);
    ~Connection();

    void connect();
    void reconnect();
    void disconnect();
    void readData(QByteArray& buffer);
    void writeData(const QCString& buffer);

protected slots:

    void connected(const KResolverEntry &remote,bool& /*skip*/);
    void error(int error);

signals:

    void disconnected();
    void socketError( const QString& );

private:

    QString m_server;
    QString m_port;
    QString m_password;
    QString m_interface;
    QString m_lastError;
    bool m_fatalError;

    KBufferedSocket* m_socket;
    QString m_serverIp;
};

#endif // CONNECTION_H
