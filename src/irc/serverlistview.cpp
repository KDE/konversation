/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ServerListView is derived from K3ListView and implements custom
  drag'n'drop behavior needed in ServerListDialog.

  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include "serverlistview.h"

#include <Q3StoredDrag>

#include <kdebug.h>


ServerListView::ServerListView(QWidget *parent)
    : K3ListView(parent)
{
}

ServerListView::~ServerListView()
{
}

Q3PtrList<Q3ListViewItem> ServerListView::selectedServerListItems()
{

    QList<Q3ListViewItem*> selectedItems = K3ListView::selectedItems();
    Q3PtrList<Q3ListViewItem> selectedServerListItems;

    Q3ListViewItem* item;// = selectedItems.first();
    foreach (item, selectedItems)
    //while (item)
    {
        if (item->parent())
        {
            if (!item->parent()->isSelected())
                selectedServerListItems.append(item);
        }
        else
        {
            selectedServerListItems.append(item);
        }

        //item = selectedItems.next();
    }

    return selectedServerListItems;
}

void ServerListView::findDrop(const QPoint &pos, Q3ListViewItem *&parent, Q3ListViewItem *&after)
{
    QPoint p (contentsToViewport(pos));

    // Get the position to put it in
    Q3ListViewItem *atpos = itemAt(p);

    Q3ListViewItem *above;
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

Q3DragObject* ServerListView::dragObject()
{
    if (!currentItem())
        return 0;

    QList<Q3ListViewItem*> selected = selectedItems();
    Q3ListViewItem* item;// = selected.first();

    foreach (item, selected)
    //while (item)
    {
        if (!item->dragEnabled())
            return 0;

        //item = selected.next();
    }

    return new Q3StoredDrag("application/x-qlistviewitem", viewport());
}

#include "serverlistview.moc"
