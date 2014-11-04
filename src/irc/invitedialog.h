// Copyright 2009  Peter Simonsson <peter.simonsson@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License or (at your option) version 3 or any later version
// accepted by the membership of KDE e.V. (or its successor approved
// by the membership of KDE e.V.), which shall act as a proxy
// defined in Section 14 of version 3 of the license.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef KONVERSATION_INVITEDIALOG_H
#define KONVERSATION_INVITEDIALOG_H

#include "ui_invitedialog.h"

#include <QDialog>
#include <QDialogButtonBox>

#include <QAbstractListModel>
#include <QMap>

class InviteChannelListModel : public QAbstractListModel
{
    Q_OBJECT
    public:
        struct ChannelItem
        {
            QString channel;
            QString nicknames;
            Qt::CheckState checkState;
        };

        explicit InviteChannelListModel(QObject* parent);

        void addInvite(const QString& nickname, const QString& channel);
        QString selectedChannels() const;

        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const;

        virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    private:
        QMap<QString, ChannelItem> m_channelMap;
};

class InviteDialog : public QDialog, protected Ui::InviteDialog
{
    Q_OBJECT
    public:
        explicit InviteDialog(QWidget* parent);

        void addInvite(const QString& nickname, const QString& channel);

        static bool shouldBeShown(QDialogButtonBox::StandardButton& buttonCode);

    protected Q_SLOTS:
        void slotOk();

    private:
        InviteChannelListModel* m_channelModel;

    Q_SIGNALS:
        void joinChannelsRequested(const QString& channels);
};

#endif //KONVERSATION_INVITEDIALOG_H
