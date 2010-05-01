/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/


#ifndef AWAYMANAGERKIDLETIME
#define AWAYMANAGERKIDLETIME

#include "abstractawaymanager.h"

#include <QHash>

class AwayManager : public AbstractAwayManager
{
    Q_OBJECT

    public:
        AwayManager(QObject* parent = 0);

        /**
          * Simulates user activity.
          * This tells KIdleTime that the user was active. Then the idle-time
          * calculation is restarted.
          */
        virtual void simulateUserActivity();

    private slots:
        /**
          * Called as soon as the user does some input (after he was away).
          * This will also ensure that the next idle timeout is exactly
          * the one which the user has configured for the identity.
          */
        void resumeFromIdle();

        /**
          * The timer with the given ID has reached it's timeout.
          *
          * @param timerId the ID of the KIdleTimer
          */
        void idleTimeoutReached(int timerId);


    private:
        /**
          * The list of identities which have auto-away enabled has changed.
          * This calculates the remaining time per identity until the
          * identity will automatically become "away".
          */
        virtual void identitiesOnAutoAwayChanged();

        /**
          * Updates the KIdleTime timer for the given identity.
          * If there is no timer for the identity yet it will be created.
          * In case the auto-away time has changed this will update the
          * KIdleTime timers (and the internal identity <-> timer mapping).
          *
          * @param identityId the ID of the identity for which the idle timeout
          *                   should be updated
          * @param msec the idle time in milliseconds
          */
        void implementUpdateIdleTimeout(int identityId, int idleTime);

        /**
          * A mapping between KIdleTime timer IDs (key) and identity IDs (value).
          */
        QHash<int, int> m_timerForIdentity;

        /**
          * A list of identity IDs and their corresponding auto-away times (in
          * milliseconds).
          */
        QHash<int, int> m_identityAutoAwayTimes;
};

#endif