/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2022 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#include "launcherentryhandler.h"

#include "konversation_log.h"
#include "preferences.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusPendingReply>
#include <QDBusInterface>
#include <QDBusMessage>

namespace Konversation
{

LauncherEntryHandler::LauncherEntryHandler(QObject* parent)
    : QObject(parent)
    , m_unityServiceWatcher(new QDBusServiceWatcher(this))
    , m_enabled(Preferences::self()->showLauncherEntryCount())
{
    m_unityServiceWatcher->setConnection(QDBusConnection::sessionBus());
    m_unityServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration);
    m_unityServiceWatcher->addWatchedService(QStringLiteral("com.canonical.Unity"));
    connect(m_unityServiceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &LauncherEntryHandler::handleServiceRegistered);
    connect(m_unityServiceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &LauncherEntryHandler::handleServiceUnregistered);
    QDBusPendingCall listNamesCall = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    auto callWatcher = new QDBusPendingCallWatcher(listNamesCall, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished,
            this, &LauncherEntryHandler::handleListNamesReply);

    connect(Preferences::self(), &Preferences::showLauncherEntryCountChanged, this, &LauncherEntryHandler::setEnabled);
}

LauncherEntryHandler::~LauncherEntryHandler() = default;

void LauncherEntryHandler::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;

    if (m_launcherEntryConnected) {
        emitNumber(m_enabled ? m_number : 0);
    }
}

void LauncherEntryHandler::handleListNamesReply(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError()) {
        qCWarning(KONVERSATION_LOG) << "D-Bus error on ListNames call:" << reply.error().message();
        return;
    }

    const QStringList &services = reply.value();

    m_launcherEntryConnected = services.contains(QStringLiteral("com.canonical.Unity"));
    if (m_launcherEntryConnected && m_enabled) {
        emitNumber(m_number);
    }
}

void LauncherEntryHandler::handleServiceRegistered()
{
    m_launcherEntryConnected = true;
    if (m_enabled) {
        emitNumber(m_number);
    }
}

void LauncherEntryHandler::handleServiceUnregistered()
{
    m_launcherEntryConnected = false;
}


void LauncherEntryHandler::updateNumber(int number)
{
    if (number == m_number) {
        return;
    }

    m_number = number;

    if (m_enabled && m_launcherEntryConnected) {
        emitNumber(m_number);
    }
}

void LauncherEntryHandler::emitNumber(int number)
{
    const QVariantMap signalMessageProperties{
        {QStringLiteral("count-visible"), number > 0},
        {QStringLiteral("count"), number},
    };

    QDBusMessage signalMessage = QDBusMessage::createSignal(QStringLiteral("/com/canonical/Unity/LauncherEntry"),
                                                            QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                                            QStringLiteral("Update"));
    const QString launcherId = qApp->desktopFileName() + QLatin1String(".desktop");
    signalMessage.setArguments({launcherId, signalMessageProperties});
    QDBusConnection::sessionBus().send(signalMessage);
}

}

#include "moc_launcherentryhandler.cpp"
