/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverlistitem.cpp  -  Holds the list items inside the server list preferences panel
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kdebug.h>

#include "serverlistitem.h"

ServerListItem::ServerListItem(QListViewItem* parent,
                               int newId,
                               QString arg0,
                               QString arg1,
                               QString arg2,
                               QString arg3,
                               QString arg4,
                               QString arg5,
                               QString arg6) :
                QCheckListItem(parent,QString::null,QCheckListItem::CheckBox)
{
  id=newId;
  setText(1,arg1);
  setText(2,arg2);
  setText(3,arg3);
  setText(4,arg4);
  setText(5,arg5);
  setText(6,arg6);

  group=arg0;
}

ServerListItem::~ServerListItem()
{
}

void ServerListItem::stateChange(bool state)
{
  emit stateChanged(this,state);
}

int ServerListItem::getId() const        { return id; };
QString ServerListItem::getGroup() const { return group; };

#include "serverlistitem.moc"
