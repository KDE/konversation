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
    ServerEntry(const QString &newDefinition);
    ~ServerEntry();
    QString getGroupName() const;
    QString getServerName() const;
    int getPort() const;
    QString getChannelName() const;
    QString getChannelKey() const;
    bool getAutoConnect() const;
    QString getIdentity() const;
    QString getDefinition() const;

    void setDefinition(const QString& newDefinition);
    void setIdentity(const QString& newIdentity);
    int getId() const;
    void updateProperty(int property,const QString& value);

  protected:
    QString definition;

    int id;
    static int newId;
};

#endif
