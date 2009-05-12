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
#include <qpainter.h>
#include <kdebug.h>


ServerListView::ServerListView(QWidget *parent)
: QTreeWidget(parent)
{
}

ServerListView::~ServerListView()
{
}
QList<QTreeWidgetItem*> ServerListView::selectedServerListItems()
{
    QList<QTreeWidgetItem*> selectedItems = this->selectedItems();
    QList<QTreeWidgetItem*> selectedServerListItems;
    
    foreach (QTreeWidgetItem* item, selectedItems)
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
    }
    return selectedServerListItems;
}

void ServerListView::dragEnterEvent(QDragEnterEvent *e)
{
    QModelIndex index = indexAt(e->pos());
    if (index.isValid())
    {
        e->accept();
        setState(DraggingState);
    }
    else
    {
        e->ignore();
    }
}

void ServerListView::dragMoveEvent(QDragMoveEvent *e)
{
    
    QTreeWidgetItem* item = this->itemAt(e->pos());
    ServerListView* source = qobject_cast<ServerListView*>(e->source());
    
    QTreeWidgetItem* sItem= source->currentItem();
    e->ignore();         //by default because
    m_dropRect= QRect(); // we're cynics dur
    QModelIndex index = indexAt(e->pos());
    if (!this->droppingOnItself(e, index)) 
    {
        if (index.isValid())
        {
            QRect rect = visualItemRect(item);
            m_dropPosition = position(e->pos(), rect);
            int width = this->width();
            if (indexOfTopLevelItem(sItem) >= 0 && indexOfTopLevelItem(item) >= 0) //this means it is a top level item ie server group
            {
                switch (m_dropPosition)
                {
                    case 0:
                        m_dropRect = QRect(0, rect.top(), width, 0);
                        e->accept();  
                        break;
                    case 1:
                        if (!item->isExpanded()) //don't give the impression we're dropping parents on children <- lol
                        {
                            m_dropRect = QRect(0, rect.bottom(), width, 0);
                            e->accept();
                        }
                        break;
                        //we ignore case 2 because it gets sent a null qrect and that's default
                    case 3:
                        m_dropRect= QRect(0, rect.bottom(), width, 0);
                        e->accept();
                        break;
                }      
            }            
        }
        else //viewports...bleh
        {
            if(indexOfTopLevelItem(sItem) >= 0) // in case of children break glass
            {
                QTreeWidgetItem* lastTop = topLevelItem(topLevelItemCount()-1);
                QRect rect = QRect();
                if(!lastTop->isExpanded())
                rect = visualItemRect(lastTop); // dishes out the last top item
                else
                {
                    rect = visualItemRect(lastTop->child(lastTop->childCount()-1));
                }
                m_dropPosition= 3;
                int width = this->width();
                m_dropRect= QRect(0, rect.bottom(), width, 0);
                e->accept();
            }
        }
        
    }
    
    emit aboutToMove();
    viewport()->update();
}
void ServerListView::dragLeaveEvent(QDragLeaveEvent *)
{
    //stopAutoScroll();
    m_dropRect = QRect();
    setState(NoState);
    viewport()->update();
    //restart sorting
    emit moved();
}
//implemented so we can find the position, also means we can nix OnItem -YAY'
//For reference: AboveItem=0 BelowItem=1 (OnItem=2) OnViewport=3 
//(Onitem) is basically completely nixed in the rest of the code, keeping it here in case somebody likes it later
int ServerListView::position(const QPoint &pos, const QRect &rect)
{
    int r = 3;
    
    const int margin = 2;
    if (pos.y() - rect.top() < margin) {
        r = 0;
    } else if (rect.bottom() - pos.y() < margin) {
        r = 1;
    } else if (rect.contains(pos, true)) {
        r = 2;
    }
    
    //for some reason i think this gets rid of onitem on it's own which makes me mad
    if (r == 2) 
        r = pos.y() < rect.center().y() ? 0 : 1;
    
    return r; 
}
//I dont want to talk about it
void ServerListView::paintDropIndicator(QPainter *painter)
{
    if(m_dropRect.isNull())
        return;
    QStyleOption opt;
    opt.init(this);
    opt.rect = m_dropRect;
    this->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, painter, this);
}

