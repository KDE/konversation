/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2012 Eike Hein <hein@kde.org>
*/

#include "topichistorymodel.h"
#ifdef HAVE_QCA2
#include "cipher.h"
#endif

#include <KCategorizedSortFilterProxyModel>
#include <QLocale>


TopicHistoryModel::TopicHistoryModel(QObject* parent) : QAbstractListModel(parent)
{
#ifdef HAVE_QCA2
    m_cipher = 0;
#endif
}

TopicHistoryModel::~TopicHistoryModel()
{
}

QString TopicHistoryModel::authorPlaceholder()
{
    return i18n("Unknown");
}


QString TopicHistoryModel::currentTopic()
{
    if (m_topicList.isEmpty())
        return QString();

    return data(index(m_topicList.count() - 1, 0)).toString();
}

void TopicHistoryModel::appendTopic(const QString& text, const QString& author, QDateTime timestamp)
{
    int newIndex = m_topicList.count();

    if (!m_topicList.isEmpty())
    {
        const Topic& current = m_topicList.last();

        if (text == current.text && author == current.author)
        {
            setCurrentTopicMetadata(author);
            return;
        }
    }

    beginInsertRows(QModelIndex(), newIndex, newIndex);

    Topic topic;
    topic.text = text;
    topic.author = author.section(QLatin1Char('!'), 0, 0);
    topic.timestamp = timestamp;

    m_topicList.append(topic);

    endInsertRows();

    currentTopicChanged(text);
}

void TopicHistoryModel::setCurrentTopicMetadata(const QString& author, QDateTime timestamp)
{
    if (m_topicList.isEmpty())
        return;

    Topic currentTopic = m_topicList.last();
    int row = m_topicList.count() - 1;

    currentTopic.author = author.section(QLatin1Char('!'), 0, 0);
    currentTopic.timestamp = timestamp;

    if (m_topicList.count() >= 2 && m_topicList.at(row - 1) == currentTopic)
    {
        beginRemoveRows(QModelIndex(), row, row);
        m_topicList.removeLast();
        endRemoveRows();
    }
    else
    {
        m_topicList[row] = currentTopic;
        emit dataChanged(index(row, 1), index(row, 2));
    }
}

#ifdef HAVE_QCA2
void TopicHistoryModel::setCipher(Konversation::Cipher* cipher)
{
    emit layoutAboutToBeChanged();
    beginResetModel();

    m_cipher = cipher;

    endResetModel();
    emit layoutChanged();
}

void TopicHistoryModel::clearCipher()
{
    emit layoutAboutToBeChanged();
    beginResetModel();

    m_cipher = 0;

    endResetModel();
    emit layoutChanged();
}
#endif

QVariant TopicHistoryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_topicList.count())
        return QVariant();

    const Topic& topic = m_topicList[index.row()];

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0:
#ifdef HAVE_QCA2
                if (m_cipher)
                {
                     QByteArray cipherText = m_cipher->decryptTopic(topic.text.toUtf8());

                     // HACK: The horrible undocumented magic values here are straight out of
                     // Channel, where this code once lived.
                     return QString::fromUtf8(cipherText.data() + 2, cipherText.length() - 2);
                }
                else
#endif
                    return topic.text;

                break;
            case 1:
                if (!topic.author.isEmpty())
                    return topic.author;
                else
                    return authorPlaceholder();

                break;
            case 2:
                return QLocale().toString(topic.timestamp, QLocale::ShortFormat);
                break;
        }
    }
    else if (role == Qt::UserRole && index.column() == 2)
        return QVariant(topic.timestamp);

    return QVariant();
}


QVariant TopicHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0:
                return i18n("Text");
                break;
            case 1:
                return i18n("Author");
                break;
            case 2:
                return i18n("Time");
                break;
        }
    }

    return QVariant();
}

int TopicHistoryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return 3;
}

int TopicHistoryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return m_topicList.count();
}
