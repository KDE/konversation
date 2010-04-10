/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Copyright (C) 2008 Eike Hein <hein@kde.org>
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

    signals:
        /**
          * emitted when the list of identities which have auto-away enabled changes
          */
        void identitiesOnAutoAwayChanged();

        /**
          * emitted when the away state of the identities in the identityList
          * has changed.
          *
          * @param identityList a list of identity IDs for which the away status has changed
          * @param away the new away status
          */
        void toggleAway(const QList<int>& identityList, bool away);

    public slots:
        void identitiesChanged();

        void identityOnline(int identityId);
        void identityOffline(int identityId);

        void requestAllAway(const QString& reason = "");
        void requestAllUnaway();

        /**
          * marks all identities which have auto-away enabled as away
          */
        virtual void setManagedIdentitiesAway();

        /**
          * marks all identities which have auto-away enabled as "not away"
          */
        virtual void setManagedIdentitiesUnaway();

        /**
          * marks all given identities as away
          *
          * @param identityList a list of identitiy IDs which will be marked as away
          */
        void setIdentitiesAway(const QList<int>& identityList);

        /**
          * marks all given identities as "not away"
          *
          * @param identityList a list of identitiy IDs which will be marked as "not away"
          */
        void setIdentitiesUnaway(const QList<int>& identityList);

        void toggleGlobalAway(bool away);
        void updateGlobalAwayAction(bool away);

        /**
          * toggles the away status for all identities in the identityList
          * the given away flag is the new away status of the identities
          *
          * @param identityList a list of identity IDs of which the away status should be changed
          * @param away the new away status of the identities
          */
        void toggleIdentitiesAwayStatus(const QList<int>& identityList, bool away);

    protected:
        /**
          * resets the idle status
          * NOTE: this method is abstract. if you inherit AbstractAwayManager you need to implement this
          */
        virtual void resetIdle() = 0;

        QList<int> m_identitiesOnAutoAway;

        ConnectionManager* m_connectionManager;
};

#endif
