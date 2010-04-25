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

    // Catch the first "resume event" (= user input).
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::simulateUserActivity()
{
    // Tell KIdleTime that it should reset the user's idle status.
    KIdleTime::instance()->simulateUserActivity();

    // also call the base implementation (so the default logic is executed).
    AbstractAwayManager::simulateUserActivity();
}

void AwayManager::implementRemoveUnusedIdleTimeouts()
{
    const QHash<int, int> idleTimeouts = KIdleTime::instance()->idleTimeouts();

    if (idleTimeouts.count() > 0)
    {
        QHash<int, int>::ConstIterator it;

        // Loop through the list of all KIdleTimers.
        for (it = idleTimeouts.constBegin(); it != idleTimeouts.constEnd(); ++it)
        {
            int timeout = it.value();

            // Check if the list with all idle timeouts does not contain the current timeout.
            if (!m_idleTimeouts.contains(timeout))
            {
                int timerId = it.key();

                // Then we need to remove it from KIdleTime.
                KIdleTime::instance()->removeIdleTimeout(timerId);
            }
        }
    }
}

void AwayManager::implementAddIdleTimeouts()
{
    foreach (int timeout, m_idleTimeouts)
    {
        // Get the timerId for the given timeout.
        int timerId = KIdleTime::instance()->idleTimeouts().key(timeout, -1);

        // Check if there's already a timer with the given timeout.
        if (timerId == -1)
            // If there's not timer yet we should create a new one.
            KIdleTime::instance()->addIdleTimeout(timeout);
    }
}

void AwayManager::resumeFromIdle()
{
    // We are not idle anymore.
    implementIdleAutoAway(true);
}

void AwayManager::idleTimeoutReached(int timerId)
{
    Q_UNUSED(timerId);

    // Check which identities are away now.
    implementIdleAutoAway(false);

    // Since we're away we now need to watch for resume events (keyboard input, etc).
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::identitiesOnAutoAwayChanged()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    // Clear the list of idle timeouts (this will ensure that only timeouts
    // which are actually used by any identity are in the list).
    m_idleTimeouts.clear();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();

        // The idle timeout for the current identity in ms.
        int identityIdleTimeout = identity->getAwayInactivity() * 60 * 1000;

        // Check if we still need to add the idle timeout to our list.
        if (!m_idleTimeouts.contains(identityIdleTimeout))
            m_idleTimeouts.append(identityIdleTimeout);
    }

    // Add all used idle timeouts (if necessary).
    implementAddIdleTimeouts();

    // Remove all unused timeouts.
    implementRemoveUnusedIdleTimeouts();
}

#include "awaymanagerkidletime.moc"
