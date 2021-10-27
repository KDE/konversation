/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2008 Eli J. MacKenzie <argonel at gmail.com>
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
        ~QueueTuner() override;

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

    protected:
        void contextMenuEvent (QContextMenuEvent*) override;

    private:
        Server* m_server;
        QTimer m_timer;
        bool &m_vis;

        Q_DISABLE_COPY(QueueTuner)
};

#endif
