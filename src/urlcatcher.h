/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  shows all URLs found by the client
  begin:     Die Mai 27 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef URLCATCHER_H
#define URLCATCHER_H

#include "chatwindow.h"
#include "ui_urlcatcherui.h"

#include <QAbstractListModel>

class QSortFilterProxyModel;

struct UrlItem
{
    QString nick;
    QString url;
};

bool operator==(const UrlItem& item, const UrlItem& item2);

class UrlCatcherModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        UrlCatcherModel(QObject* parent);

        void append(const UrlItem& item);
        virtual bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
        void setUrlList(const QList<UrlItem>& list);
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        int rowCount(const QModelIndex& parent = QModelIndex()) const;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    private:
        QList<UrlItem> m_urlList;
};

class UrlCatcher : public ChatWindow, private Ui::URLCatcherUI
{
    Q_OBJECT

    public:
        explicit UrlCatcher(QWidget* parent);
        ~UrlCatcher();

        virtual bool canBeFrontView()   { return true; }

    signals:
        void deleteUrl(const QString& who,const QString& url);
        void clearUrlList();

    public slots:
        void setUrlList(const QStringList& urlList);
        void addUrl(const QString& who,const QString& url);

    protected slots:
        void urlSelected(const QItemSelection& selected);
        void openUrl(const QModelIndex& index);
        void openUrlClicked();

        void copyUrlClicked();
        void deleteUrlClicked();
        void saveListClicked();
        void clearListClicked();

        void bookmarkUrl();
        void saveLinkAs();
        void contextMenu(const QPoint& p);

        //TODO removable in 4.2 see constructor
        void filterChanged();
        void updateFilter();

    protected:
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        QTimer* m_filterTimer;

        UrlCatcherModel* m_urlListModel;
        QSortFilterProxyModel* m_proxyModel;
};
#endif
