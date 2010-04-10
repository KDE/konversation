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
        void resumeFromIdle();
        void idleTimeoutReached(int timerId);

        void autoAwayIdentitiesChanged();

    private:
        void removeIdleTimeout(int timerId, int identityId);
        void addIdleTimeout(int timeout, int identityId);

        virtual void resetIdle();

        QHash<int, int> m_identityIdTimerIdHash;
};

#endif