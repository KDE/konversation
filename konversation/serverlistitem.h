/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverlistitem.h  -  Holds the list items inside the server list preferences panel
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

*/


#ifndef SERVERLISTITEM_H
#define SERVERLISTITEM_H

#include <qlistview.h>
#include <qstring.h>

/*
  @author Dario Abatianni
*/

class ServerListItem : public QObject, public QCheckListItem
{
  Q_OBJECT

  public:
    ServerListItem(QListViewItem* parent,
                   int newId,
                   QString arg0,
                   QString arg1=QString::null,
                   QString arg2=QString::null,
                   QString arg3=QString::null,
                   QString arg4=QString::null,
                   QString arg5=QString::null,
                   QString arg6=QString::null,
                   QString arg7=QString::null);
    ~ServerListItem();

    int getId() const;
    QString getGroup() const;

  signals:
    void stateChanged(ServerListItem* myself,bool state);

  protected:
    void stateChange(bool state);
    int id;
    QString group;
};

#endif
