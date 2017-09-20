/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#ifndef COMPLETER_H
#define COMPLETER_H

#include <QObject>
#include <QPointer>
#include <QSortFilterProxyModel>

class Completer;
class Server;
class UserCompletionModel;

class QCompleter;
class QStringListModel;

class MatchesModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

    public:
        explicit MatchesModel(Completer *completer);
        virtual ~MatchesModel();

        QString pinnedMatch() const;
        void setPinnedMatch(const QString &pinnedMatch);

        Q_INVOKABLE QString at(int row);

    Q_SIGNALS:
        void countChanged() const;

    protected:
        bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        Completer *m_completer;
        QString m_pinnedMatch;
};

class Completer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
    Q_PROPERTY(QAbstractItemModel *matches READ matches CONSTANT)

    public:
        explicit Completer(QObject *parent = 0);
        virtual ~Completer();

        QObject *contextView() const;
        void setContextView(QObject *view);

        QAbstractItemModel *sourceModel() const;
        void setSourceModel(QAbstractItemModel *sourceModel);

        QString prefix() const;
        void setPrefix(const QString &prefix);

        QAbstractItemModel *matches() const;

    Q_SIGNALS:
        void prefixChanged() const;

    private:
        QCompleter *m_completer;
        MatchesModel *m_matchesModel;
        QPointer<QAbstractItemModel> m_sourceModel;
        UserCompletionModel *m_userCompletionModel;
        QStringListModel *m_sortedCommandsModel;
        QPointer<QObject> m_contextView;
};

#endif
