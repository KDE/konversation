/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include "usermodel.h"
#include "chatwindow.h"
#include "nickinfo.h"
#include "server.h"

#include <QMetaEnum>

FilteredUserModel::FilteredUserModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_filterView(nullptr)
{
    setSortRole(UserModel::LowercaseName);
    sort(0);
}

FilteredUserModel::~FilteredUserModel()
{
}

void FilteredUserModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (QSortFilterProxyModel::sourceModel() == sourceModel) {
        return;
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);

    QObject::connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this,
        [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent)

            for (int i = first; i <= last; ++i) {
                const QModelIndex &sourceIdx = QSortFilterProxyModel::sourceModel()->index(i, 0);
                m_channelNickCache.remove(static_cast<NickInfo *>(sourceIdx.internalPointer()));
            }
        }
    );
}

QObject *FilteredUserModel::filterView() const
{
    return m_filterView;
}

void FilteredUserModel::setFilterView(QObject *view)
{
    // WIPQTQUICK HACK Only filter for channels for now.
    const ChatWindow *chatWin = qobject_cast<ChatWindow *>(view);

    if (view) {
        if (chatWin->getType() != ChatWindow::Channel) {
            setFilterView(nullptr);
            return;
        }
    }

    if (m_filterView != view) {
        m_filterView = view;

        if (view) {
            setSourceModel(chatWin->getServer()->getUserModel());
        }

        m_channelNickCache.clear();
        invalidateFilter();

        emit filterViewChanged();
    }
}

bool FilteredUserModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    if (!m_filterView) {
        return true;
    }

    // WIPQTQUICK HACK
    const ChatWindow *chatWin = qobject_cast<ChatWindow *>(m_filterView);

    const QModelIndex &sourceIdx = sourceModel()->index(sourceRow, 0);
    const NickInfo *nickInfo = static_cast<NickInfo *>(sourceIdx.internalPointer());
    Server *server = chatWin->getServer();
    const QStringList &channels = server->getNickJoinedChannels(nickInfo->getNickname());

    if (channels.contains(chatWin->getName())) {
        return true;
    }

    return false;
}

QVariant FilteredUserModel::data(const QModelIndex &index, int role) const
{
    if (!m_filterView) {
        return QSortFilterProxyModel::data(index, role);
    }

    if (role == UserModel::TimeStamp) {
        const QModelIndex &sourceIdx = mapToSource(index);
        // WIPQTQUICK Oh my god the hoops/casts.
        const ChannelNickPtr channelNick = const_cast<FilteredUserModel *>(this)->getChannelNick(static_cast<NickInfo *>(sourceIdx.internalPointer()));

        if (channelNick) {
            channelNick->timeStamp();
        }
    }

    return QSortFilterProxyModel::data(index, role);
}

ChannelNickPtr FilteredUserModel::getChannelNick(const NickInfo *nickInfo)
{
    if (!m_filterView) {
        return ChannelNickPtr();
    }

    const auto &it = m_channelNickCache.constFind(nickInfo);

    if (it != m_channelNickCache.constEnd()) {
        return *it;
    }

    // WIPQTQUICK HACK
    const ChatWindow *chatWin = qobject_cast<ChatWindow *>(m_filterView);

    const ChannelNickPtr channelNick = chatWin->getServer()->getChannelNick(chatWin->getName(),
        nickInfo->getNickname());
    m_channelNickCache.insert(nickInfo, channelNick);

    return channelNick;
}

UserModel::UserModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

UserModel::~UserModel()
{
}

QHash<int, QByteArray> UserModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"));

    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QModelIndex UserModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(column)
    Q_UNUSED(parent)

    if (row < 0 || row >= m_nicks.count()) {
        return QModelIndex();
    }

    return createIndex(row, column, m_nicks.at(row).data());
}

QVariant UserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_nicks.count()) {
        return QVariant();
    }

    const NickInfoPtr nick = m_nicks.at(index.row());

    if (role == Qt::DisplayRole) {
        return nick->getNickname();
    } else if (role == LowercaseName) {
        return nick->loweredNickname();
    } /*if (role == CompletionMangling) {
        QString mangled(nick->getNickname());

        for (int i = mangled.length(); i >= 0; --i) {
            const QChar &char = mangled[i];

            if (!char.isLetterOrNumber()) {
                mangled.remove(i, 1);
            }
        }

        return mangled;
    }*/

    return QVariant();
}

int UserModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_nicks.count();
}

void UserModel::add(const NickInfoPtr nickInfo)
{
    beginInsertRows(QModelIndex(), m_nicks.count(), m_nicks.count());

    m_nicks.append(nickInfo);

    endInsertRows();
}

void UserModel::remove(const NickInfoPtr nickInfo)
{
    const int row = m_nicks.indexOf(nickInfo);

    if (row != -1) {
        beginRemoveRows(QModelIndex(), row, row);

        m_nicks.remove(row);

        endRemoveRows();
    }
}

void UserModel::changed(const NickInfoPtr nickInfo)
{
    const int row = m_nicks.indexOf(nickInfo);

    if (row != -1) {
        const QModelIndex &idx = index(row, 0);
        emit dataChanged(idx, idx); // WIPQTQUICK TODO Only updated roles that actually changed.
    }
}

void UserModel::clear()
{
    beginResetModel();

    m_nicks.clear();

    endResetModel();
}
