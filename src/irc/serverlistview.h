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
        ~ServerListView() override;

    private:
        bool badDropSelection();
    Q_SIGNALS:
        void moved();
        void aboutToMove();
        
    protected:        
        void dragMoveEvent(QDragMoveEvent *e) Q_DECL_OVERRIDE;
        void dragLeaveEvent(QDragLeaveEvent *) Q_DECL_OVERRIDE;
        void dragEnterEvent(QDragEnterEvent *) Q_DECL_OVERRIDE;
        void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
};

#endif
