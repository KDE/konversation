/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include <QAbstractListModel>

#include <QColor>

struct Message {
    QString viewId; // HACK
    QString timeStamp;
    QString nick;
    QColor nickColor;
    QString text;
};

class MessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum AdditionalRoles {
        ViewId = Qt::UserRole + 1,
        TimeStamp,
        Nick,
        NickColor
    };
    Q_ENUM(AdditionalRoles)

    explicit MessageModel(QObject *parent = 0);
    virtual ~MessageModel();

    QHash<int, QByteArray> roleNames() const override;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE void appendMessage(const QString &viewId,
        const QString &timeStamp,
        const QString &nick,
        const QColor &nickColor,
        const QString &text);

private:
    QList<Message> m_messageList;
};
