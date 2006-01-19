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

#include <qdragobject.h>
#include <kdebug.h>

#include "serverlistview.h"

/*
  @author Eike Hein
*/

ServerListView::ServerListView(QWidget *parent)
    : KListView(parent)
{
}

ServerListView::~ServerListView()
{
}

void ServerListView::findDrop(const QPoint &pos, QListViewItem *&parent, QListViewItem *&after)
{
    QPoint p (contentsToViewport(pos));

    // Get the position to put it in
    QListViewItem *atpos = itemAt(p);

    QListViewItem *above;
    if (!atpos) // put it at the end
        above = lastItem();
    else
    {
        // Get the closest item before us ('atpos' or the one above, if any)
        if (p.y() - itemRect(atpos).topLeft().y() < (atpos->height()/2))
            above = atpos->itemAbove();
        else
            above = atpos;
    }

    if (above)
    {
        if (above->firstChild())
        {
            after = above;
            parent = after->parent();
            return;
        }
        else
        {
            after = above->parent();
            parent = after ? after->parent() : 0L;
            return;
        }
    }
    // set as sibling
    after = above;
    parent = after ? after->parent() : 0L;
}

QDragObject* ServerListView::dragObject()
{
    if (!currentItem())
        return 0;

    QPtrList<QListViewItem> selected = selectedItems();
    QListViewItem* item = selected.first();

    while (item)
    {
        if (!item->dragEnabled())
            return 0;

        item = selected.next();
    }

    return new QStoredDrag("application/x-qlistviewitem", viewport());
}

#include "serverlistview.moc"
