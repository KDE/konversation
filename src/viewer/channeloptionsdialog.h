/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONCHANNELOPTIONSDIALOG_H
#define KONVERSATIONCHANNELOPTIONSDIALOG_H

#include "ui_channeloptionsui.h"
#include "channel.h"

#include <QStringList>

#include <QDialog>

class QTimer;

namespace Konversation
{

    class ChannelOptionsDialog : public QDialog
    {
        Q_OBJECT
            public:
            explicit ChannelOptionsDialog(Channel *channel);
            ~ChannelOptionsDialog() override;

            QString topic() const;
            QStringList modes() const;

            /**
             * Return the whatsThis string for a mode-change button.
             * These strings are shared between the Channel Options Dialog
             * and the Channel window.
             */
            static QString whatsThisForMode(char mode);

        public Q_SLOTS:
            void refreshAllowedChannelModes();
            void refreshModes();
            void refreshEnableModes(bool forceUpdate = false);
            void toggleAdvancedModes();

            void refreshBanList();
            void addBan(const QString& newban);
            void addBanClicked();
            void updateBanClicked();
            void removeBan(const QString& ban);
            void removeBanClicked();
            void banSelectionChanged();
            void hostmaskChanged(const QString&);

            void changeOptions();

        protected:
            void showEvent(QShowEvent* event) override;
            void hideEvent(QHideEvent* event) override;

        private Q_SLOTS:
            void topicHistoryItemClicked(const QItemSelection& selection);
            void topicBeingEdited(bool edited);

            void startHistorySearchTimer(const QString &filter);
            void updateHistoryFilter();

        private:
            bool m_editingTopic;
            bool m_isAnyTypeOfOp;

            Ui::ChannelOptionsUI m_ui;
            Channel *m_channel;

            QTimer* m_historySearchTimer;

            Q_DISABLE_COPY(ChannelOptionsDialog)
    };

    class BanListViewItem : public QTreeWidgetItem
    {
        public:
            explicit BanListViewItem( QTreeWidget *parent );
            BanListViewItem(QTreeWidget *parent, const QString& label1, const QString& label2 = QString(), uint timestamp = 0);
            ~BanListViewItem() override = default;

            bool operator<(const QTreeWidgetItem &item) const override;

        private:
            QDateTime m_timestamp;

            Q_DISABLE_COPY(BanListViewItem)
    };
}
#endif
