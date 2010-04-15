/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (c) 2008 Eike Hein <hein@kde.org>
  Copyright (c) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#ifndef ABSTRACTAWAYMANAGER_H
#define ABSTRACTAWAYMANAGER_H

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
          * Marks all identities which have auto-away enabled as "not away".
          */
        virtual void setManagedIdentitiesUnaway();

        void toggleGlobalAway(bool away);
        void updateGlobalAwayAction(bool away);

    protected:
        /**
          * Marks all given identities as away if they have auto-away enabled.
          *
          * @param identityList a list of identitiy IDs which will be marked as away
          */
        void setIdentitiesAway(const QList<int>& identityList);
        
        /**
          * Marks all given identities as "not away" if they have automatic un-away enabled.
          *
          * @param identityList a list of identitiy IDs which will be marked as "not away"
          */
        void setIdentitiesUnaway(const QList<int>& identityList);
        
        /**
          * Called when the list of identities which have auto-away enabled has changed.
          * NOTE: This method is abstract. if you inherit AbstractAwayManager you need to implement this.
          */
        virtual void identitiesOnAutoAwayChanged() = 0;
        
        /**
          * Resets the idle status.
          * NOTE: This method is abstract. if you inherit AbstractAwayManager you need to implement this.
          */
        virtual void resetIdle() = 0;

        QList<int> m_identitiesOnAutoAway;

        ConnectionManager* m_connectionManager;
};

#endif
