// -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

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

#include "connection.h"

Connection::Connection(const QString& server,
		       const QString& port,
		       const QString& password
                       )
{
    // FIXME SSL Support

    // Initialize our variables
    m_server = server;
    m_port = port;
    m_password = password;
    m_fatalError = false;
    m_lastError = QString::null;
    m_socket = new KBufferedSocket(m_server,m_port,this,"server_socket");

    // Catch the signals
    QObject::connect(m_socket,SIGNAL(gotError(int)),this,SLOT(error(int)));
    QObject::connect(m_socket,SIGNAL(connected(const KResolverEntry&,bool&)),
                     this,SLOT(connected(const KResolverEntry&,bool&)));
    QObject::connect(m_socket,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
    QObject::connect(m_socket,SIGNAL(readyWrite()),this,SIGNAL(readyWrite()));

    // Ready to fire
    connect();
}

Connection::~Connection()
{
    if(m_fatalError)
        delete m_socket;
    else // Not fatal so KNetwork will clean this mess for us
        m_socket->deleteLater();
}

void Connection::connect()
{
    // FIXME Bind to spesific interface (eth0,ppp0,...)
    m_socket->connect(m_server,m_port);
}

void Connection::connected(const KResolverEntry& remote, bool& /*skip*/)
{
    m_serverIp = remote.address().toString();
}

void Connection::disconnect()
{
    m_socket->close();
}

void Connection::error(int error)
{
  if( m_socket->isFatalError(error) )
      m_fatalError = true;
  else
      m_fatalError = false;

  m_lastError = m_socket->errorString();
  emit socketError( m_lastError );
}

void Connection::readData(QByteArray& buffer)
{
    // FIXME Do we need different thing for SSL?
    int max_bytes = m_socket->bytesAvailable();
    m_socket->readBlock( buffer.data(), max_bytes );
}

void Connection::writeData( const QCString& buffer )
{
    m_socket->writeBlock( buffer, buffer.length() );
}


#include "connection.moc"
