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
//Added by qt3to4:
#include <Q3PtrList>


class Q3DragObject;

class ServerListView : public KListView
{
    Q_OBJECT

    public:
        explicit ServerListView(QWidget *parent);
        ~ServerListView();

        Q3PtrList<Q3ListViewItem> selectedServerListItems();

    protected:
        void findDrop(const QPoint &pos, Q3ListViewItem *&parent, Q3ListViewItem *&after);
        Q3DragObject* dragObject();
};

#endif
