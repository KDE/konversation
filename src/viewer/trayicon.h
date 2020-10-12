/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Peter Simonsson <psn@linux.se>
*/

#ifndef TRAYICON_H
#define TRAYICON_H

#include <KStatusNotifierItem>

namespace Konversation
{
    /**
     * This class handles the system tray icon
     */
    class TrayIcon : public KStatusNotifierItem
    {
        Q_OBJECT

        public:
            explicit TrayIcon(QWidget* parent = nullptr);
            ~TrayIcon();

            bool notificationEnabled() { return m_notificationEnabled; }

            void restore();

        public Q_SLOTS:
            void startNotification();
            void endNotification();
            void setNotificationEnabled(bool notify) { m_notificationEnabled = notify; }
            void setAway(bool away);
            void updateAppearance();

        private:
            bool m_notificationEnabled;
            bool m_away;
    };
}

#endif
