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

class Connection
{
  Q_OBJECT;
  
 public:
  
  Connection(const QString& server, int port, const QString& password, const QString& interface);
  ~Connection();

  void connect();
  void reconnect();
  void disconnect();

 signals:
  
  void disconnected();
  void error(const QString& error);

 private:

  QString m_server;
  int m_port;
  QString m_password;
  QString m_interface;
  bool m_fatalError;
  
  KBufferedSocket* m_socket;
  QString m_serverIp;

};

#endif // CONNECTION_H
