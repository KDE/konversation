/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverentry.cpp  -  description
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qstringlist.h>

#include <kdebug.h>

#include "serverentry.h"

int ServerEntry::newId=0;

ServerEntry::ServerEntry(const QString &newDefinition)
{
  // Every entry gets a unique ID
  id=newId++;
  definition=newDefinition;
  QStringList properties=QStringList::split(',',getDefinition(),true);
  while(properties.count()<9)
  {
    definition=definition+",";
    properties.append(QString::null);
  }
}

ServerEntry::~ServerEntry()
{
}

void ServerEntry::updateProperty(int property,const QString& value)
{
  QStringList properties=QStringList::split(',',getDefinition(),true);
  properties[property]=value;
  setDefinition(properties.join(","));
}

QString ServerEntry::getServerName() const
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  return definition[1];
}

int ServerEntry::getPort() const
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  return definition[2].toInt();
}

QString ServerEntry::getChannelName() const
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  return definition[4];
}

QString ServerEntry::getChannelKey() const
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  return definition[5];
}

bool ServerEntry::getAutoConnect() const
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  return (definition[6]=="1") ? true : false;
}

QString ServerEntry::getIdentity() const
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  return definition[7];
}

QString ServerEntry::getDefinition() const { return definition; }
void ServerEntry::setDefinition(const QString& newDefinition) { definition=newDefinition; }
int ServerEntry::getId() const{ return id; }

void ServerEntry::setIdentity(const QString& newIdentity)
{
  QStringList definition(QStringList::split(',',getDefinition(),true));
  definition[7]=newIdentity;
  setDefinition(definition.join(","));
}
