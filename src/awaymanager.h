/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Copyright (C) 2008 Eike Hein <hein@kde.org>
  Copyright (c) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#ifndef AWAYMANAGER_H
#define AWAYMANAGER_H

#include "abstractawaymanager.h"

#include <QTimer>
#include <QTime>

struct AwayManagerPrivate;

class AwayManager : public AbstractAwayManager
{
    Q_OBJECT

    public:
        AwayManager(QObject* parent = 0);
        ~AwayManager();
        
    public slots:
        virtual void setManagedIdentitiesAway();
        virtual void setManagedIdentitiesUnaway();

    private slots:
        void checkActivity();

    private:
        /**
          * the list of identities which have auto-away enabled has changed
          * this starts or stops the timer
          */
        virtual void identitiesOnAutoAwayChanged();
        
        virtual void resetIdle();

        bool Xactivity();

        void implementIdleAutoAway(bool activity);

        QTime m_idleTime;
        QTimer* m_activityTimer;
        
        struct AwayManagerPrivate* d;
};

#endif
