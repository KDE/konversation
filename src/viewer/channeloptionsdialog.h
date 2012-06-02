/*
  This program is free software; you can redistribute it and/or modifydvancedModes
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/

#ifndef KONVERSATIONCHANNELOPTIONSDIALOG_H
#define KONVERSATIONCHANNELOPTIONSDIALOG_H

#include "ui_channeloptionsui.h"
#include "channel.h"

#include <QStringList>
#include <QAbstractListModel>

#include <kdialog.h>


namespace Konversation
{
    class TopicListModel;

    class ChannelOptionsDialog : public KDialog
    {
        Q_OBJECT
            public:
            explicit ChannelOptionsDialog(Channel *channel);
            ~ChannelOptionsDialog();

            QString topic();
            QStringList modes();

            /**
             * Return the whatsThis string for a mode-change button.
             * These strings are shared between the Channel Options Dialog
             * and the Channel window.
             */
            static QString whatsThisForMode(char mode);

        public slots:
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

        protected slots:
            void topicHistoryItemClicked(const QItemSelection& selection);
            void topicBeingEdited(bool edited);

        protected:
            void showEvent(QShowEvent* event);
            void hideEvent(QHideEvent* event);

            bool m_editingTopic;
            bool m_isAnyTypeOfOp;

        private:
            Ui::ChannelOptionsUI m_ui;
            Channel *m_channel;
    };

    class BanListViewItem : public QTreeWidgetItem
    {
        public:
            explicit BanListViewItem( QTreeWidget *parent );
            BanListViewItem(QTreeWidget *parent, const QString& label1, const QString& label2 = QString(), uint timestamp = 0);

            bool operator<(const QTreeWidgetItem &item) const;

        protected:
            QDateTime m_timestamp;
    };
}
#endif
