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

#define QT3_SUPPORT //TODO remove when porting away from K3ListView

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
            void refreshTopicHistory();
            void refreshAllowedChannelModes();
            void refreshModes();
            void refreshEnableModes(bool forceUpdate = false);
            void toggleAdvancedModes();

            void refreshBanList();
            void addBan(const QString& newban);
            void addBanClicked();
            void removeBan(const QString& ban);
            void removeBanClicked();
            void banEdited(Q3ListViewItem *edited);

            void changeOptions();


        protected slots:
            void topicHistoryItemClicked(const QItemSelection& selection);
            void topicBeingEdited();

            void cancelClicked();
            void okClicked();


        protected:
            bool m_editingTopic;
            Q3ListViewItem *m_NewBan;


        private:
            Ui::ChannelOptionsUI m_ui;
            Channel *m_channel;

            TopicListModel* m_topicModel;
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

    struct TopicItem
    {
        QString author;
        QString topic;
        QDateTime timestamp;
    };

    class TopicListModel : public QAbstractListModel
    {
        Q_OBJECT
        public:
            TopicListModel(QObject* parent);

            void setTopicList(const QList<TopicItem>& list);

            int columnCount(const QModelIndex& parent = QModelIndex()) const;
            int rowCount(const QModelIndex& parent = QModelIndex()) const;

            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
            QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

        private:
            QList<TopicItem> m_topicList;
    };
}
#endif
