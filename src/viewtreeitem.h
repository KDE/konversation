/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#ifndef VIEWTREEITEM_H
#define VIEWTREEITEM_H

#include <qobject.h>
#include <qlistview.h>
#include <qtooltip.h>
#include <qguardedptr.h>

#include "chatwindow.h"

class ChatWindow;
class Images;

class ViewTreeItem : public QListViewItem
{

    public:
        ViewTreeItem(QListView* parent, const QString& name, ChatWindow* view);
        ViewTreeItem(QListViewItem* parent, const QString& name, ChatWindow* view, int sortIndex = -1);
        ViewTreeItem(QListViewItem* parent, QListViewItem* afterItem, const QString& name, ChatWindow* view);
        // Minimal constructor for separator items.
        explicit ViewTreeItem(QListView* parent);
        ~ViewTreeItem();

        void setSortIndex(int newSortIndex);
        int getSortIndex() const;

        void setName(const QString& name);
        QString getName() const;
        bool isTruncated() const;

        void setView(ChatWindow* view);
        ChatWindow* getView() const;

        ChatWindow::WindowType getViewType() const;
        void setViewType(ChatWindow::WindowType);

        void setColor(QColor color);
        QColor getColor() const;

        void setIcon(const QPixmap& pm);

        void setHighlighted(bool highlight);
        void setCloseButtonShown(bool show);
        void setCloseButtonEnabled();
        bool getCloseButtonEnabled();

        bool sortLast() const;
        bool isSeparator() const;
        int compare(QListViewItem* i, int col, bool ascending) const;

        void setup();
        void paintFocus(QPainter* p, const QColorGroup& cg, const QRect& r);
        void paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align);

        QColor mixColor(const QColor &color1, const QColor &color2);

    private:
        uint m_sortIndex;
        static int s_availableSortIndex;
        QGuardedPtr<ChatWindow> m_view;
        ChatWindow::WindowType m_viewType;
        QColor m_color;

        bool m_isSeparator;
        bool m_isHighlighted;
        bool m_isTruncated;
        bool m_customColorSet;

        Images* images;

        QPixmap m_closeButton;
        QPixmap m_disabledCloseButton;
        QPixmap m_oldPixmap;
        bool m_closeButtonShown;
        bool m_closeButtonEnabled;
};

#endif
