/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverentry.h  -  description
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef SERVERENTRY_H
#define SERVERENTRY_H

#include <qstring.h>

/*
  @author Dario Abatianni
*/

class ServerEntry
{
  public:
    ServerEntry(const QString newDefinition);
    ~ServerEntry();
    QString getServerName();
    int getPort();
    QString getChannelName();
    QString getChannelKey();
    bool getAutoConnect();
    QString getIdentity();
    QString getDefinition();

    void setDefinition(const QString& newDefinition);
    int getId();
    void updateProperty(int property,const QString& value);

  protected:
    QString definition;

    int id;
    static int newId;
};

#endif
