/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#ifndef SERVERLISTVIEW_H
#define SERVERLISTVIEW_H

#include <QTreeWidget>

#include <QDropEvent>

/**
 * ServerListView is derived from QTreeWidget and implements overly
 * complex custom drag'n'drop behavior needed in ServerListDialog.
 */
class ServerListView : public QTreeWidget
{
    Q_OBJECT
    
    public:
        explicit ServerListView(QWidget *parent);
        ~ServerListView() override;

    Q_SIGNALS:
        void moved();
        void aboutToMove();

    protected:
        void dragMoveEvent(QDragMoveEvent *e) override;
        void dragLeaveEvent(QDragLeaveEvent *) override;
        void dragEnterEvent(QDragEnterEvent *) override;
        void dropEvent(QDropEvent *event) override;

    private:
        bool badDropSelection();

    private:
        Q_DISABLE_COPY(ServerListView)
};

#endif
