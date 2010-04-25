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

class AwayManager : public AbstractAwayManager
{
    Q_OBJECT

    public:
        AwayManager(QObject* parent = 0);


    private slots:
        /**
          * Called as soon as the user does some input (after he was away).
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
          * This handles the (de-)registration of KIdleTimers.
          */
        virtual void identitiesOnAutoAwayChanged();

        /**
          * Removes unneeded timeouts (for example after a user changed the
          * auto-away time) from KIdleTime. Only timeouts which are still needed
          * (= which are still in the idle timeout list) are kept.
          */
        void implementRemoveUnusedIdleTimeouts();

        /**
          * Adds all timeouts (from the idle timeout list) to KIdleTime if
          * KIdleTime does not know about them yet.
          */
        void implementAddIdleTimeouts();

        QList<int> m_idleTimeouts;
};

#endif