/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#ifndef INPUTHISTORYMODEL_H
#define INPUTHISTORYMODEL_H

#include "chatwindow.h"

#include <QSortFilterProxyModel>

#include <QPointer>

class InputHistoryModel;

struct HistoryItem {
    QObject *view;
    QString text;
    bool editing;
    int cursorPosition;
};

class FilteredInputHistoryModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QObject* filterView READ filterView WRITE setFilterView NOTIFY filterViewChanged)

    public:
        explicit FilteredInputHistoryModel(QObject *parent = nullptr);
        ~FilteredInputHistoryModel() override;

        QObject *filterView() const;
        void setFilterView(QObject *view);

        void setSourceModel(QAbstractItemModel *sourceModel) override;

        Q_INVOKABLE void append(QObject *view, const QString &text,
            bool editing = false, int cursorPosition = -1);
        Q_INVOKABLE void remove(const QModelIndex &index);

    Q_SIGNALS:
        void countChanged() const;
        void filterViewChanged() const;

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        QPointer<QObject> m_filterView;
        InputHistoryModel *m_inputHistoryModel;
};

class InputHistoryModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        enum AdditionalRoles {
            View = Qt::UserRole + 1,
            Editing,
            CursorPosition
        };
        Q_ENUM(AdditionalRoles)

        explicit InputHistoryModel(QObject *parent = nullptr);
        ~InputHistoryModel() override;

        QHash<int, QByteArray> roleNames() const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        void append(QObject *view, const QString &text, bool editing, int cursorPosition);
        void remove(const QModelIndex &index);
        void clear();

        void cull(const QObject *view);

    private:
        QVector<HistoryItem> m_historyItems;
};

#endif
