/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2022 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#ifndef LAUNCHERENTRYHANDLER_H
#define LAUNCHERENTRYHANDLER_H

#include <QObject>

class QDBusServiceWatcher;
class QDBusPendingCallWatcher;

namespace Konversation
{

class LauncherEntryHandler : public QObject
{
    Q_OBJECT

public:
    explicit LauncherEntryHandler(QObject* parent = nullptr);
    ~LauncherEntryHandler() override;

public:
    void updateNumber(int number);
    void setEnabled(bool enabled);

private:
    void handleListNamesReply(QDBusPendingCallWatcher* watcher);
    void handleServiceRegistered();
    void handleServiceUnregistered();

    void emitNumber(int number);

private:
    QDBusServiceWatcher* m_unityServiceWatcher;
    int m_number = 0;
    bool m_launcherEntryConnected = false;
    bool m_enabled = false;
};

}

#endif
