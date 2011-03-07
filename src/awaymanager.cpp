/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
  Copyright (C) 2008 Eike Hein <hein@kde.org>
  Copyright (C) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#include "awaymanager.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "preferences.h"

#include <KActionCollection>
//FIXME: Change to pretty <KIdleTime> include when KDE_IS_VERSION(4, 5, 0)
#include <kidletime.h>
#include <KToggleAction>

AwayManager::AwayManager(QObject* parent) : QObject(parent)
{
    m_connectionManager = static_cast<Application*>(kapp)->getConnectionManager();

    connect(KIdleTime::instance(), SIGNAL(resumingFromIdle()), this, SLOT(resumeFromIdle()));
    connect(KIdleTime::instance(), SIGNAL(timeoutReached(int)), this, SLOT(idleTimeoutReached(int)));

    // Catch the first "resume event" (= user input) so we correctly catch the first
    // resume event in case the user is already idle on startup).
    KIdleTime::instance()->catchNextResumeEvent();
}

AwayManager::~AwayManager()
{
}

int AwayManager::minutesToMilliseconds(int minutes)
{
    return minutes * 60 * 1000;
}

void AwayManager::identitiesChanged()
{
    QHash<int, int> newIdentityWithIdleTimeMapping;

    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();
        int identityId = identity->id();

        // Calculate the auto-away time in milliseconds.
        int identityIdleTime = minutesToMilliseconds(identity->getAwayInactivity());

        if (identity && identity->getAutomaticAway() && server->isConnected())
            newIdentityWithIdleTimeMapping[identityId] = identityIdleTime;
    }

    m_identitiesWithIdleTimesOnAutoAway = newIdentityWithIdleTimeMapping;

    identitiesOnAutoAwayChanged();
}

void AwayManager::identityOnline(int identityId)
{
    IdentityPtr identity = Preferences::identityById(identityId);

    if (identity && identity->getAutomaticAway() &&
        !m_identitiesWithIdleTimesOnAutoAway.contains(identityId))
    {
        m_identitiesWithIdleTimesOnAutoAway[identityId] = minutesToMilliseconds(identity->getAwayInactivity());

        // Notify the AwayManager implementation that a user which has auto-away enabled is not online.
        identityOnAutoAwayWentOnline(identityId);
    }
}

void AwayManager::identityOffline(int identityId)
{
    if (m_identitiesWithIdleTimesOnAutoAway.remove(identityId))
        // Notify the AwayManager implementation that a user which has auto-away enabled is now offline.
        identityOnAutoAwayWentOffline(identityId);
}

void AwayManager::implementManagedAway(int identityId)
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        if (server->getIdentity()->id() == identityId && server->isConnected() && !server->isAway())
            server->requestAway();
    }
}

void AwayManager::setManagedIdentitiesAway()
{
    QHash<int, int>::ConstIterator itr = m_identitiesWithIdleTimesOnAutoAway.constBegin();

    for (; itr != m_identitiesWithIdleTimesOnAutoAway.constEnd(); ++itr)
        implementManagedAway(itr.key());
}

void AwayManager::implementManagedUnaway(const QList<int>& identityList)
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();

        if (identityList.contains(identity->id()) && identity->getAutomaticUnaway()
            && server->isConnected() && server->isAway())
        {
            server->requestUnaway();
        }
    }
}

void AwayManager::setManagedIdentitiesUnaway()
{
    // Set the "not away" status for all identities which have
    // auto-away enabled.
    implementManagedUnaway(m_identitiesWithIdleTimesOnAutoAway.keys());
}

void AwayManager::requestAllAway(const QString& reason)
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
        if (server->isConnected())
            server->requestAway(reason);
}

void AwayManager::requestAllUnaway()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
        if (server->isConnected() && server->isAway())
            server->requestUnaway();
}

void AwayManager::setGlobalAway(bool away)
{
    if (away)
        requestAllAway();
    else
        requestAllUnaway();
}

void AwayManager::updateGlobalAwayAction(bool away)
{
    // Regardless of any implementation: If the given parameter indicates
    // that the user is not away we should simulate user activity to
    // ensure that the away-status of the user is really reset.
    if (!away)
        simulateUserActivity();

    Application* konvApp = static_cast<Application*>(kapp);
    KToggleAction* awayAction = qobject_cast<KToggleAction*>(konvApp->getMainWindow()->actionCollection()->action("toggle_away"));

    if (!awayAction)
        return;

    if (away)
    {
        const QList<Server*> serverList = m_connectionManager->getServerList();
        int awayCount = 0;

        foreach (Server* server, serverList)
        {
            if (server->isAway())
                awayCount++;
        }

        if (awayCount == serverList.count())
        {
            awayAction->setChecked(true);
            awayAction->setIcon(KIcon("im-user-away"));
        }
    }
    else
    {
        awayAction->setChecked(false);
        awayAction->setIcon(KIcon("im-user"));
    }
}

