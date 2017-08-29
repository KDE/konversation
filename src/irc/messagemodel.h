/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include "chatwindow.h"

#include <QSortFilterProxyModel>

#include <QColor>
#include <QPointer>

struct Message {
    QObject *view;
    QString timeStamp;
    QString nick;
    QColor nickColor;
    QString text;
    bool action;
};

class FilteredMessageModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QObject* filterView READ filterView WRITE setFilterView NOTIFY filterViewChanged)

    public:
        explicit FilteredMessageModel(QObject *parent = 0);
        virtual ~FilteredMessageModel();

        QObject *filterView() const;
        void setFilterView(QObject *view);

        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    signals:
        void filterViewChanged() const;

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        QObject *m_filterView;
};

class MessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum AdditionalRoles {
        Type = Qt::UserRole + 1,
        View,
        TimeStamp,
        TimeStampMatchesPrecedingMessage, // Implemented in FilteredMessageModel for search efficiency.
        Author,
        AuthorMatchesPrecedingMessage, // Implemented in FilteredMessageModel for search efficiency.
        NickColor,
    };
    Q_ENUM(AdditionalRoles)

    enum MessageType {
        NormalMessage = 0,
        ActionMessage
    };
    Q_ENUM(MessageType)

    explicit MessageModel(QObject *parent = 0);
    virtual ~MessageModel();

    QHash<int, QByteArray> roleNames() const override;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE void appendMessage(QObject *view,
        const QString &timeStamp,
        const QString &nick,
        const QColor &nickColor,
        const QString &text,
        const MessageType type = NormalMessage);

    void cullMessages(const QObject *view);

private:
    QList<Message> m_messages;
};
