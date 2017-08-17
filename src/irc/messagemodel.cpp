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
    if (!index.isValid() || index.row() >= m_messageList.count()) {
        return QVariant();
    }

    const Message &msg = m_messageList.at(index.row());

    if (role == Qt::DisplayRole) {
        return msg.text;
    } else if (role == ViewId) {
        return msg.viewId;
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
    return parent.isValid() ? 0 : m_messageList.count();
}

void MessageModel::appendMessage(const QString &viewId,
    const QString &timeStamp,
    const QString &nick,
    const QColor &nickColor,
    const QString &text)
{
    beginInsertRows(QModelIndex(), m_messageList.count(), m_messageList.count());

    Message msg;

    msg.viewId = viewId;
    msg.timeStamp = timeStamp;
    msg.nick = nick;
    msg.nickColor = nickColor;
    msg.text = text;

    m_messageList.append(msg);

    endInsertRows();
}
