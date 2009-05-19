/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

/*
ServerListView is derived from QTreeWidget and implements overly
complex custom drag'n'drop behavior needed in ServerListDialog.

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

    private:
        bool badDropSelection();
    signals:
        void moved();
        void aboutToMove();
        
    protected:        
        void dragMoveEvent(QDragMoveEvent *e);
        void dragLeaveEvent(QDragLeaveEvent *);
        void dropEvent(QDropEvent *event);
};

#endif
