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
    connect(KIdleTime::instance(), SIGNAL(timeoutReached(int, int)), this, SLOT(idleTimeoutReached(int)));

    // catch the first "resume event" (= user input)
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::removeIdleTimeout(int timerId, int identityId)
{
    // remove the timer from KIdleTime and from our internal hash
    KIdleTime::instance()->removeIdleTimeout(timerId);
    
    m_identityIdTimerIdHash.remove(identityId);
}

void AwayManager::addIdleTimeout(int timeout, int identityId)
{
    // add a new idle timeout and add it to the internal hash
    int newTimerId = KIdleTime::instance()->addIdleTimeout(timeout);
    
    m_identityIdTimerIdHash[identityId] = newTimerId;
}

void AwayManager::resetIdle()
{
    // simulate user activity (which reset all idle timers)
    KIdleTime::instance()->simulateUserActivity();
}

void AwayManager::resumeFromIdle()
{
    // mark all identities which have auto-away enabled "not away"
    implementManagedUnaway(m_identitiesOnAutoAway);
}

void AwayManager::idleTimeoutReached(int timerId)
{
    // get the identity ID for the given timer
    int identityId = m_identityIdTimerIdHash.key(timerId, -1);
    
    if (identityId != -1)
    {
        QList<int> identitiesIdleTimeExceeded;

        identitiesIdleTimeExceeded.append(identityId);

        implementManagedAway(identitiesIdleTimeExceeded);
    }
    else
        kDebug() << "could not find identity for timer " << timerId << " - bug?";    
    
    // wait for user input (so we get the "resumingFromIdle" signal fired
    // as soon as the user does something)
    KIdleTime::instance()->catchNextResumeEvent();
}

void AwayManager::identitiesOnAutoAwayChanged()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();
    
    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();

        // the idle timeout for the current identity in ms
        int identityIdleTimeout = identity->getAwayInactivity() * 60 * 1000;
        int identityId = identity->id();
        
        // check if the identity is already in our hash
        if (m_identityIdTimerIdHash.contains(identityId))
        {
            int timerId = m_identityIdTimerIdHash[identityId];
            
            // check if the identity is not in the auto-away list
            if (!m_identitiesOnAutoAway.contains(identityId))
                // then we need to remove the idle timeout
                removeIdleTimeout(timerId, identityId);
            else
            {
                const QHash<int, int> idleTimeouts = KIdleTime::instance()->idleTimeouts();
                
                if (idleTimeouts.contains(timerId))
                {
                    // check if the timeout has changed
                    if (idleTimeouts[timerId] != identityIdleTimeout)
                    {
                        // then we need to remove and re-add the timeout
                        removeIdleTimeout(timerId, identityId);
                        addIdleTimeout(identityIdleTimeout, identityId);
                    }
                }
                else
                    kDebug() << "WARNING: bug? we have a timer ID (" << timerId << ") but KIdleTime does not know about it?";
            }
        }
        else
        {
            // we are not watching a timeout for the current identity
            // check if the identity has auto-away configured
            if (m_identitiesOnAutoAway.contains(identityId))
                // then we need to add a timeout for it
                addIdleTimeout(identityIdleTimeout, identityId);
        }
    }
}

#include "awaymanagerkidletime.moc"
