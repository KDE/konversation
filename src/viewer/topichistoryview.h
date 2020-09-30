/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2005-2007 Peter Simonsson <psn@linux.se>
  Copyright (C) 2012 Eike Hein <hein@kde.org>
*/

#ifndef TOPICHISTORYVIEW_H
#define TOPICHISTORYVIEW_H

#include <KCategorizedSortFilterProxyModel>
#include <KCategorizedView>
#include <KWidgetItemDelegate>


#include <KTextEdit>


class Server;


class TopicHistorySortfilterProxyModel : public KCategorizedSortFilterProxyModel
{
    Q_OBJECT

    friend class TopicHistoryView;

    public:
        explicit TopicHistorySortfilterProxyModel(QObject* parent = nullptr);
        ~TopicHistorySortfilterProxyModel();

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        void setSourceModel(QAbstractItemModel* model) override;


    protected:
        bool filterAcceptsColumn ( int source_column, const QModelIndex & source_parent ) const override;


    private Q_SLOTS:
        void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
};


class TopicHistoryLabel : public KTextEdit
{
    Q_OBJECT

    public:
        explicit TopicHistoryLabel(QWidget* parent = nullptr);
        ~TopicHistoryLabel();


    public Q_SLOTS:
        void setTextSelectable(bool selectable);
};


class TopicHistoryItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

    public:
        explicit TopicHistoryItemDelegate(QAbstractItemView* itemView, QObject* parent = nullptr);
        ~TopicHistoryItemDelegate();

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        bool eventFilter(QObject* watched, QEvent* event) override;


    protected:
        QList<QWidget*> createItemWidgets (const QModelIndex& index) const override;
        void updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem& option,
            const QPersistentModelIndex& index) const override;


    private:
        TopicHistoryLabel* m_hiddenLabel;
        bool m_shownBefore;
};


class TopicHistoryView : public KCategorizedView
{
    Q_OBJECT

    public:
        explicit TopicHistoryView(QWidget* parent = nullptr);
        ~TopicHistoryView();

        void setServer(Server* server) { m_server = server; }

        bool textSelectable() const;
        void setTextSelectable(bool selectable);

        void setModel(QAbstractItemModel* model) override;


    Q_SIGNALS:
        void textSelectableChanged(bool selectable);


    protected:
        void resizeEvent(QResizeEvent* event) override;
        void contextMenuEvent (QContextMenuEvent* event) override;
        void updateGeometries() override;


    private Q_SLOTS:
        void updateSelectedItemWidgets();


    private:
        Server* m_server;
        TopicHistorySortfilterProxyModel* m_proxyModel;
        bool m_textSelectable;
};

#endif

struct Topic;
