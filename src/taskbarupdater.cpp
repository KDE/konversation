/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2018-2020 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2020-2021 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "taskbarupdater.h"
#include "konversation_log.h"
#include "preferences.h"

#include <QApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusConnectionInterface>

TaskbarUpdater::TaskbarUpdater(QObject *parent) :
    QObject(parent),
    m_unityServiceWatcher(new QDBusServiceWatcher(this)),
    m_unityServiceAvailable(false),
    m_unread(0)
{
    initUnityService();
}

void TaskbarUpdater::updateUnread()
{
    if(!m_unityServiceAvailable)
        return;

    const QString launcherId = qApp->desktopFileName() + QLatin1String(".desktop");
    bool showCount = m_unread > 0 && Preferences::self()->showUnreadOnTaskbar();
    const QVariantMap properties{
        {QStringLiteral("count-visible"), showCount},
        {QStringLiteral("count"), m_unread}
    };

    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/kde/konversation/UnityLauncher"),
                                                      QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                                      QStringLiteral("Update"));
    message.setArguments({launcherId, properties});
    QDBusConnection::sessionBus().send(message);
}

void TaskbarUpdater::initUnityService()
{
    m_unityServiceWatcher->setConnection(QDBusConnection::sessionBus());
    m_unityServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration);
    m_unityServiceWatcher->addWatchedService(QStringLiteral("com.canonical.Unity"));
    connect(m_unityServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &service) {
        Q_UNUSED(service)
        m_unityServiceAvailable = true;
        updateUnread();
    });

    connect(m_unityServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        Q_UNUSED(service)
        m_unityServiceAvailable = false;
    });

    // QDBusConnectionInterface::isServiceRegistered blocks
    QDBusPendingCall listNamesCall = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    auto callWatcher = new QDBusPendingCallWatcher(listNamesCall, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QStringList> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qCWarning(KONVERSATION_LOG) << "DBus reported an error " << reply.error().message();
            return;
        }

        const QStringList &services = reply.value();

        m_unityServiceAvailable = services.contains(QLatin1String("com.canonical.Unity"));
        if (m_unityServiceAvailable) {
            updateUnread();
        }
    });
}

void TaskbarUpdater::increaseUnread()
{
    m_unread++;
    updateUnread();
}

void TaskbarUpdater::decreaseUnread(uint count)
{
    m_unread -= count;
    updateUnread();
}
