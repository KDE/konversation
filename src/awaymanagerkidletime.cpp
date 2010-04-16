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

    // catch the first "resume event" (= user input)
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::implementRemoveUnusedIdleTimeouts()
{
    const QHash<int, int> idleTimeouts = KIdleTime::instance()->idleTimeouts();

    if (idleTimeouts.count() > 0)
    {
        QHash<int, int>::ConstIterator it;

        // loop through the list of all KIdleTimers
        for (it = idleTimeouts.constBegin(); it != idleTimeouts.constEnd(); ++it)
        {
            int timeout = it.value();

            // check if the list with all idle timeouts does not contain the current timeout
            if (!m_idleTimeouts.contains(timeout))
            {
                int timerId = it.key();

                // then we need to remove it from KIdleTime
                KIdleTime::instance()->removeIdleTimeout(timerId);
            }
        }
    }
}

void AwayManager::implementAddIdleTimeouts()
{
    foreach (int timeout, m_idleTimeouts)
    {
        // get the timerId for the given timeout
        int timerId = KIdleTime::instance()->idleTimeouts().key(timeout, -1);

        // check if there's already a timer with the given timeout
        if (timerId == -1)
        {
            // if not create a new idle timeout
            KIdleTime::instance()->addIdleTimeout(timeout);
        }
    }
}

void AwayManager::resetIdle()
{
    // simulate user activity (which reset all idle timers)
    KIdleTime::instance()->simulateUserActivity();

    // also call the base implementation
    AbstractAwayManager::resetIdle();
}

void AwayManager::resumeFromIdle()
{
    // we are not idle anymore
    implementIdleAutoAway(true);
}

void AwayManager::idleTimeoutReached(int timerId)
{
    Q_UNUSED(timerId);

    // check which identities are away now
    implementIdleAutoAway(false);

    // since we're away we now need to watch for resume events
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::identitiesOnAutoAwayChanged()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    // clear the list of idle timeouts (this will ensure that only timeouts
    // which are actually used by any identity are in the list)
    m_idleTimeouts.clear();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();

        // the idle timeout for the current identity in ms
        int identityIdleTimeout = identity->getAwayInactivity() * 60 * 1000;

        // check if we still need to add the idle timeout to our list
        if (!m_idleTimeouts.contains(identityIdleTimeout))
        {
            m_idleTimeouts.append(identityIdleTimeout);
        }
    }

    // add all used idle timeouts (if necessary)
    implementAddIdleTimeouts();

    // remove all unused timeouts
    implementRemoveUnusedIdleTimeouts();
}

#include "awaymanagerkidletime.moc"
