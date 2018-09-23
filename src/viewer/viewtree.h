/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006-2009 Eike Hein <hein@kde.org>
*/

// FIXME KF5 Port: ViewTree TODO: Close buttons, DND, separator painting.

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
        explicit ViewTreeDelegate(QObject *parent = nullptr);
        ~ViewTreeDelegate() Q_DECL_OVERRIDE;

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;
        QSize preferredSizeHint(const QModelIndex& index) const;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;

    private:
        ViewTree* m_view;
};

class ViewTree : public QTreeView
{
    Q_OBJECT

    public:
        explicit ViewTree(QWidget *parent);
        ~ViewTree() Q_DECL_OVERRIDE;

        void setModel(QAbstractItemModel *model) Q_DECL_OVERRIDE;

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
        bool event(QEvent* event) Q_DECL_OVERRIDE;
        void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
        void drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;
        void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;
        void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
        void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
        void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
        void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
        void contextMenuEvent(QContextMenuEvent* event) Q_DECL_OVERRIDE;
        void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;
        void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    private Q_SLOTS:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) Q_DECL_OVERRIDE;

    private:
        QPointer<ChatWindow> m_pressedView;

        QPoint m_pressPos;

        int m_accumulatedWheelDelta;
        bool m_lastWheelDeltaDirection;
};

#endif
