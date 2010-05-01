/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#include "awaymanagerkidletime.h"

#include "server.h"
#include "connectionmanager.h"

#include <kidletime.h>

AwayManager::AwayManager(QObject* parent) : AbstractAwayManager(parent)
{
    connect(KIdleTime::instance(), SIGNAL(resumingFromIdle()), this, SLOT(resumeFromIdle()));
    connect(KIdleTime::instance(), SIGNAL(timeoutReached(int)), this, SLOT(idleTimeoutReached(int)));

    // Catch the first "resume event" (= user input) so we correctly catch the first
    // resume event in case the user is already idle on startup).
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::implementUpdateIdleTimeout(int identityId, int idleTime)
{
    // Ensure we get sane values passed.
    Q_ASSERT(idleTime > 0);

    const QHash<int, int> idleTimeouts = KIdleTime::instance()->idleTimeouts();

    // Get the timer which is currently active for the identity.
    int timerId = m_timerForIdentity.key(identityId, -1);

    // Checks if we already have a timer for the given identity.
    // If we already have a timer we need to make sure that we're always
    // waiting for the given "idleTime" as the idle timeout may change.
    if (timerId != -1 && idleTimeouts[timerId] != idleTime)
    {
        // Remove the idle timeout.
        KIdleTime::instance()->removeIdleTimeout(timerId);

        // Also remove the timer/identity mapping from our hashtable.
        m_timerForIdentity.remove(timerId);

        // Then also reset the timer ID (as it's invalid now).
        timerId = -1;
    }

    // Check if we already have a timer.
    if (timerId == -1)
    {
        // Make sure we don't have a timer for the given identity yet.
        // Otherwise there's a bug somewhere.
        Q_ASSERT(m_timerForIdentity.key(identityId, -1) != -1);

        // If there's not timer yet we should create a new one.
        int newTimerId = KIdleTime::instance()->addIdleTimeout(idleTime);

        // Make sure we keep track of the identity <-> KIdleTimer mapping.
        m_timerForIdentity[newTimerId] = identityId;
    }
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

    foreach (int identityId, m_identitiesOnAutoAway)
        // Update the idle timeout for the identity to the configured timeout.
        // This is needed in case the timer is not set to fire after the full
        // away time but a shorter time (for example if the user was away on startup
        // we want the timer to fire after "configured away-time" minus "time the user
        // is already idle"). Then the timer's interval is wrong.
        // Now (if needed) we simply remove the old timer and add a new one with the
        // correct interval.
        implementUpdateIdleTimeout(identityId, m_identityAutoAwayTimes[identityId]);
}

void AwayManager::idleTimeoutReached(int timerId)
{
    // Get the identity ID for the given timer ID.
    int identityId = m_timerForIdentity[timerId];

    // Mark the identity as away.
    implementManagedAway(identityId);

    // Since we're away we now need to watch for resume events (keyboard input, etc).
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::identitiesOnAutoAwayChanged()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();
        int identityId = identity->id();

        // The idle timeout for the current identity in ms.
        int identityIdleTimeout = identity->getAwayInactivity() * 60 * 1000;

        // The remaining time until the user will be marked as "auto-away".
        int remainingTime = identityIdleTimeout - KIdleTime::instance()->idleTime();

        // Check if the user should be away right now.
        if (remainingTime <= 0)
        {
            // Mark the current identity as away.
            implementManagedAway(identityId);

            // As at least one identity is away we have to catch the next
            // resume event.
            KIdleTime::instance()->catchNextResumeEvent();

            // Since the user is away right now the next auto-away should occur
            // in X minutes (where X is the the timeout which the user has
            // configured for the identity).
            remainingTime = identityIdleTimeout;
        }

        // Store the idle timeout for the current identity.
        m_identityAutoAwayTimes[identityId] = identityIdleTimeout;

        // Update the idle timeout for the current identity.
        implementUpdateIdleTimeout(identityId, remainingTime);
    }
}

#include "awaymanagerkidletime.moc"
