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

#include "channel.h"

#include <qstringlist.h>

#include <kdialog.h>

#include "ui_channeloptionsui.h"

namespace Konversation
{
    class ChannelOptionsUI;

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
            void refreshTopicHistory();
            void refreshAllowedChannelModes();
            void refreshModes();
            void refreshEnableModes();
            void toggleAdvancedModes();

            void refreshBanList();
            void addBan(const QString& newban);
            void addBanClicked();
            void removeBan(const QString& ban);
            void removeBanClicked();
            void banEdited(Q3ListViewItem *edited);

            void changeOptions();


        protected slots:
            void topicHistoryItemClicked(QTreeWidgetItem* item);
            void topicBeingEdited();

            void cancelClicked();
            void okClicked();


        protected:
            bool m_editingTopic;
            Q3ListViewItem *m_NewBan;


        private:
            Ui::ChannelOptionsUI m_ui;
            Channel *m_channel;
    };


    // This is needed to overcome two deficiencies in K3ListViewItem
    // First there is no signal emitted when a rename is canceled
    // Second there is no way to get the old value of an item after a rename
    class BanListViewItem : public K3ListViewItem
    {
        public:
            explicit BanListViewItem( Q3ListView *parent );
            BanListViewItem(Q3ListView *parent, bool isNew);
            BanListViewItem(Q3ListView *parent, const QString& label1, const QString& label2 = QString(), uint timestamp = 0);
            BanListViewItem (Q3ListView *parent, bool isNew, const QString& label1, const QString& label2 = QString(), uint timestamp = 0);

            QString getOldValue() { return m_oldValue; }
            QDateTime timestamp() { return m_timestamp; }

            virtual QString text(int column) const;
            virtual int compare(Q3ListViewItem *i, int col, bool ascending) const;
            virtual void startRename(int col);


        protected:
            virtual void cancelRename(int col);

            QString m_oldValue;
            bool m_isNewBan;
            QDateTime m_timestamp;
    };

}
#endif
