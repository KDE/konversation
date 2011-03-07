/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class handles the system tray icon
  begin:     Sun Nov 9 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#ifndef TRAYICON_H
#define TRAYICON_H

#include <KStatusNotifierItem>

namespace Konversation
{
    class TrayIcon : public KStatusNotifierItem
    {
        Q_OBJECT

        public:
            explicit TrayIcon(QWidget* parent = 0);
            ~TrayIcon();

            bool notificationEnabled() { return m_notificationEnabled; }

            void restore();

        public slots:
            void startNotification();
            void endNotification();
            void setNotificationEnabled(bool notify) { m_notificationEnabled = notify; }
            void updateAppearance();

        private:
            bool m_notificationEnabled;

            QString m_nomessagePix;
            QString m_messagePix;
    };
}

#endif
