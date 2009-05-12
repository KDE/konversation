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

#ifndef SERVERLISTVIEW_H
#define SERVERLISTVIEW_H

#include <QTreeWidget>

#include <QDropEvent>

class ServerListView : public QTreeWidget
{
    Q_OBJECT
    
    public:
        explicit ServerListView(QWidget *parent);
        ~ServerListView();
        
        QList<QTreeWidgetItem*> selectedServerListItems();
        
    signals:
        void moved();
        void aboutToMove();
        
    private:
        int m_dropPosition;
        QRect m_dropRect;
        
    protected:        
        void dragEnterEvent(QDragEnterEvent *e);
        void dragMoveEvent(QDragMoveEvent *e);
        void dragLeaveEvent(QDragLeaveEvent *);
        int position(const QPoint &pos, const QRect &rect);
        void paintDropIndicator(QPainter *painter);
        void paintEvent(QPaintEvent *event);
        void dropEvent(QDropEvent *event);
        bool dropOn(QDropEvent *event, int *dropRow, int *dropCol, QModelIndex *dropIndex);
        bool droppingOnItself(QDropEvent *event, const QModelIndex &index);
};

#endif
