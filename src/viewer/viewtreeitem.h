/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#ifndef VIEWTREEITEM_H
#define VIEWTREEITEM_H

#include "chatwindow.h"

#include <qobject.h>
#include <q3listview.h>
#include <qpointer.h>
#include <qpixmap.h>


class ChatWindow;
class Images;

class ViewTreeItem : public Q3ListViewItem
{

    public:
        ViewTreeItem(Q3ListView* parent, const QString& name, ChatWindow* view);
        ViewTreeItem(Q3ListViewItem* parent, const QString& name, ChatWindow* view, int sortIndex = -1);
        ViewTreeItem(Q3ListViewItem* parent, Q3ListViewItem* afterItem, const QString& name, ChatWindow* view);
        // Minimal constructor for separator items.
        explicit ViewTreeItem(Q3ListView* parent);
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
        int compare(Q3ListViewItem* i, int col, bool ascending) const;

        void setup();
        void paintFocus(QPainter* p, const QColorGroup& cg, const QRect& r);
        void paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align);

        QColor mixColor(const QColor &color1, const QColor &color2);

    private:
        uint m_sortIndex;
        static int s_availableSortIndex;
        QPointer<ChatWindow> m_view;
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
