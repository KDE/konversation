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


    public slots:
        virtual void setManagedIdentitiesAway();


    private slots:
        void checkActivity();


    private:
        /**
          * The list of identities which have auto-away enabled has changed.
          * This starts or stops the timer (depending on what's needed).
          */
        virtual void identitiesOnAutoAwayChanged();

        /**
          * Restarts the idle time calculation.
          */
        virtual void resetIdle();

        /**
          * Returns the idle time in seconds.
          */
        virtual int idleTime();

        /**
          * Decides which identities should be marked as "away".
          */
        void implementIdleAutoAway();

        bool Xactivity();

        QTimer* m_activityTimer;

        QTime m_idleTime;

        struct AwayManagerPrivate* d;
};

#endif
