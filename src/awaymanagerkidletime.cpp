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

void AwayManager::resetIdle()
{
    // The KIdleTime based implementation does not need to
    // do anything here (it should know itself it has to 
    // reset the idle time)
}

int AwayManager::idleTime()
{
    // Calculate the idle time in seconds.
    return KIdleTime::instance()->idleTime() / 1000;
}

void AwayManager::simulateUserActivity()
{
    // Tell KIdleTime that it should reset the user's idle status.
    KIdleTime::instance()->simulateUserActivity();

    // also call the base implementation (so the default logic is executed).
    AbstractAwayManager::simulateUserActivity();
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

    KIdleTime::instance()->removeAllIdleTimeouts();

    foreach (Server* server, serverList)
    {
        // TODO: Move the whole block into a separate method (maybe into multiple
        // methods). Also check if we have to call catchNextResumeEvent() somewhere.

        // The idle timeout for the current identity in ms.
        int identityIdleTimeout = identity->getAwayInactivity() * 60 * 1000;

        // The current idle time in ms.
        int currentIdleTime = idleTime() * 1000;

        // The remaining time until the user will be marked as "auto-away".
        int remainingTime = identityIdleTimeout - currentIdleTime;

        // Check if the user should be away right now.
        if (remainingTime <= 0)
        {
            // Mark him away right now.
            implementIdleAutoAway(false);
        }
        else
        {
            // Get the timerId for the timeout with the remaining time.
            int timerId = KIdleTime::instance()->idleTimeouts().key(remainingTime, -1);

            // Check if there's already a timer with the remaining time (we might have more
            // identities with the same remaining time so this is necessary).
            if (timerId == -1)
                // If there's not timer yet we should create a new one.
                KIdleTime::instance()->addIdleTimeout(remainingTime);
        }
    }
}

#include "awaymanagerkidletime.moc"
