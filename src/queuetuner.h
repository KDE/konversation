/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy 
    defined in Section 14 of version 3 of the license.
*/

/*
  Copyright (C) 2008 Eli J. MacKenzie <argonel at gmail.com>
*/


#ifndef QUEUETUNER_H
#define QUEUETUNER_H

#include "ui_queuetunerbase.h"

class Server;
class ViewContainer;
class QTimer;

#include <QTimer>

class QueueTuner: public QWidget, private Ui::QueueTunerBase
{
    Q_OBJECT

    public:
        QueueTuner(QWidget* parent, ViewContainer *container);
        ~QueueTuner();
        void contextMenuEvent (QContextMenuEvent*) override;

    public Q_SLOTS:
        void setServer(Server* newServer);
        void getRates();
        void timerFired();
        virtual void hide();
        virtual void show();
        virtual void open();
        virtual void close();
        void slowRateChanged(int);
        void slowTypeChanged(int);
        void slowIntervalChanged(int);
        void normalRateChanged(int);
        void normalTypeChanged(int);
        void normalIntervalChanged(int);
        void fastRateChanged(int);
        void fastTypeChanged(int);
        void fastIntervalChanged(int);
        void serverDestroyed(QObject*);

    Q_SIGNALS:
        void hidden();

    private:
        Server* m_server;
        QTimer m_timer;
        bool &m_vis;
};


#endif
