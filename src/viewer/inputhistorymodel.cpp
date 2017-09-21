/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include "inputhistorymodel.h"

#include <QMetaEnum>

FilteredInputHistoryModel::FilteredInputHistoryModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_filterView(nullptr)
{
    QObject::connect(this, &QAbstractItemModel::modelReset, this, &FilteredInputHistoryModel::countChanged);
    QObject::connect(this, &QAbstractItemModel::rowsInserted, this, &FilteredInputHistoryModel::countChanged);
    QObject::connect(this, &QAbstractItemModel::rowsRemoved, this, &FilteredInputHistoryModel::countChanged);
}

FilteredInputHistoryModel::~FilteredInputHistoryModel()
{
}

QObject *FilteredInputHistoryModel::filterView() const
{
    return m_filterView;
}

void FilteredInputHistoryModel::setFilterView(QObject *view)
{
    if (m_filterView != view) {
        m_filterView = view;

        QObject::connect(view, &QObject::destroyed,
            this, [this]() { invalidateFilter(); });

        invalidateFilter();

        emit filterViewChanged();
    }
}

void FilteredInputHistoryModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    m_inputHistoryModel = qobject_cast<InputHistoryModel *>(sourceModel);

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

void FilteredInputHistoryModel::append(QObject *view, const QString &text, bool editing, int cursorPosition)
{
    if (!m_inputHistoryModel) {
        return;
    }

    m_inputHistoryModel->append(view, text, editing, cursorPosition);
}

void FilteredInputHistoryModel::remove(const QModelIndex &index)
{
    m_inputHistoryModel->remove(mapToSource(index));
}

bool FilteredInputHistoryModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    if (!m_filterView) {
        return true;
    }

    const QModelIndex &sourceIdx = sourceModel()->index(sourceRow, 0);
    const QObject *view = qvariant_cast<QObject *>(sourceIdx.data(InputHistoryModel::View));

    if (view && view == m_filterView) {
        return true;
    }

    return false;
}

InputHistoryModel::InputHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

InputHistoryModel::~InputHistoryModel()
{
}

QHash<int, QByteArray> InputHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"));

    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QVariant InputHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_historyItems.count()) {
        return QVariant();
    }

    const HistoryItem &item = m_historyItems.at(index.row());

    if (role == Qt::DisplayRole) {
        return item.text;
    } else if (role == View) {
        return qVariantFromValue<QObject *>(item.view);
    } else if (role == Editing) {
        return item.editing;
    } else if (role == CursorPosition) {
        return item.cursorPosition;
    }

    return QVariant();
}

int InputHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_historyItems.count();
}

void InputHistoryModel::append(QObject *view, const QString &text, bool editing, int cursorPosition)
{
    if (!view || text.isEmpty()) {
        return;
    }

    beginInsertRows(QModelIndex(), m_historyItems.count(), m_historyItems.count());

    HistoryItem item;

    item.view = view;
    item.text = text;
    item.editing = editing;
    item.cursorPosition = cursorPosition;

    m_historyItems.append(item);

    endInsertRows();
}

void InputHistoryModel::remove(const QModelIndex& index)
{
    if (!index.isValid() || index.row() >= m_historyItems.count()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    m_historyItems.removeAt(index.row());
    endRemoveRows();
}

void InputHistoryModel::cull(const QObject *view)
{
    int i = 0;

    while (i < m_historyItems.count()) {
        const HistoryItem &item = m_historyItems.at(i);

        if (item.view == view) {
            beginRemoveRows(QModelIndex(), i, i);
            m_historyItems.removeAt(i);
            endRemoveRows();
        } else {
            ++i;
        }
    }
}

void InputHistoryModel::clear()
{
    beginResetModel();

    m_historyItems.clear();

    endResetModel();
}
