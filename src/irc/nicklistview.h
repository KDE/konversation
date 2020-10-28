/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef NICKLISTVIEW_H
#define NICKLISTVIEW_H

#include "channel.h"
#include "images.h"
#include "common.h"

#include <QTreeWidget>




/**
 *  Channel Nick List, including context menu
 */
class NickListView : public QTreeWidget
{
    Q_OBJECT

    public:
        NickListView(QWidget* parent, Channel *chan);
        ~NickListView() override;

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

                Q_DISABLE_COPY(NoSorting)

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
        bool event(QEvent* ev) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;

        // Drag & Drop support
        QStringList mimeTypes () const override;
        bool canDecodeMime(QDropEvent const *event) const;
        bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action) override;
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;

    private:
        Channel *m_channel;

        static int s_minimumRowHeight;
        static void updateMinimumRowHeight();

        Q_DISABLE_COPY(NickListView)
};

#endif
