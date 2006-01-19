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

#include <kdialogbase.h>

#include "servergroupsettings.h"

class QLineEdit;
class QComboBox;
class QListBox;
class QCheckBox;
class QToolButton;

namespace Konversation
{

    class ServerGroupDialog : public KDialogBase
    {
        Q_OBJECT
            public:
            ServerGroupDialog(const QString& title, QWidget* parent = 0, const char* name = 0);
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
            QLineEdit* m_nameEdit;
            QComboBox* m_identityCBox;
            QLineEdit* m_commandEdit;
            QListBox* m_serverLBox;
            QListBox* m_channelLBox;
            QCheckBox* m_autoConnectCBox;
            ServerList m_serverList;
            ChannelList m_channelList;
            ChannelList m_channelHistory;
            QToolButton* m_upServerBtn;
            QToolButton* m_downServerBtn;
            QToolButton* m_upChannelBtn;
            QToolButton* m_downChannelBtn;
            bool m_expanded;

            int m_id;
            int m_sortIndex;

            bool m_identitiesNeedsUpdate;

            bool m_editedServer;
            uint m_editedServerIndex;
    };

}
#endif