void ServerListView::paintEvent(QPaintEvent *event)
{
    #ifndef QT_NO_CURSOR
    if (viewport()->cursor().shape() != Qt::ForbiddenCursor) {
    #endif
        QPainter painter(viewport());
        drawTree(&painter, event->region());
        this->paintDropIndicator(&painter);
    #ifndef QT_NO_CURSOR
    }
    #endif
}

void ServerListView::dropEvent(QDropEvent *event) 
{
    if (event->source() == this && (event->dropAction() == Qt::MoveAction ||
        dragDropMode() == QAbstractItemView::InternalMove)) 
    {
        QModelIndex topIndex;
        int col = -1;
        int row = -1;
        if (this->dropOn(event, &row, &col, &topIndex)) 
        {
            QList<QModelIndex> idxs = selectedIndexes();
            QList<QPersistentModelIndex> indexes;
            for (int i = idxs.count()-1; i >= 0; --i) 
                indexes.append(idxs.at(i));
            
            if (indexes.contains(topIndex))
                return;
            
            // When removing items the drop location could shift
            QPersistentModelIndex dropRow = model()->index(row, col, topIndex);
            
            // Remove the items
            QList<QTreeWidgetItem *> taken;
            for (int i = indexes.count() - 1; i >= 0; --i) //lol at unescessary complexity causing bugs
            {
                QTreeWidgetItem *parent = itemFromIndex(indexes.at(i));
                if (!parent || !parent->parent()) 
                {
                    taken.append(takeTopLevelItem(indexes.at(i).row()));
                }
            }
            
            // insert them back in at their new positions
            for (int i = 0; i < indexes.count(); ++i) 
            {
                // Either at a specific point or appended
                if (row == -1) 
                {
                    insertTopLevelItem(topLevelItemCount(), taken.takeFirst());
                    if (i==0) setCurrentItem(topLevelItem(topLevelItemCount()-1));
                    else topLevelItem(topLevelItemCount()-1)->setSelected(true);
                } 
                else 
                {
                    int r = dropRow.row() >= 0 ? dropRow.row() : row;
                    insertTopLevelItem(qMin(r, topLevelItemCount()), taken.takeFirst());
                    if (i==0) setCurrentItem(topLevelItem(qMin(r, topLevelItemCount()-1)));
                    else topLevelItem(qMin(r, topLevelItemCount()-1))->setSelected(true);
                }
            }
            
            event->accept();
            // Don't want QAbstractItemView to delete it because it was "moved" we already did it
            event->setDropAction(Qt::CopyAction);
        }
    }
    QTreeView::dropEvent(event);
    m_dropRect = QRect();
    emit moved();
}
bool ServerListView::dropOn(QDropEvent *event, int *dropRow, int *dropCol, QModelIndex *dropIndex)
{
    if (event->isAccepted())
        return false;
    
    QModelIndex index;
    // rootIndex() (i.e. the viewport) might be a valid index
    if (viewport()->rect().contains(event->pos())) {
        index = this->indexAt(event->pos());
        if (!index.isValid() || !this->visualRect(index).contains(event->pos()))
            index = rootIndex();
    }
    
    // If we are allowed to do the drop
    if (this->supportedDropActions() & event->dropAction()) {
        int row = -1;
        int col = -1;
        if (index != rootIndex()) 
        {
            m_dropPosition = position(event->pos(), this->visualRect(index));
            switch (m_dropPosition)
            {
                case 0:
                    row = index.row();
                    col = index.column();
                    index = index.parent();
                    break;
                case 1:
                    row = index.row() + 1;
                    col = index.column();
                    index = index.parent();
                    break;
            }
        } 
        else 
        {
            m_dropPosition = 3;
        }
        *dropIndex = index;
        *dropRow = row;
        *dropCol = col;
        if (!droppingOnItself(event, index))
            return true;
    }
    return false;
}
bool ServerListView::droppingOnItself(QDropEvent *event, const QModelIndex &index)
{
    Qt::DropAction dropAction = event->dropAction();
    dropAction = Qt::MoveAction;
    if (event->source() == this) {
        QModelIndexList selectedIndexes = this->selectedIndexes();
        QModelIndex child = index;
        while (child.isValid() && child != rootIndex()) 
        {
            if (selectedIndexes.contains(child))
                return true;
            child = child.parent();
        }
    }
    return false;
}
#include "serverlistview.moc"
