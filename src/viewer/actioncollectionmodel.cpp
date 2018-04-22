
/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2018 Eike Hein <hein@kde.org>
*/

#include "actioncollectionmodel.h"

#include <QMetaEnum>

ActionCollectionModel::ActionCollectionModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_actionCollection(nullptr)
{
}

ActionCollectionModel::~ActionCollectionModel() = default;

QHash<int, QByteArray> ActionCollectionModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"));

    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QVariant ActionCollectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_actionCollection->count()) {
        return QVariant();
    }

    if (role == KeySequence) {
        return m_actionCollection->action(index.row())->shortcut().toString();
    }

    return QVariant();
}

int ActionCollectionModel::rowCount(const QModelIndex &parent) const
{
    if (!m_actionCollection) {
        return 0;
    }

    return parent.isValid() ? 0 : m_actionCollection->count();
}

KActionCollection* ActionCollectionModel::actionCollection() const
{
    return m_actionCollection;
}

void ActionCollectionModel::setActionCollection(KActionCollection* collection)
{
    if (m_actionCollection != collection) {
        beginResetModel();

        if (m_actionCollection) {
            disconnect(m_actionCollection, &KActionCollection::inserted, this, &ActionCollectionModel::reset);
            disconnect(m_actionCollection, &KActionCollection::removed, this, &ActionCollectionModel::reset);
        }

        m_actionCollection = collection;

        if (collection) {
            connect(m_actionCollection, &KActionCollection::inserted, this, &ActionCollectionModel::reset);
            connect(m_actionCollection, &KActionCollection::removed, this, &ActionCollectionModel::reset);
        }

        endResetModel();
    }
}

void ActionCollectionModel::trigger(int row)
{
    if (row < 0 && row > (m_actionCollection->count() - 1)) {
        return;
    }

    m_actionCollection->action(row)->trigger();
}

void ActionCollectionModel::reset()
{
    beginResetModel();
    endResetModel();
}
