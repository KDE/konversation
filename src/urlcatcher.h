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
  Copyright (C) 2010 Eike Hein <hein@kde.org>
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
        ~UrlDateItem() Q_DECL_OVERRIDE;

        QVariant data(int role) const Q_DECL_OVERRIDE;
};


class UrlSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
        explicit UrlSortFilterProxyModel(QObject* parent = nullptr);
        ~UrlSortFilterProxyModel() Q_DECL_OVERRIDE;

        Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;


    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const Q_DECL_OVERRIDE;
};


class UrlCatcher : public ChatWindow
{
    Q_OBJECT

    public:
        explicit UrlCatcher(QWidget* parent);
        ~UrlCatcher() Q_DECL_OVERRIDE;


    protected:
        void childAdjustFocus() Q_DECL_OVERRIDE;
        bool event(QEvent* event) Q_DECL_OVERRIDE;


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

        KToolBar* m_toolBar;
        QMenu* m_contextMenu;

        QList<QAction*> m_itemActions;
        QList<QAction*> m_listActions;

        QTreeView* m_urlTree;
        QLineEdit* m_searchLine;
        QTimer* m_filterTimer;
};

#endif
