/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006-2009 Eike Hein <hein@kde.org>
*/

/* FIXME KF5 port
FIXME ViewTree port

Currently missing:

- Custom delegate painting
- Close buttons
- DND
- Middle click close
- Seperator painting
*/

#ifndef VIEWTREE_H
#define VIEWTREE_H

#include <QStyledItemDelegate>
#include <QListView>

class ChatWindow;

class ViewTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit ViewTreeDelegate(QObject *parent = 0);
        ~ViewTreeDelegate();

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

class ViewTree : public QListView
{
    Q_OBJECT

    public:
        explicit ViewTree(QWidget *parent);
        ~ViewTree();

    public Q_SLOTS:
        void updateAppearance();
        void selectView(const QModelIndex &index);

    Q_SIGNALS:
        void sizeChanged() const;
        void showView(ChatWindow* view) const;
        void showViewContextMenu(QWidget* widget, const QPoint& point) const;

    protected:
        void resizeEvent(QResizeEvent* event);
        void contextMenuEvent(QContextMenuEvent* event);
        void wheelEvent(QWheelEvent* event);
        void keyPressEvent(QKeyEvent* event);

    private Q_SLOTS:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
};

#endif
