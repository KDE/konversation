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

#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QMetaEnum>

#include <QQuickItem>

#define MAX_MESSAGES 500000
#define MAX_MESSAGES_TOLERANCE 501000
#define ALLOCATION_BATCH_SIZE 500

FilteredMessageModel::FilteredMessageModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_filterView(nullptr)
{
    m_selectionModel = new QItemSelectionModel(this, this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged,
            this, &FilteredMessageModel::selectionChanged);
}

FilteredMessageModel::~FilteredMessageModel()
{
}

void FilteredMessageModel::bla(QObject *obj)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    //item->setKeepMouseGrab(false);
    item->ungrabMouse();
}

QObject *FilteredMessageModel::filterView() const
{
    return m_filterView;
}

void FilteredMessageModel::setFilterView(QObject *view)
{
    if (m_filterView != view) {
        m_filterView = view;

        m_selectionModel->clear();
        invalidateFilter();

        emit filterViewChanged();
    }
}

bool FilteredMessageModel::hasSelection()
{
    return m_selectionModel->hasSelection();
}

void FilteredMessageModel::clearAndSelect(const QVariantList &rows)
{
    QItemSelection newSelection;

    int iRow = -1;

    foreach (const QVariant &row, rows) {
        iRow = row.toInt();

        if (iRow < 0) {
            return;
        }

        const QModelIndex &idx = index(iRow, 0);
        newSelection.select(idx, idx);
    }

    m_selectionModel->select(newSelection, QItemSelectionModel::ClearAndSelect);
}

void FilteredMessageModel::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QModelIndexList indices = selected.indexes();
    indices.append(deselected.indexes());

    foreach(const QModelIndex index, indices) {
        emit dataChanged(index, index,  QVector<int>{MessageModel::Selected});
    }

    emit hasSelectionChanged();

    copySelectionToClipboard(QClipboard::Selection);
}

void FilteredMessageModel::copySelectionToClipboard(QClipboard::Mode mode)
{
    if (!hasSelection()) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();

    if (!clipboard) {
        return;
    }

    QStringList selected;

    foreach (const QModelIndex &index, m_selectionModel->selectedIndexes())
    {
        selected.append(index.data(MessageModel::ClipboardSerialization).toString());
    }

    clipboard->setText(selected.join('\n'), mode);
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

QVariant FilteredMessageModel::data(const QModelIndex &index, int role) const
{
    if (role == MessageModel::Selected) {
        return m_selectionModel->isSelected(index);
    } else if (role == MessageModel::AuthorMatchesPrecedingMessage) {
        const int precedingMessageRow = index.row() - 1;

        if (precedingMessageRow >= 0) {
            const QModelIndex &precedingMessage = QSortFilterProxyModel::index(precedingMessageRow, 0);
            return (index.data(MessageModel::Author) == precedingMessage.data(MessageModel::Author));
        }

        return false;
    } else if (role == MessageModel::TimeStampMatchesPrecedingMessage) {
        const int precedingMessageRow = index.row() - 1;

        if (precedingMessageRow >= 0) {
            const QModelIndex &precedingMessage = QSortFilterProxyModel::index(precedingMessageRow, 0);
            return (index.data(MessageModel::TimeStamp) == precedingMessage.data(MessageModel::TimeStamp));
        }

        return false;
    }

    return QSortFilterProxyModel::data(index, role);
}

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_allocCount(0)
{
    // Pre-allocate batch.
    m_messages.reserve(ALLOCATION_BATCH_SIZE);
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
        return msg.formattedText;
    } else if (role == Type) {
        return (msg.action ? ActionMessage : NormalMessage);
    } else if (role == View) {
        return qVariantFromValue<QObject *>(msg.view);
    } else if (role == TimeStamp) {
        return msg.timeStamp;
    } else if (role == Author) {
        return msg.nick;
    } else if (role == NickColor) {
        return msg.nickColor;
    } else if (role == ClipboardSerialization) {
        return clipboardSerialization(msg);
    }

    return QVariant();
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    // Limit model to MAX_MESSAGES.
    return parent.isValid() ? 0 : qMax(MAX_MESSAGES, m_messages.count());
}

void MessageModel::appendMessage(QObject *view,
    const QString &timeStamp,
    const QString &nick,
    const QColor &nickColor,
    const QString &text,
    const QString &formattedText,
    const MessageType type)
{
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());

    Message msg;

    msg.view = view;
    msg.timeStamp = timeStamp;
    msg.nick = nick;
    msg.nickColor = nickColor;
    msg.text = text;
    msg.formattedText = formattedText;
    msg.action = (type == ActionMessage);

    m_messages.append(msg);

    endInsertRows();

    ++m_allocCount;

    // Grow the list in batches to make the insertion a little
    // faster.
    if (m_allocCount == ALLOCATION_BATCH_SIZE) {
        m_allocCount = 0;
        m_messages.reserve(m_messages.count() + ALLOCATION_BATCH_SIZE);
    }

    // Whenever we grow above MAX_MESSAGES_TOLERANCE, cull to
    // MAX_MESSAGES. I.e. we cull in batches, not on every new
    // message.
    if (m_messages.count() > MAX_MESSAGES_TOLERANCE) {
        m_messages = m_messages.mid(MAX_MESSAGES_TOLERANCE, MAX_MESSAGES);
    }
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

QString MessageModel::clipboardSerialization(const Message& msg) const
{
    // WIPQTQUICK TODO: msg.text is preformatted HTML, we need the raw in the
    // model to derive this properly.
    if (msg.action) {
        return QString("[%1] * %2 %3").arg(msg.timeStamp).arg(msg.nick).arg(msg.text);
    }

    return QString("[%1] <%2> %3").arg(msg.timeStamp).arg(msg.nick).arg(msg.text);

}
