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
		       QString port, 
		       const QString& password, 
		       const QString& interface)
{

  // Initialize our variables
  m_server = server;
  m_port = port;
  m_password = password;
  m_interface = interface;
  m_fatalError = false;
  m_socket = new KBufferedSocket(this,"server_socket");
  
  // Catch the signals
  connect(m_socket,SIGNAL(gotError(int)),this,SLOT(error()));
  connect(m_socket,SIGNAL(connected(const KResolverEntry&)),this,SLOT(connected(const KResolverEntry&)));
  
  // Ready to fire
  connect();
}

Connection::~Connection()
{
  m_socket->deleteLater();
}

void Connection::connect()
{
  m_socket->connect(m_servername,m_port);
}
 
void Connection::connected(const KResolverEntry& remote)
{
  m_serverIp = remote->address()->toString();
}

void Connection::disconnect()
{
  m_server->close();
}

#include "connection.moc"
