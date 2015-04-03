/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006-2009 Eike Hein <hein@kde.org>
*/

// FIXME KF5 Port: ViewTree TODO: Close buttons, DND, seperator painting.

#ifndef VIEWTREE_H
#define VIEWTREE_H

#include <QStyledItemDelegate>
#include <QTreeView>
#include <QPointer>

class ChatWindow;
class ViewTree;

class ViewTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit ViewTreeDelegate(QObject *parent = 0);
        ~ViewTreeDelegate();

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
        QSize preferredSizeHint(const QModelIndex& index) const;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    private:
        ViewTree* m_view;
};

class ViewTree : public QTreeView
{
    Q_OBJECT

    public:
        explicit ViewTree(QWidget *parent);
        ~ViewTree();

        virtual void setModel(QAbstractItemModel *model);

        bool dropIndicatorOnItem() const;

    public Q_SLOTS:
        void updateAppearance();
        void selectView(const QModelIndex &index);

    Q_SIGNALS:
        void sizeChanged() const;
        void showView(ChatWindow* view) const;
        void closeView(ChatWindow* view) const;
        void showViewContextMenu(QWidget* widget, const QPoint& point) const;

    protected:
        bool event(QEvent* event);
        void paintEvent(QPaintEvent* event);
        void drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        void resizeEvent(QResizeEvent* event);
        void mousePressEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);
        void mouseMoveEvent(QMouseEvent *event);
        void dragEnterEvent(QDragEnterEvent *event);
        void dragMoveEvent(QDragMoveEvent *event);
        void contextMenuEvent(QContextMenuEvent* event);
        void wheelEvent(QWheelEvent* event);
        void keyPressEvent(QKeyEvent* event);

    private Q_SLOTS:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        QPointer<ChatWindow> m_pressedView;

        QPoint m_pressPos;
};

#endif
