/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ServerListView is derived from KListView and implements custom
  drag'n'drop behavior needed in ServerListDialog.
  begin:     Wed Jan 18 2006
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#ifndef SERVERLISTVIEW_H
#define SERVERLISTVIEW_H

#include <klistview.h>

/*
  @author Eike Hein
*/

class QDragObject;

class ServerListView : public KListView
{
    Q_OBJECT

        public:
        ServerListView(QWidget *parent);
        ~ServerListView();

        protected:
        void findDrop(const QPoint &pos, QListViewItem *&parent, QListViewItem *&after);
        QDragObject* dragObject();
};

#endif
