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

#include <qptrlist.h>
#include <qpixmap.h>

#include <ksystemtray.h>

class QTimer;
class Server;

namespace Konversation
{

    class TrayIcon : public KSystemTray
    {
        Q_OBJECT
            public:
            TrayIcon(QWidget* parent = 0);
            ~TrayIcon();

            bool notificationEnabled() { return m_notificationEnabled; }

        public slots:
            void startNotification();
            void endNotification();
            void setNotificationEnabled(bool notify) { m_notificationEnabled = notify; }

        protected slots:
            void blinkTimeout();

        protected:
            void mousePressEvent(QMouseEvent *e);

        private:
            QTimer* m_blinkTimer;
            bool m_blinkOn;

            bool m_notificationEnabled;

            QPixmap m_nomessagePix;
            QPixmap m_messagePix;
    };

}
#endif
