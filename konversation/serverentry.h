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

#include <qstring.h>

#ifndef SERVERENTRY_H
#define SERVERENTRY_H

/*
  @author Dario Abatianni
*/

class ServerEntry
{
  public:
    ServerEntry(const QString newDefinition);
    ~ServerEntry();
    QString getDefinition() { return definition; };
    void setDefinition(const QString& newDefinition) { definition=newDefinition; };
    int getId() { return id; };
    void updateProperty(int property,const QString& value);

  protected:
    QString definition;

    int id;
    static int newId;
};

#endif
