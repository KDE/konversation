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

#include <config-konversation.h>

#ifdef HAVE_NOTIFICATIONITEM
#include <knotificationitem-1/knotificationitem.h>

using namespace Experimental;
#else
#include <ksystemtrayicon.h>

class QTimer;
#endif

namespace Konversation
{
#ifdef HAVE_NOTIFICATIONITEM
    class TrayIcon : public KNotificationItem
#else
    class TrayIcon : public KSystemTrayIcon
#endif
    {
        Q_OBJECT
        
        public:
            explicit TrayIcon(QWidget* parent = 0);
            ~TrayIcon();

            void setVisibility(bool visible);
            bool notificationEnabled() { return m_notificationEnabled; }

        public slots:
            void startNotification();
            void endNotification();
            void setNotificationEnabled(bool notify) { m_notificationEnabled = notify; }
            void updateAppearance();

        #ifndef HAVE_NOTIFICATIONITEM
        protected slots:
            void blinkTimeout();
        #endif

        private:
            bool m_notificationEnabled;

        #ifdef HAVE_NOTIFICATIONITEM
            QString m_nomessagePix;
            QString m_messagePix;
        #else
            QTimer* m_blinkTimer;
            bool m_blinkOn;

            QIcon m_nomessagePix;
            QIcon m_messagePix;
        #endif
    };
}

#endif
