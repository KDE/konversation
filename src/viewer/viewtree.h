/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006-2009 Eike Hein <hein@kde.org>
*/

#ifndef VIEWTREE_H
#define VIEWTREE_H

#include <k3listview.h>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>


class ViewTreeItem;
class ChatWindow;

class ViewTree : public K3ListView
{
    Q_OBJECT

    public:
        explicit ViewTree(QWidget *parent);
        ~ViewTree();

        void selectFirstView(bool select);
        void addView(const QString& name, ChatWindow* view, const QIcon &iconset, bool select = false, ChatWindow* afterView = 0);
        void setViewName(ChatWindow* view, const QString& name);
        void setViewColor(ChatWindow* view, QColor color);
        void setViewIcon(ChatWindow* view, const QIcon &iconset);

        void moveViewUp(ChatWindow* view);
        void moveViewDown(ChatWindow* view);
        bool canMoveViewUp(ChatWindow* view);
        bool canMoveViewDown(ChatWindow* view);

        QList<ChatWindow*> getSortedViewList();

    public slots:
        void updateAppearance();
        void removeView(ChatWindow* view);
        void selectView(ChatWindow* view);
        void unHighlight();

    signals:
        void setViewTreeShown(bool show);
        void showView(ChatWindow* view);
        void closeView(ChatWindow* view);
        void showViewContextMenu(QWidget* widget, const QPoint& point);
        void sizeChanged();
        void syncTabBarToTree();

    protected:
        bool event(QEvent* e);
        void contentsMousePressEvent(QMouseEvent* e);
        void contentsMouseReleaseEvent(QMouseEvent* e);
        void contentsMouseMoveEvent(QMouseEvent* e);
        void contentsWheelEvent(QWheelEvent* e);
        void contentsContextMenuEvent(QContextMenuEvent* e);
        void keyPressEvent(QKeyEvent* e);

        void resizeEvent(QResizeEvent* e);

        void findDrop(const QPoint &pos, Q3ListViewItem *&parent, Q3ListViewItem *&after);
        Q3DragObject* dragObject();

        void paintEmptyArea(QPainter* p, const QRect& rect);

    private slots:
        void announceSelection(Q3ListViewItem* item);
        void slotAboutToMoveView();
        void slotMovedView();
        void enableCloseButton();

    private:
        void toggleSeparator();
        void selectUpper(bool wrap = false);
        void selectLower(bool wrap = false);

        ViewTreeItem* getItemForView(ChatWindow* view);
        ViewTreeItem* getParentItemForView(ChatWindow* view);
        ViewTreeItem* getLastChild(Q3ListViewItem* parent);

        bool canMoveItemUp(ViewTreeItem* item);
        bool canMoveItemDown(ViewTreeItem* item);

        bool isAboveIcon(QPoint point, ViewTreeItem* item);
        void hideCloseButtons(ViewTreeItem* exception = 0);

        ViewTreeItem* m_separator;
        int m_specialViewCount;

        // Controls whether or not to select the first view added
        // to the tree.
        bool m_selectFirstView;

        // Used in mouse handling to determine whether both the press
        // and the release event occurred over a close button.
        bool m_pressedAboveCloseButton;
        ViewTreeItem* m_closeButtonItem;
        QTimer* m_enableCloseButtonTimer;

        // Used for middle-click close.
        ViewTreeItem* m_middleClickItem;
};

#endif
