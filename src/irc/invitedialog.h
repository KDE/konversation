/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

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

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

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

    Q_SIGNALS:
        void joinChannelsRequested(const QString& channels);

    private Q_SLOTS:
        void slotOk();

    private:
        InviteChannelListModel* m_channelModel;
};

#endif //KONVERSATION_INVITEDIALOG_H
