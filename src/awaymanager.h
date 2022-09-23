/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#ifndef AWAYMANAGER_H
#define AWAYMANAGER_H

#include <QObject>
#include <QHash>

class ConnectionManager;

class AwayManager : public QObject
{
    Q_OBJECT

    public:
        explicit AwayManager(QObject* parent = nullptr);
        ~AwayManager() override;


    public Q_SLOTS:
        void identitiesChanged();

        void identityOnline(int identityId);
        void identityOffline(int identityId);

        void requestAllAway(const QString& reason = QString());
        void requestAllUnaway();

        /**
          * Marks all identities which have auto-away enabled as away.
          */
        void setManagedIdentitiesAway();

        /**
          * Marks all identities which have auto-away and automatic un-away enabled as "not away".
          */
        void setManagedIdentitiesUnaway();

        /**
          * This tells KIdleTime that the user was active. Then the idle-time
          * calculation is restarted.
          */
        void resetUserActivity();

        void setGlobalAway(bool away);
        void updateGlobalAwayAction(bool away);


    private Q_SLOTS:
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
          * Marks the given identity as away if it has auto-away enabled.
          *
          * @param identityId the ID of the identity.
          */
        void implementManagedAway(int identityId);

        /**
          * Marks all given identities as "not away" if they have automatic un-away enabled.
          *
          * @param identityList a list of identitiy IDs which will be marked as "not away"
          */
        void implementManagedUnaway(const QList<int>& identityList);

        /**
          * Called when the list of identities which have auto-away enabled has changed.
          * This first drops all active idle timers. Then it triggers recalculation
          * of all idle timers for all identities which have auto-away enabled.
          */
        void identitiesOnAutoAwayChanged();

        /**
          * An identity which has auto-away enabled went offline.
          * Triggers the recalculation of the auto-away timer of the identity.
          *
          * @param identityId the ID of the identity which just went offline
          */
        void identityOnAutoAwayWentOnline(int identityId);

        /**
          * An identity which has auto-away enabled went offline.
          * Stops the idle timer corresponding to the given identity.
          *
          * @param identityId the ID of the identity which just went offline
          */
        void identityOnAutoAwayWentOffline(int identityId);

        /**
          * Creates a new idle timer for the given identity with the given
          * idle time. Also adds the timer to the identity <-> timer mapping.
          *
          * @param identityId the ID of the identity for which the timer is
          * @param idleTime the interval of the idle timer
          */
        void implementAddIdleTimeout(int identityId, int idleTime);

        /**
          * Removes the timer with the given ID. Also removes the timer from the
          * identity <-> timer mapping.
          *
          * @param timerId the ID of the timer which should be rmoved.
          */
        void implementRemoveIdleTimeout(int timerId);

        /**
          * Updates the KIdleTime timer for the given identity.
          * If there is no timer for the identity yet it will be created.
          * In case the auto-away time has changed this will update the
          * KIdleTime timers (and the internal identity <-> timer mapping).
          *
          * @param identityId the ID of the identity for which the idle timeout
          *                   should be updated
          */
        void implementUpdateIdleTimeout(int identityId);

        /**
          * Calculates the remaining time until the idle time has elapsed.
          *
          * @param identityId the identity for which the remaining time should
          *                   get calculated
          */
        int calculateRemainingTime(int identityId);

        /**
          * Marks the given identity as away. Also starts catching KIdleTime
          * resume events.
          *
          * @param identityId the identity which should be marked as away
          */
        void implementMarkIdentityAway(int identityId);

        /**
          * Converts the given minutes to milliseconds.
          *
          * @param minutes the number of minutes
          */
        static int minutesToMilliseconds(int minutes);

    private:
        /**
          * A list of identity IDs and their corresponding auto-away times (in
          * milliseconds).
          */
        QHash<int, int> m_identitiesWithIdleTimesOnAutoAway;

        /**
          * A mapping between KIdleTime timer IDs (key) and identity IDs (value).
          */
        QHash<int, int> m_timerForIdentity;

        ConnectionManager* m_connectionManager;

        Q_DISABLE_COPY(AwayManager)
};

#endif
