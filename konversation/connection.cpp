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
		       const QString& password, 
		       const QString& interface)
{

  // Initialize our variables
  m_server = server;
  m_port = port;
  m_password = password;
  m_interface = interface;
  m_fatalError = false;
  m_lastError = QString::null;
  m_socket = new KBufferedSocket(m_server,m_port,this,"server_socket");
  
  // Catch the signals
  QObject::connect(m_socket,SIGNAL(gotError(int)),this,SLOT(error(int)));
  QObject::connect(m_socket,SIGNAL(connected(const KResolverEntry&,bool&)),
	  this,SLOT(connected(const KResolverEntry&,bool&)));
  QObject::connect(m_socket,SIGNAL(readyRead()),this,SLOT(readData()));
  
  // Ready to fire
  connect();
}

Connection::~Connection()
{
  if(m_fatalError)
    m_socket->deleteLater();
  else
    delete m_socket;
}

void Connection::connect()
{
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
}

void Connection::readData()
{
}
  
  
#include "connection.moc"
