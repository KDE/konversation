/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#include "serverlistview.h"


ServerListView::ServerListView(QWidget *parent)
: QTreeWidget(parent)
{
}

ServerListView::~ServerListView()
{
}
void ServerListView::dragMoveEvent(QDragMoveEvent *e)
{
    QTreeWidgetItem* item = this->itemAt(e->position().toPoint());
    QTreeWidgetItem* sItem = this->currentItem();
    bool bad;
    //  bad selection || dropping on viewport must be a toplevelitem || children of the same parent (or lack thereof)
    if (badDropSelection() || (!item && indexOfTopLevelItem(sItem)<0) || (item && item->parent() != sItem->parent()))
    {
        bad=true;
        setDropIndicatorShown(false);
    }
    else
    {
        bad=false;
        setDropIndicatorShown(true);
    }
    QTreeView::dragMoveEvent(e);

    if(bad)
        e->ignore();

}
void ServerListView::dragLeaveEvent(QDragLeaveEvent *e)
{
    QAbstractItemView::dragLeaveEvent(e);
    Q_EMIT moved();
}

void ServerListView::dragEnterEvent(QDragEnterEvent *e)
{
    Q_EMIT aboutToMove();
    QAbstractItemView::dragEnterEvent(e);
}

bool ServerListView::badDropSelection()
{
    const QList<QTreeWidgetItem*> items = selectedItems();
    QList<QTreeWidgetItem*> children;
    QTreeWidgetItem* parent;
    int t=0; //top item count
    for (QTreeWidgetItem* item : items) {
        if (indexOfTopLevelItem(item)<0) // is a child
        {
                children.append(item);
                parent = children.at(0)->parent();
                // make sure all the children have the same parent
                for (QTreeWidgetItem* child : std::as_const(children)) {
                    if (child->parent() != parent)
                        return true;
                }
        }
        else    t++;

        if (t > 0 && !children.isEmpty()) //make sure we don't have a top and a child selected
            return true;
    }
    return false;
}

void ServerListView::dropEvent(QDropEvent *event)
{
    if(badDropSelection())
    {
        event->ignore();
        return;
    }
    QTreeWidgetItem* sourceItem = currentItem();
    QTreeWidget::dropEvent(event);
    //clean up after dropEvent, flatten the list
    QList<QTreeWidgetItem*> children;
    int childIndex;
    QTreeWidgetItem* sItem;
    QTreeWidgetItem* item;
    //TODO figure out why the iterator fails to do this without crashing
    for(int i=0; i<topLevelItemCount(); i++)
    {
        for(int j=0; j<topLevelItem(i)->childCount(); j++)
        {
            if(topLevelItem(i)->child(j)->childCount() > 0)
            {
                sItem = topLevelItem(i)->child(j);
                if(sItem->text(1).isEmpty()) // if the second column has no text it's a server
                {
                    childIndex = j;
                    children = topLevelItem(i)->child(j)->takeChildren();
                    if (!children.isEmpty())
                        topLevelItem(i)->insertChildren(childIndex++, children);

                }
                else
                {
                    childIndex = i;
                    item = topLevelItem(i)->takeChild(j);
                    insertTopLevelItem(childIndex++, item);
                }
            }
        }
    }
    setCurrentItem(sourceItem);
    Q_EMIT moved();
}

#include "moc_serverlistview.cpp"
