/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2008 Eike Hein <hein@kde.org>
  Copyright (C) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#ifndef AWAYMANAGER_H
#define AWAYMANAGER_H

#include "abstractawaymanager.h"

#include <QTime>
#include <QTimer>

struct AwayManagerPrivate;

class AwayManager : public AbstractAwayManager
{
    Q_OBJECT

    public:
        AwayManager(QObject* parent = 0);
        ~AwayManager();

        /**
          * Simulates user activity. This simply resets the idle status.
          */
        virtual void simulateUserActivity();


    public slots:
        virtual void setManagedIdentitiesAway();


    private slots:
        void checkActivity();


    protected:
        /**
          * The list of identities which have auto-away enabled has changed.
          * This starts or stops the timer (depending on what's needed).
          */
        virtual void identitiesOnAutoAwayChanged();

        /**
          * Restarts the idle time calculation.
          */
        void resetIdle();

        /**
          * Returns the idle time in milliseconds.
          */
        int idleTime();

        /**
          * Decides which identities should be marked as "away".
          */
        void implementIdleAutoAway();

        bool Xactivity();

        /**
          * Marks all given identities as "not away" if they have automatic un-away enabled.
          * Also resets the idle status.
          *
          * @param identityList a list of identitiy IDs which will be marked as "not away"
          */
        virtual void implementManagedUnaway(const QList<int>& identityList);

        /**
          * An identity which has auto-away enabled went online.
          *
          * @param identityId the ID of the identity which just went online
          */
        virtual void identityOnAutoAwayWentOnline(int identityId);

        /**
          * An identity which has auto-away enabled went offline.
          *
          * @param identityId the ID of the identity which just went offline
          */
        virtual void identityOnAutoAwayWentOffline(int identityId);

    private:
        QTimer* m_activityTimer;

        QTime m_idleTime;

        struct AwayManagerPrivate* d;
};

#endif