int AwayManager::calculateRemainingTime(int identityId)
{
    // Get the idle time for the identity.
    int identityIdleTimeout = m_identitiesWithIdleTimesOnAutoAway[identityId];

    // The remaining time until the user will be marked as "auto-away".
    int remainingTime = identityIdleTimeout - KIdleTime::instance()->idleTime();

    return remainingTime;
}

void AwayManager::implementUpdateIdleTimeout(int identityId)
{
    const QHash<int, int> idleTimeouts = KIdleTime::instance()->idleTimeouts();

    // calculate the remaining time until the user will be marked as away.
    int remainingTime = calculateRemainingTime(identityId);

    // Check if the user should be away right now.
    if (remainingTime <= 0)
    {
        implementMarkIdentityAway(identityId);

        // Since the user is away right now the next auto-away should occur
        // in X minutes (where X is the the timeout which the user has
        // configured for the identity).
        remainingTime = m_identitiesWithIdleTimesOnAutoAway[identityId];
    }

    // Get the timer which is currently active for the identity.
    int timerId = m_timerForIdentity.key(identityId, -1);

    // Checks if we already have a timer for the given identity.
    // If we already have a timer we need to make sure that we're always
    // waiting for the remaining time.
    if (idleTimeouts[timerId] != remainingTime)
    {
        // Remove the idle timeout.
        implementRemoveIdleTimeout(timerId);

        // Then also reset the timer ID (as the timer does not exist anymore).
        timerId = -1;
    }

    // Check if we already have a timer.
    if (timerId == -1)
        // If not create a new timer.
        implementAddIdleTimeout(identityId, remainingTime);
}

void AwayManager::implementAddIdleTimeout(int identityId, int idleTime)
{
    // Create a new timer.
    int newTimerId = KIdleTime::instance()->addIdleTimeout(idleTime);

    // Make sure we keep track of the identity <-> KIdleTimer mapping.
    m_timerForIdentity[newTimerId] = identityId;
}

void AwayManager::implementRemoveIdleTimeout(int timerId)
{
    // Make sure we got a valid timer ID.
    if (timerId != -1)
    {
        // Remove the idle timeout.
        KIdleTime::instance()->removeIdleTimeout(timerId);

        // Also remove the timer/identity mapping from our hashtable.
        m_timerForIdentity.remove(timerId);
    }
}

void AwayManager::implementMarkIdentityAway(int identityId)
{
    // Mark the current identity as away.
    implementManagedAway(identityId);

    // As at least one identity is away we have to catch the next
    // resume event.
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::simulateUserActivity()
{
    // Tell KIdleTime that it should reset the user's idle status.
    KIdleTime::instance()->simulateUserActivity();
}

void AwayManager::resumeFromIdle()
{
    // We are not idle anymore.
    setManagedIdentitiesUnaway();

    QHash<int, int>::ConstIterator itr = m_identitiesWithIdleTimesOnAutoAway.constBegin();

    for (; itr != m_identitiesWithIdleTimesOnAutoAway.constEnd(); ++itr)
        // Update the idle timeout for the identity to the configured timeout.
        // This is needed in case the timer is not set to fire after the full
        // away time but a shorter time (for example if the user was away on startup
        // we want the timer to fire after "configured away-time" minus "time the user
        // is already idle"). Then the timer's interval is wrong.
        // Now (if needed) we simply remove the old timer and add a new one with the
        // correct interval.
        implementUpdateIdleTimeout(itr.key());
}

void AwayManager::idleTimeoutReached(int timerId)
{
    // Get the identity ID for the given timer ID.
    int identityId = m_timerForIdentity[timerId];

    // Mark the identity as away.
    implementMarkIdentityAway(identityId);
}

void AwayManager::identitiesOnAutoAwayChanged()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    // Since the list of identities has changed we want to drop all timers.
    KIdleTime::instance()->removeAllIdleTimeouts();

    // Also clear the list of identity <-> timer mappings.
    m_timerForIdentity.clear();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();
        int identityId = identity->id();

        // Only add idle timeouts for identities which have auto-away
        // enabled.
        if (m_identitiesWithIdleTimesOnAutoAway.contains(identityId))
            // Update the idle timeout for the current identity.
            implementUpdateIdleTimeout(identityId);
    }
}

void AwayManager::identityOnAutoAwayWentOnline(int identityId)
{
    // Simply update the idle timeout for the identity (this will
    // take care of all necessary calculations).
    implementUpdateIdleTimeout(identityId);
}

void AwayManager::identityOnAutoAwayWentOffline(int identityId)
{
    // Get the timer for the given identity.
    int timerId = m_timerForIdentity.key(identityId, -1);

    // Then remove the timer.
    implementRemoveIdleTimeout(timerId);
}

#include "awaymanager.moc"
