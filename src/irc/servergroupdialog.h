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

#ifndef KONVERSATIONSERVERGROUPDIALOG_H
#define KONVERSATIONSERVERGROUPDIALOG_H

#include "servergroupsettings.h"

#include <kdialog.h>


namespace Ui
{
class ServerGroupDialogUI;
}

namespace Konversation
{

    class ServerGroupDialog : public KDialog
    {
        Q_OBJECT
        public:
            explicit ServerGroupDialog(const QString& title, QWidget* parent = 0);
            ~ServerGroupDialog();

            void setServerGroupSettings(ServerGroupSettingsPtr settings);
            ServerGroupSettingsPtr serverGroupSettings();

            ServerSettings editedServer();

            int execAndEditServer(ServerSettings server);

            bool identitiesNeedsUpdate() const { return m_identitiesNeedsUpdate; }

        protected slots:
            virtual void slotOk();

            void addServer();
            void editServer();
            void editServer(ServerSettings server);
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
            bool m_configBacked;
            int m_id;
            int m_sortIndex;

            bool m_identitiesNeedsUpdate;

            bool m_editedServer;
            int m_editedServerIndex;
            ServerList m_serverList;
            ChannelList m_channelList;
            ChannelList m_channelHistory;
    };

}
#endif
