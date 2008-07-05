/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONSERVERLISTDIALOG_H
#define KONVERSATIONSERVERLISTDIALOG_H

#include "serverlistview.h"
#include "common.h"
#include "servergroupsettings.h"

#include <kdialogbase.h>

class ConnectionSettings;
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

            int selectedChildrenCount();

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
            explicit ServerListDialog(QWidget *parent = 0, const char *name = 0);
            ~ServerListDialog();

        public slots:
            void updateServerList();

        signals:
            void connectTo(Konversation::ConnectionFlag flag, int serverGroupId);
            void connectTo(Konversation::ConnectionFlag flag, ConnectionSettings& connectionSettings);
            void serverGroupsChanged(const Konversation::ServerGroupSettings* serverGroup = 0);
            void serverGroupNameChanged(const QString& newName, const QString& oldName);

        protected slots:
            virtual void slotOk();
            void slotClose();
            void slotAdd();
            void slotEdit();
            void slotDelete();

            void slotSetGroupExpanded(QListViewItem* item);
            void slotSetGroupCollapsed(QListViewItem* item);

            void slotAboutToMove();
            void slotMoved();

            void updateButtons();

            void setShowAtStartup(bool show);

        protected:
            QListViewItem* insertServerGroup(ServerGroupSettingsPtr serverGroup);
            void addServerGroup(ServerGroupSettingsPtr serverGroup);

        private:
            ServerListView* m_serverList;
            QPushButton* m_addButton;
            QPushButton* m_editButton;
            QPushButton* m_delButton;

            bool m_selectedItem;
            int m_selectedServerGroupId;
            ServerSettings m_selectedServer;
            QListViewItem* m_selectedItemPtr;

            int m_lastSortColumn;
            SortOrder m_lastSortOrder;
    };
}
#endif
