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
            void refreshTopicHistory();
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
            void hostmaskChanged(QString);

            void changeOptions();

        protected slots:
            void topicHistoryItemClicked(const QItemSelection& selection);
            void topicBeingEdited(bool edited);

        protected:
            bool m_editingTopic;
            bool m_isAnyTypeOfOp;

        private:
            Ui::ChannelOptionsUI m_ui;
            Channel *m_channel;

            TopicListModel* m_topicModel;
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

            QList<TopicItem> topicList() const;
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
