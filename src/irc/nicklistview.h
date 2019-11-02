/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Channel Nick List, including context menu
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICKLISTVIEW_H
#define NICKLISTVIEW_H

#include "channel.h"
#include "images.h"
#include "common.h"

#include <QTreeWidget>


class QTimer;


class NickListView : public QTreeWidget
{
    Q_OBJECT

    public:
        NickListView(QWidget* parent, Channel *chan);
        ~NickListView() Q_DECL_OVERRIDE;

        /** Call when the icons have been changed.
         */
        void refresh();
        void setWhatsThis();

        void setSortingEnabled(bool enable); // WARNING: originally non-virtual
        void sortByColumn(int column, Qt::SortOrder order); // WARNING: originally non-virtual
        void fastSetSortingEnabled(bool value);
        int findLowerBound(const QTreeWidgetItem& item) const;
        void executeDelayedItemsLayout();

        static int getMinimumRowHeight();

        // A helper class to disable sorting while in scope
        class NoSorting
        {
            private:
                NickListView *w;
                bool enabled;
            public:
                NoSorting(NickListView *w) : w(w), enabled(w->isSortingEnabled())
                {
                    if (enabled) w->fastSetSortingEnabled(false);
                }
                ~NoSorting()
                {
                    if (enabled) w->fastSetSortingEnabled(true);
                }
        };

    public Q_SLOTS:
        /** Resort the listview. CAUTION: this might be CPU intensive
         */
        void resort();

    protected:
        //! Reimplemented for dynamic tooltips
        bool event(QEvent* ev) Q_DECL_OVERRIDE;
        void contextMenuEvent(QContextMenuEvent* ev) Q_DECL_OVERRIDE;

        // Drag & Drop support
        QStringList mimeTypes () const Q_DECL_OVERRIDE;
        bool canDecodeMime(QDropEvent const *event) const;
        bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action) Q_DECL_OVERRIDE;
        void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
        void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;

        Channel *m_channel;
        QTimer *m_resortTimer;

    private:
        static int s_minimumRowHeight;
        static void updateMinimumRowHeight();
};
#endif
