/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverlistitem.h  -  description
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef SERVERLISTITEM_H
#define SERVERLISTITEM_H

#include <qlistview.h>

/*
  @author Dario Abatianni
*/

class ServerListItem : public QObject, public QCheckListItem
{
  Q_OBJECT

  public:
    ServerListItem(QListView* parent,int newId,QString arg0,
                   QString arg1=QString::null,
                   QString arg2=QString::null,
                   QString arg3=QString::null,
                   QString arg4=QString::null,
                   QString arg5=QString::null,
                   QString arg6=QString::null);
    ~ServerListItem();
    int getId() { return id; };

  signals:
    void stateChanged(ServerListItem* myself,bool state);

  protected:
    void stateChange(bool state);
    int id;
};

#endif
