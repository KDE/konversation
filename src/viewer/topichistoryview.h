/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2005-2007 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
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
        ~TopicHistorySortfilterProxyModel() override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        void setSourceModel(QAbstractItemModel* model) override;


    protected:
        bool filterAcceptsColumn ( int source_column, const QModelIndex & source_parent ) const override;


    private Q_SLOTS:
        void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    private:
        Q_DISABLE_COPY(TopicHistorySortfilterProxyModel)
};


class TopicHistoryLabel : public KTextEdit
{
    Q_OBJECT

    public:
        explicit TopicHistoryLabel(QWidget* parent = nullptr);
        ~TopicHistoryLabel() override;


    public Q_SLOTS:
        void setTextSelectable(bool selectable);

    private:
        Q_DISABLE_COPY(TopicHistoryLabel)
};


class TopicHistoryItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

    public:
        explicit TopicHistoryItemDelegate(QAbstractItemView* itemView, QObject* parent = nullptr);
        ~TopicHistoryItemDelegate() override;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        bool eventFilter(QObject* watched, QEvent* event) override;


    protected:
        QList<QWidget*> createItemWidgets (const QModelIndex& index) const override;
        void updateItemWidgets(const QList<QWidget *> &widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const override;

    private:
        TopicHistoryLabel* m_hiddenLabel;
        bool m_shownBefore;

        Q_DISABLE_COPY(TopicHistoryItemDelegate)
};


class TopicHistoryView : public KCategorizedView
{
    Q_OBJECT

    public:
        explicit TopicHistoryView(QWidget* parent = nullptr);
        ~TopicHistoryView() override;

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

        Q_DISABLE_COPY(TopicHistoryView)
};

#endif

struct Topic;
