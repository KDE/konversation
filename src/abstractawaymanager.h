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
        void identitiesOnAutoAwayChanged();

    public slots:
        void identitiesChanged();

        void identityOnline(int identityId);
        void identityOffline(int identityId);

        void requestAllAway(const QString& reason = "");
        void requestAllUnaway();

        virtual void setManagedIdentitiesAway();
        void setManagedIdentitiesUnaway();

        void toggleGlobalAway(bool away);
        void updateGlobalAwayAction(bool away);

    protected:
        virtual void resetIdle() = 0;

        QList<int> m_identitiesOnAutoAway;

        ConnectionManager* m_connectionManager;
};

#endif
