/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONSERVERGROUPDIALOG_H
#define KONVERSATIONSERVERGROUPDIALOG_H

#include "servergroupsettings.h"

#include <QDialog>
class QPushButton;

namespace Ui
{
class ServerGroupDialogUI;
class ChannelDialogUI;
class ServerDialogUI;
}

namespace Konversation
{
    class ServerGroupDialog : public QDialog
    {
        Q_OBJECT
        public:
            explicit ServerGroupDialog(const QString& title, QWidget* parent = nullptr);
            ~ServerGroupDialog() override;

            void setServerGroupSettings(ServerGroupSettingsPtr settings);
            ServerGroupSettingsPtr serverGroupSettings();

            ServerSettings editedServer() const;

            int execAndEditServer(const ServerSettings &server);

            bool identitiesNeedsUpdate() const { return m_identitiesNeedsUpdate; }

        public Q_SLOTS:
            void accept() override;

        private Q_SLOTS:
            void addServer();
            void editServer();
            void editServer(const ServerSettings &server);
            void deleteServer();
            void updateServerArrows();
            void moveServerUp();
            void moveServerDown();

            void addChannel();
            void editChannel();
            void deleteChannel();
            void updateChannelArrows();
            void moveChannelUp();
            void moveChannelDown();

            void editIdentity();

        private:
            Ui::ServerGroupDialogUI* m_mainWidget;
            bool m_expanded;
            bool m_enableNotifications;
            int m_id;
            int m_sortIndex;

            bool m_identitiesNeedsUpdate;

            bool m_editedServer;
            int m_editedServerIndex;
            ServerList m_serverList;
            ChannelList m_channelList;
            ChannelList m_channelHistory;

            Q_DISABLE_COPY(ServerGroupDialog)
    };

    class ServerDialog : public QDialog
    {
        Q_OBJECT
        public:
            explicit ServerDialog(const QString& title, QWidget *parent = nullptr);
            ~ServerDialog() override;

            void setServerSettings(const ServerSettings& server);
            ServerSettings serverSettings() const;

        private Q_SLOTS:
            void slotOk();
            void slotServerNameChanged( const QString& );

        private:
            Ui::ServerDialogUI* m_mainWidget;
            QPushButton *m_okButton;

            Q_DISABLE_COPY(ServerDialog)
    };

    class ChannelSettings;
    class ChannelDialog : public QDialog
    {
        Q_OBJECT
        public:
            explicit ChannelDialog(const QString& title, QWidget *parent = nullptr);
            ~ChannelDialog() override;

            void setChannelSettings(const ChannelSettings& channel);
            ChannelSettings channelSettings() const;

        private Q_SLOTS:
            void slotOk();
            void slotServerNameChanged( const QString& );

        private:
            Ui::ChannelDialogUI* m_mainWidget;
            QPushButton *m_okButton;

            Q_DISABLE_COPY(ChannelDialog)
    };
}

#endif
