/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ServerListView is derived from KListView and implements custom
  drag'n'drop behavior needed in ServerListDialog.

  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#ifndef SERVERLISTVIEW_H
#define SERVERLISTVIEW_H

#include <klistview.h>


class QDragObject;

class ServerListView : public KListView
{
    Q_OBJECT

    public:
        explicit ServerListView(QWidget *parent);
        ~ServerListView();

        QPtrList<QListViewItem> selectedServerListItems();

    protected:
        void findDrop(const QPoint &pos, QListViewItem *&parent, QListViewItem *&after);
        QDragObject* dragObject();
};

#endif
