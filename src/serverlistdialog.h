/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONSERVERLISTDIALOG_H
#define KONVERSATIONSERVERLISTDIALOG_H

#include <kdialogbase.h>
#include <klistview.h>

#include "servergroupsettings.h"

class Preferences;
class QPushButton;
class QStringList;

namespace Konversation
{

    class ServerListItem : public KListViewItem
    {
        public:
            ServerListItem(QListViewItem* parent, int serverId, const QString& serverGroup,
                const QString& identity, const QString& channels, bool autoConnect);
            ServerListItem(QListView* parent, int serverId, const QString& serverGroup,
                const QString& identity, const QString& channels, bool autoConnect);

            int serverId() const { return m_serverId; }
            bool autoConnect() const { return m_autoConnect; }
            void setAutoConnect(bool ac);
            //      virtual void paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align);

            virtual int rtti() const { return 10001; }

        protected:
            virtual void activate();

        private:
            int m_serverId;
            bool m_autoConnect;
    };

    class ServerListDialog : public KDialogBase
    {
        Q_OBJECT
            public:
            ServerListDialog(QWidget *parent = 0, const char *name = 0);
            ~ServerListDialog();

        public slots:
            void updateServerGroupList();

            signals:
            void connectToServer(int serverId);

        protected slots:
            virtual void slotOk();
            virtual void slotApply();

            void slotAdd();
            void slotEdit();
            void slotDelete();

            void updateButtons();

        protected:
            QListViewItem* findBranch(QString name, bool generate = true);
            QStringList createGroupList();
                                                  /// Adds a list item to the list view
            QListViewItem* addListItem(ServerGroupSettingsPtr serverGroup);

            void addServerGroup(ServerGroupSettingsPtr serverGroup);

        private:
            KListView* m_serverList;
            QPushButton* m_addButton;
            QPushButton* m_editButton;
            QPushButton* m_delButton;
    };
}
#endif
