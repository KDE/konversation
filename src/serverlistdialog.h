/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#ifndef KONVERSATIONSERVERLISTDIALOG_H
#define KONVERSATIONSERVERLISTDIALOG_H

#include <kdialogbase.h>

#include <serverlistview.h>
#include "servergroupsettings.h"

class Preferences;
class QPushButton;
class QStringList;

namespace Konversation
{

    class ServerListItem : public KListViewItem
    {
        public:
            ServerListItem(KListView* parent, int serverGroupId, int sortIndex,
                const QString& serverGroup, const QString& identity, const QString& channels);
            ServerListItem(QListViewItem* parent, int serverGroupId, int sortIndex, 
                const QString& name, const ServerSettings& server);

            int serverGroupId() const { return m_serverGroupId; }

            void setSortIndex(int id) { m_sortIndex = id; }
            int sortIndex() const { return m_sortIndex; }

            ServerSettings server() const { return m_server; }
            QString name() const { return m_name; }
            bool isServer() const { return m_isServer; }

            int compare(QListViewItem *i, int col, bool ascending) const;

        private:
            int m_serverGroupId;
            int m_sortIndex;
            QString m_name;
            ServerSettings m_server;
            bool m_isServer;
    };

    class ServerListDialog : public KDialogBase
    {
        Q_OBJECT
            public:
            ServerListDialog(QWidget *parent = 0, const char *name = 0);
            ~ServerListDialog();

        public slots:
            void updateServerList();

        signals:
            void connectToServer(int serverId);
            void connectToServer(int serverId, Konversation::ServerSettings quickServer);
            void serverGroupsChanged();

        protected slots:
            virtual void slotOk();

            void slotAdd();
            void slotEdit();
            void slotDelete();

            void slotSetGroupExpanded(QListViewItem* item);
            void slotSetGroupCollapsed(QListViewItem* item);

            void slotAboutToMove();
            void slotMoved();

            void updateButtons();

        protected:
            QListViewItem* insertServerGroup(ServerGroupSettingsPtr serverGroup);
            void addServerGroup(ServerGroupSettingsPtr serverGroup);

        private:
            ServerListView* m_serverList;
            QPushButton* m_addButton;
            QPushButton* m_editButton;
            QPushButton* m_delButton;

            bool m_editedItem;
            int m_editedServerGroupId;
            ServerSettings m_editedServer;
            QListViewItem* m_editedItemPtr;

            int m_lastSortColumn;
            SortOrder m_lastSortOrder;
    };
}
#endif
