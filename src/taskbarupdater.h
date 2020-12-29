/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2020-2021 Peter Simonsson <peter.simonsson@gmail.com>
*/

#ifndef TASKBARUPDATER_H
#define TASKBARUPDATER_H

#include <QObject>

class QDBusServiceWatcher;

/**
 * Uses the com.canonical.Unity DBUS service to set the number of unread
 * messages in the taskbar.
 */
class TaskbarUpdater : public QObject
{
    Q_OBJECT
    public:
        TaskbarUpdater(QObject *parent);

    public Q_SLOTS:
        void increaseUnread();
        void decreaseUnread(uint count);

    private Q_SLOTS:
        void updateUnread();

    private:
        void initUnityService();

        QDBusServiceWatcher *m_unityServiceWatcher;
        bool m_unityServiceAvailable;

        uint m_unread;
};

#endif // TASKBARUPDATER_H
