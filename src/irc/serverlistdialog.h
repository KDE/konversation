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

#include "common.h"
#include "servergroupsettings.h"
#include "serverlistview.h"
#include <kdialog.h>

class ConnectionSettings;
class QTreeWidgetItem;
class QCheckBox;
namespace Ui
{
class ServerListDialogUI;
}
namespace Konversation
{
    class ServerListItem : public QTreeWidgetItem
    {
        public:
            ServerListItem(QTreeWidget* tree, QStringList & strings);
            ServerListItem(QTreeWidgetItem* parent, QStringList & strings);
            bool operator<(const QTreeWidgetItem &other) const;
            enum DataRole
            {
                SortIndex = Qt::UserRole + 2
            };
    };

    class ServerListDialog : public KDialog
    {
        Q_OBJECT

        public:
            explicit ServerListDialog(const QString& title, QWidget *parent = 0);
            ~ServerListDialog();
            enum DataRole
            {
                ServerGroupId = Qt::UserRole + 1,
                SortIndex = Qt::UserRole + 2,
                IsServer = Qt::UserRole + 3,
                ServerId = Qt::UserRole + 4
            };

        public slots:
            void updateServerList();

        signals:
            void connectTo(Konversation::ConnectionFlag flag, int serverGroupId);
            void connectTo(Konversation::ConnectionFlag flag, ConnectionSettings& connectionSettings);
            void serverGroupsChanged(const Konversation::ServerGroupSettingsPtr serverGroup = Konversation::ServerGroupSettingsPtr());

        protected slots:
            virtual void slotOk();
            void slotClose();
            void slotAdd();
            void slotEdit();
            void slotDelete();

            void slotSetGroupExpanded(QTreeWidgetItem* item);
            void slotSetGroupCollapsed(QTreeWidgetItem* item);

            void slotAboutToMove();
            void slotMoved();

            void updateButtons();

            void setShowAtStartup(bool show);

        protected:
            QTreeWidgetItem* insertServerGroup(ServerGroupSettingsPtr serverGroup);
            void addServerGroup(ServerGroupSettingsPtr serverGroup);
            
            int m_lastSortColumn;
            Qt::SortOrder m_lastSortOrder;

        private:
            int selectedChildrenCount(QTreeWidgetItem* item);
            Ui::ServerListDialogUI* m_mainWidget;
            QPushButton* m_addButton;
            QPushButton* m_delButton;
            QPushButton* m_editButton;
            QCheckBox* m_showAtStartup;
            ServerListView* m_serverList;

            bool m_selectedItem;
            int m_selectedServerGroupId;
            ServerSettings m_selectedServer;
            QTreeWidgetItem* m_selectedItemPtr;

    };
}
#endif
