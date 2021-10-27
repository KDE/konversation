/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONSERVERLISTDIALOG_H
#define KONVERSATIONSERVERLISTDIALOG_H

#include "common.h"
#include "servergroupsettings.h"
#include "serverlistview.h"
#include "ui_serverlistdialogui.h"
#include <QDialog>

class ConnectionSettings;
class QTreeWidgetItem;

namespace Konversation
{
    class ServerListItem : public QTreeWidgetItem
    {
        public:
            ServerListItem(QTreeWidget* tree, const QStringList & strings);
            ServerListItem(QTreeWidgetItem* parent, const QStringList & strings);
            ~ServerListItem() override = default;
            bool operator<(const QTreeWidgetItem &other) const override;
            enum DataRole
            {
                SortIndex = Qt::UserRole + 2
            };
    private:
            Q_DISABLE_COPY(ServerListItem)
    };

    class ServerListDialog : public QDialog, private Ui::ServerListDialogUI
    {
        Q_OBJECT

        public:
            explicit ServerListDialog(const QString& title, QWidget *parent = nullptr);
            ~ServerListDialog() override;
            enum DataRole
            {
                ServerGroupId = Qt::UserRole + 1,
                SortIndex = Qt::UserRole + 2,
                IsServer = Qt::UserRole + 3,
                ServerId = Qt::UserRole + 4
            };

        public Q_SLOTS:
            void updateServerList();

        Q_SIGNALS:
            void connectTo(Konversation::ConnectionFlag flag, int serverGroupId);
            void connectTo(Konversation::ConnectionFlag flag, ConnectionSettings connectionSettings);
            void serverGroupsChanged(const Konversation::ServerGroupSettingsPtr serverGroup = Konversation::ServerGroupSettingsPtr());

        private Q_SLOTS:
            void slotOk();
            void slotAdd();
            void slotEdit();
            void slotDelete();

            void slotSetGroupExpanded(QTreeWidgetItem* item);
            void slotSetGroupCollapsed(QTreeWidgetItem* item);

            void slotAboutToMove();
            void slotMoved();

            void updateButtons();

            void setShowAtStartup(bool show);

        private:
            QTreeWidgetItem* insertServerGroup(ServerGroupSettingsPtr serverGroup);
            void addServerGroup(ServerGroupSettingsPtr serverGroup);

            int selectedChildrenCount(QTreeWidgetItem* item);

        private:
            int m_lastSortColumn;
            Qt::SortOrder m_lastSortOrder;

            bool m_selectedItem;
            int m_selectedServerGroupId;
            ServerSettings m_selectedServer;
            QTreeWidgetItem* m_selectedItemPtr;

            Q_DISABLE_COPY(ServerListDialog)
    };
}
#endif
