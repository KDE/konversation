/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#ifndef TOPICHISTORYMODEL_H
#define TOPICHISTORYMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include "channel.h"


#if HAVE_QCA2
namespace Konversation
{
    class Cipher;
}
#endif


struct Topic
{
    QString author;
    QString text;
    QDateTime timestamp;

    bool operator==(const Topic& other) const
    {
        return (author == other.author
            && text == other.text
            && timestamp == other.timestamp);
    }
};

class TopicHistoryModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        explicit TopicHistoryModel(QObject* parent = nullptr);
        ~TopicHistoryModel() override;

        QString currentTopic() const;

        void appendTopic(const QString& text, const QString& author = QString(), const QDateTime &timestamp = QDateTime::currentDateTime());
        void setCurrentTopicMetadata(const QString& author, const QDateTime &timestamp = QDateTime::currentDateTime());

#if HAVE_QCA2
        void setCipher(Konversation::Cipher* cipher);
        void clearCipher();
#endif

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        static QString authorPlaceholder();


    Q_SIGNALS:
        void currentTopicChanged(const QString& text);


    private:
        QList<Topic> m_topicList;
#if HAVE_QCA2
        Konversation::Cipher* m_cipher;
#endif

        Q_DISABLE_COPY(TopicHistoryModel)
};

#endif
