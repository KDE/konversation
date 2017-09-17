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

#include <QClipboard>
#include <QColor>
#include <QPointer>

class QItemSelectionModel;

struct Message {
    QObject *view;
    QString timeStamp;
    QString nick;
    QColor nickColor;
    QString text;
    QString formattedText;
    bool action;
};

class FilteredMessageModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QObject* filterView READ filterView WRITE setFilterView NOTIFY filterViewChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)

    public:
        explicit FilteredMessageModel(QObject *parent = 0);
        virtual ~FilteredMessageModel();

        QObject *filterView() const;
        void setFilterView(QObject *view);

        bool hasSelection();

        Q_INVOKABLE void clearAndSelect(const QVariantList &rows);
        Q_INVOKABLE void copySelectionToClipboard(QClipboard::Mode mode = QClipboard::Clipboard);

        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        Q_INVOKABLE void bla(QObject *obj);

    Q_SIGNALS:
        void filterViewChanged() const;
        void hasSelectionChanged() const;

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private Q_SLOTS:
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    private:
        QObject *m_filterView;
        QItemSelectionModel *m_selectionModel;
};

class MessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum AdditionalRoles {
        Selected = Qt::UserRole + 1, // WIPQTQUICK TODO This is implemented by FMM, maybe I should extend roles there.
        Type,
        View,
        TimeStamp,
        TimeStampMatchesPrecedingMessage, // Implemented in FilteredMessageModel for search efficiency.
        Author,
        AuthorMatchesPrecedingMessage, // Implemented in FilteredMessageModel for search efficiency.
        NickColor,
        ClipboardSerialization
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
        const QString &formattedText,
        const MessageType type = NormalMessage);

    void cullMessages(const QObject *view);

private:
    QString clipboardSerialization(const Message &msg) const;

    QVector<Message> m_messages;
    int m_allocCount;
};
