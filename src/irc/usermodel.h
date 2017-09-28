/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#ifndef USERMODEL_H
#define USERMODEL_H

#include "chatwindow.h"
#include "channelnick.h"

#include <QPointer>
#include <QSortFilterProxyModel>

class NickInfo;
class Server;

class QItemSelectionModel;

class UserCompletionModel : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
        explicit UserCompletionModel(QObject *parent = 0);
        virtual ~UserCompletionModel();

        Server *server() const;
        void setServer(Server *server);

        QString lastActiveUser();

        virtual void setSourceModel(QAbstractItemModel *sourceModel) override;

        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        QPointer<Server> m_server;
};

class FilteredUserModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QObject* filterView READ filterView WRITE setFilterView NOTIFY filterViewChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
    Q_PROPERTY(QStringList selectedNames READ selectedNames NOTIFY hasSelectionChanged)

    public:
        explicit FilteredUserModel(QObject *parent = 0);
        virtual ~FilteredUserModel();

        QObject *filterView() const;
        void setFilterView(QObject *view);

        virtual void setSourceModel(QAbstractItemModel *sourceModel) override;

        bool hasSelection() const;
        QStringList selectedNames() const;

        Q_INVOKABLE void toggleSelected(int row);
        Q_INVOKABLE void clearAndSelect(const QVariantList &rows);
        Q_INVOKABLE void setRangeSelected(int anchor, int to);
        Q_INVOKABLE void selectAll();

        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_SIGNALS:
        void filterViewChanged() const;
        void hasSelectionChanged() const;

    private Q_SLOTS:
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        ChannelNickPtr getChannelNick(const NickInfo *nickInfo);

        QPointer<QObject> m_filterView;
        QHash<const NickInfo *, ChannelNickPtr> m_channelNickCache;

        QItemSelectionModel *m_selectionModel;
};

class UserModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        enum AdditionalRoles {
            LowercaseName = Qt::UserRole + 1,
            TimeStamp, // WIPQTQUICK Only used by FilteredUserModel.
            Selected // WIPQTQUICK TODO This is implemented by FUM, maybe I should extend roles there.
        };
        Q_ENUM(AdditionalRoles)

        explicit UserModel(QObject *parent = 0);
        virtual ~UserModel();

        QHash<int, QByteArray> roleNames() const override;

        virtual QModelIndex index(int row, int column,
            const QModelIndex &parent = QModelIndex()) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    protected:
        friend class Server;
        void add(const NickInfoPtr nickInfo);
        void remove(const NickInfoPtr nickInfo);
        void changed(const NickInfoPtr nickInfo);
        void clear();

    private:
        QVector<NickInfoPtr> m_nicks;
};

#endif
