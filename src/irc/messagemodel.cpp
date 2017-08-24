/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include "messagemodel.h"

#include <QMetaEnum>

FilteredMessageModel::FilteredMessageModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_filterView(nullptr)
{
}

FilteredMessageModel::~FilteredMessageModel()
{
}

QObject *FilteredMessageModel::filterView() const
{
    return m_filterView;
}

void FilteredMessageModel::setFilterView(QObject *view)
{
    if (m_filterView != view) {
        m_filterView = view;

        invalidateFilter();

        emit filterViewChanged();
    }
}

bool FilteredMessageModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    if (!m_filterView) {
        return false;
    }

    const QModelIndex &sourceIdx = sourceModel()->index(sourceRow, 0);
    const QObject *view = qvariant_cast<QObject *>(sourceIdx.data(MessageModel::View));

    if (view && view == m_filterView) {
        return true;
    }

    return false;
}

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

MessageModel::~MessageModel()
{
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"));

    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.count()) {
        return QVariant();
    }

    const Message &msg = m_messages.at(index.row());

    if (role == Qt::DisplayRole) {
        return msg.text;
    } else if (role == View) {
        return qVariantFromValue<QObject *>(msg.view);
    } else if (role == TimeStamp) {
        return msg.timeStamp;
    } else if (role == Nick) {
        return msg.nick;
    } else if (role == NickColor) {
        return msg.nickColor;
    }

    return QVariant();
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_messages.count();
}

void MessageModel::appendMessage(QObject *view,
    const QString &timeStamp,
    const QString &nick,
    const QColor &nickColor,
    const QString &text)
{
    beginInsertRows(QModelIndex(), 0, 0);

    Message msg;

    msg.view = view;
    msg.timeStamp = timeStamp;
    msg.nick = nick;
    msg.nickColor = nickColor;
    msg.text = text;

    m_messages.prepend(msg);

    endInsertRows();
}

void MessageModel::cullMessages(const QObject *view)
{
    int i = 0;

    while (i < m_messages.count()) {
        const Message &msg = m_messages.at(i);

        if (msg.view == view) {
            beginRemoveRows(QModelIndex(), i, i);
            m_messages.removeAt(i);
            endRemoveRows();
        } else {
            ++i;
        }
    }
}
