/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Copyright (c) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
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

    private slots:
        /**
          * called as soon as the user does some input (after he was away)
          */
        void resumeFromIdle();

        /**
          * the timer with the given ID has reached it's timeout
          *
          * @param timerId the ID of the KIdleTimer
          */
        void idleTimeoutReached(int timerId);

    private:
        /**
          * the list of identities which have auto-away enabled has changed
          * this handles the (de-)registration of KIdleTimers
          */
        virtual void identitiesOnAutoAwayChanged();
        
        /**
          * removes an idle timeout
          *
          * @param timerId the ID of the KIdleTimer
          * @param identityId the ID of the identity to which the idle timer belongs
          */
        void removeIdleTimeout(int timerId, int identityId);

        /**
          * adds a KIdleTimer with the given timeout for the given identity
          *
          * @param timeout the timeout for the timer (milliseconds)
          * @param identityId the ID of the identity to which the timer belongs
          */
        void addIdleTimeout(int timeout, int identityId);

        /**
          * resets the idle status (simulates user activity)
          */
        virtual void resetIdle();

        /**
          * a hashtable which contains identity IDs (key)
          * and KIdleTimer timer IDs (value)
          */
        QHash<int, int> m_identityIdTimerIdHash;
};

#endif