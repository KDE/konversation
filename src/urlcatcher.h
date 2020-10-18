/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2010 Eike Hein <hein@kde.org>
*/

#ifndef URLCATCHER_H
#define URLCATCHER_H

#include <QSortFilterProxyModel>
#include <QStandardItem>

#include "chatwindow.h"


class QTreeView;
class QLineEdit;
class QMenu;
class KToolBar;


class UrlDateItem : public QStandardItem
{
    public:
        explicit UrlDateItem(const QDateTime& dateTime);
        ~UrlDateItem() override;

        QVariant data(int role) const override;

  private:
        Q_DISABLE_COPY(UrlDateItem)
};


class UrlSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
        explicit UrlSortFilterProxyModel(QObject* parent = nullptr);
        ~UrlSortFilterProxyModel() override;

        Qt::ItemFlags flags(const QModelIndex& index) const override;

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        Q_DISABLE_COPY(UrlSortFilterProxyModel)
};


class UrlCatcher : public ChatWindow
{
    Q_OBJECT

    public:
        explicit UrlCatcher(QWidget* parent);
        ~UrlCatcher() override;


    protected:
        void childAdjustFocus() override;
        bool event(QEvent* event) override;


    private Q_SLOTS:
        void updateItemActionStates();
        void updateListActionStates();
        void openContextMenu(const QPoint& p);
        void openUrl(const QModelIndex& index);
        void openSelectedUrls();
        void saveSelectedUrls();
        void bookmarkSelectedUrls();
        void copySelectedUrls();
        void deleteSelectedUrls();
        void saveUrlModel();
        void clearUrlModel();

        void updateFilter();
        void startFilterTimer(const QString &filter);

    private:
        void setupActions();
        void setupUrlTree();

    private:
        KToolBar* m_toolBar;
        QMenu* m_contextMenu;

        QList<QAction*> m_itemActions;
        QList<QAction*> m_listActions;

        QTreeView* m_urlTree;
        QLineEdit* m_searchLine;
        QTimer* m_filterTimer;

        Q_DISABLE_COPY(UrlCatcher)
};

#endif
