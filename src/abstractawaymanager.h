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

#ifndef ABSTRACTAWAYMANAGER_H
#define ABSTRACTAWAYMANAGER_H

#include "identity.h"

#include <QObject>
#include <QList>

class ConnectionManager;

class AbstractAwayManager : public QObject
{
    Q_OBJECT

    public:
        AbstractAwayManager(QObject* parent = 0);


    public slots:
        void identitiesChanged();

        void identityOnline(int identityId);
        void identityOffline(int identityId);

        void requestAllAway(const QString& reason = "");
        void requestAllUnaway();

        /**
          * Marks all identities which have auto-away enabled as away.
          */
        virtual void setManagedIdentitiesAway();

        /**
          * Marks all identities which have auto-away and automatic un-away enabled as "not away".
          */
        virtual void setManagedIdentitiesUnaway();

        /**
          * Simulates user activity. This means the implementation should ensure
          * the idle indicators should be reset as if the user did some input
          * himself.
          * NOTE: This method is abstract. If you inherit AbstractAwayManager you need to implement this.
          */
        virtual void simulateUserActivity() = 0;

        void toggleGlobalAway(bool away);
        void updateGlobalAwayAction(bool away);


    protected:
        /**
          * Marks all given identities as away if they have auto-away enabled.
          *
          * @param identityList a list of identitiy IDs which will be marked as away
          */
        void implementManagedAway(const QList<int>& identityList);

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
          * NOTE: This method is abstract. If you inherit AbstractAwayManager you need to implement this.
          */
        virtual void identitiesOnAutoAwayChanged() = 0;

        /**
          * Resets the idle time.
          * NOTE: This method is abstract. If you inherit AbstractAwayManager you need to implement this.
          */
        virtual void resetIdle() = 0;

        QList<int> m_identitiesOnAutoAway;

        ConnectionManager* m_connectionManager;
};

#endif
