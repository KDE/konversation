/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include "completer.h"
#include "chatwindow.h" // WIPQTQUICK TODO Fix coupling
#include "outputfilter.h"
#include "server.h" // WIPQTQUICK TODO Fix coupling
#include "usermodel.h" // WIPQTQUICK TODO Fix coupling

#include <QCompleter>
#include <QStringListModel>

MatchesModel::MatchesModel(Completer *completer)
    : QSortFilterProxyModel(completer)
    , m_completer(completer)
{
    sort(0);

    QObject::connect(this, &QAbstractItemModel::modelReset, this, &MatchesModel::countChanged);
    QObject::connect(this, &QAbstractItemModel::rowsInserted, this, &MatchesModel::countChanged);
    QObject::connect(this, &QAbstractItemModel::rowsRemoved, this, &MatchesModel::countChanged);
}

MatchesModel::~MatchesModel() = default;

QString MatchesModel::pinnedMatch() const
{
    return m_pinnedMatch;
}

void MatchesModel::setPinnedMatch(const QString& pinnedMatch)
{
    if (m_pinnedMatch != pinnedMatch) {
        m_pinnedMatch = pinnedMatch;

        // Force resort.
        setDynamicSortFilter(false);
        setDynamicSortFilter(true);
    }
}

QString MatchesModel::at(int row) const
{
    if (row < 0 || row >= rowCount()) {
        return QString();
    }

    return data(index(row, 0)).toString();
}

bool MatchesModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    if (!m_pinnedMatch.isEmpty()) {
        bool leftIsPin(sourceLeft.data().toString() == m_pinnedMatch);
        bool rightIsPin(sourceRight.data().toString() == m_pinnedMatch);

        if (leftIsPin && !rightIsPin) {
            return true;
        } else if (rightIsPin && !leftIsPin) {
            return false;
        }
    }

    return (sourceLeft.row() < sourceRight.row());
}

bool MatchesModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    const QModelIndex &sourceIdx = sourceModel()->index(sourceRow, 0);

    // Filter out matches that are identical to the prefix.
    if (sourceIdx.data().toString().toLower() == m_completer->prefix()) {
        return false;
    }

    return true;
}

Completer::Completer(QObject *parent)
    : QObject(parent)
    , m_completer(new QCompleter(this))
    , m_matchesModel(new MatchesModel(this))
    , m_userCompletionModel(new UserCompletionModel(this))
    , m_sortedCommandsModel(nullptr)
{
    m_completer->setCompletionMode(QCompleter::InlineCompletion);
    m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
}

Completer::~Completer() = default;

QObject *Completer::contextView() const
{
    return m_contextView;
}

void Completer::setContextView(QObject *view)
{
    if (m_contextView != view) {
        m_contextView = view;

        // WIPQTQUICK HACK
        const ChatWindow *chatWin = qobject_cast<ChatWindow *>(view);

        if (view) {
            QObject::connect(view, &QObject::destroyed, this,
                [this]() {
                    m_userCompletionModel->setSourceModel(nullptr);
                    m_matchesModel->setPinnedMatch(QString());
                }
            );

            m_userCompletionModel->setServer(chatWin->getServer());
        }

        m_completer->setCompletionPrefix(QString());
    }
}

QAbstractItemModel *Completer::sourceModel() const
{
    return m_sourceModel;
}

void Completer::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (m_sourceModel != sourceModel) {
        m_sourceModel = sourceModel;

        if (!m_completer->completionPrefix().isEmpty() && sourceModel) {
            m_userCompletionModel->setSourceModel(sourceModel);
        }
    }
}

QString Completer::prefix() const
{
    return m_completer->completionPrefix();
}

void Completer::setPrefix(const QString &prefix)
{
    if (!m_contextView || !m_sourceModel) {
        return;
    }

    if (m_completer->completionPrefix() != prefix) {
        if (!prefix.isEmpty()) {
            if (prefix.startsWith(Preferences::self()->commandChar())) {
                if (!m_sortedCommandsModel) {
                    QStringList sortedCommands;

                    for (const QString &s : Konversation::OutputFilter::supportedCommands()) {
                        sortedCommands.append(Preferences::self()->commandChar() + s);
                    }

                    m_sortedCommandsModel = new QStringListModel(sortedCommands);
                    m_sortedCommandsModel->sort(0);
                }

                m_matchesModel->setPinnedMatch(QString());

                m_completer->setModel(m_sortedCommandsModel);
                m_completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
            } else {
                m_userCompletionModel->setSourceModel(m_sourceModel);
                m_matchesModel->setPinnedMatch(m_userCompletionModel->lastActiveUser());

                m_completer->setModel(m_userCompletionModel);
                m_completer->setModelSorting(QCompleter::UnsortedModel);
            }

            m_completer->setCompletionPrefix(prefix.toLower());
            m_matchesModel->setSourceModel(m_completer->completionModel());
        } else {
            m_completer->setModel(nullptr);
            m_userCompletionModel->setSourceModel(nullptr);
            m_matchesModel->setSourceModel(nullptr);
            m_completer->setCompletionPrefix(prefix);
        }

        m_completer->setCompletionPrefix(prefix.toLower());

        emit prefixChanged();
    }
}

QAbstractItemModel *Completer::matches() const
{
    return m_matchesModel;
}
