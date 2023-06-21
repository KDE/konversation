/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2008 Eli J. MacKenzie <argonel at gmail.com>
*/

#include "queuetuner.h"

#include "server.h"
#include "ircqueue.h"
#include "channel.h"
#include "viewcontainer.h"
#include "application.h"

#include <QContextMenuEvent>

#include <QMenu>
#include <KMessageBox>


QueueTuner::QueueTuner(QWidget* parent, ViewContainer *container)
: QWidget(parent), m_server(nullptr), m_timer(this),
    m_vis(Preferences::self()->showQueueTunerItem()->value())
{
    setupUi(this);

    m_closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    connect(m_closeButton, &QToolButton::clicked, this, &QueueTuner::close);
    connect(container, &ViewContainer::frontServerChanging, this, &QueueTuner::setServer);
    connect(&m_timer, &QTimer::timeout, this, &QueueTuner::timerFired);

    connect(m_slowRate, QOverload<int>::of(&QSpinBox::valueChanged), this, &QueueTuner::slowRateChanged);
    connect(m_slowType, QOverload<int>::of(&KComboBox::activated), this, &QueueTuner::slowTypeChanged);
    connect(m_slowInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, &QueueTuner::slowIntervalChanged);

    connect(m_normalRate, QOverload<int>::of(&QSpinBox::valueChanged), this, &QueueTuner::normalRateChanged);
    connect(m_normalType, QOverload<int>::of(&KComboBox::activated), this, &QueueTuner::normalTypeChanged);
    connect(m_normalInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, &QueueTuner::normalIntervalChanged);

    connect(m_fastRate, QOverload<int>::of(&QSpinBox::valueChanged), this, &QueueTuner::fastRateChanged);
    connect(m_fastType, QOverload<int>::of(&KComboBox::activated), this, &QueueTuner::fastTypeChanged);
    connect(m_fastInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, &QueueTuner::fastIntervalChanged);

    m_timer.setObjectName(QStringLiteral("qTuner"));
}

QueueTuner::~QueueTuner()
{
}

//lps, lpm, bps, kbps
static void rateToWidget(IRCQueue::EmptyingRate& rate, QSpinBox *r, KComboBox* t, QSpinBox *i)
{
    r->setValue(rate.m_rate);
    t->setCurrentIndex(rate.m_type);
    i->setValue(rate.m_interval/1000);
}

void QueueTuner::serverDestroyed(QObject* ref)
{
    if (ref == m_server)
        setServer(nullptr);
}

void QueueTuner::setServer(Server* newServer)
{
    bool toShow = false;

    if (!m_server && newServer)
        toShow = true;
    else if (!newServer && m_server)
        hide();

    // since this is tied to the new signal, we assume we're only getting called with a change

    m_server = newServer;

    if (toShow)
        show();

    if (m_server)
    {
        connect(m_server, &QObject::destroyed, this, &QueueTuner::serverDestroyed);

        getRates();
    }
}

void QueueTuner::getRates()
{
    if (!m_server) // you can only get the popup if there is a server, but.. paranoid
        return;

    rateToWidget(m_server->m_queues[0]->getRate(), m_slowRate, m_slowType, m_slowInterval);
    rateToWidget(m_server->m_queues[1]->getRate(), m_normalRate, m_normalType, m_normalInterval);
    rateToWidget(m_server->m_queues[2]->getRate(), m_fastRate, m_fastType, m_fastInterval);
}

void QueueTuner::timerFired()
{
    if (m_server)
    {
        IRCQueue *q=nullptr;

        q=m_server->m_queues[0];
        m_slowAge->setNum(q->currentWait()/1000);
        m_slowBytes->setNum(q->bytesSent());
        m_slowCount->setNum(q->pendingMessages());
        m_slowLines->setNum(q->linesSent());

        q=m_server->m_queues[1];
        m_normalAge->setNum(q->currentWait()/1000);
        m_normalBytes->setNum(q->bytesSent());
        m_normalCount->setNum(q->pendingMessages());
        m_normalLines->setNum(q->linesSent());

        q=m_server->m_queues[2];
        m_fastAge->setNum(q->currentWait()/1000);
        m_fastBytes->setNum(q->bytesSent());
        m_fastCount->setNum(q->pendingMessages());
        m_fastLines->setNum(q->linesSent());

        m_srverBytes->setNum(m_server->m_encodedBytesSent);
        m_globalBytes->setNum(m_server->m_bytesSent);
        m_globalLines->setNum(m_server->m_linesSent);
        m_recvBytes->setNum(m_server->m_bytesReceived);
    }
}

void QueueTuner::open()
{
    Preferences::self()->setShowQueueTuner(true);
    show();
}

void QueueTuner::close()
{
    Preferences::self()->setShowQueueTuner(false);
    QWidget::close();
}

void QueueTuner::show()
{
    if (m_server && Preferences::self()->showQueueTuner())
    {
        QWidget::show();
        m_timer.start(500);

    }
}

void QueueTuner::hide()
{
    QWidget::hide();
    m_timer.stop();
}

void QueueTuner::slowRateChanged(int v)
{
    if (!m_server) return;
    int &r=m_server->m_queues[0]->getRate().m_rate;
    r=v;
}

void QueueTuner::slowTypeChanged(int v)
{
    if (!m_server) return;
    IRCQueue::EmptyingRate::RateType &r=m_server->m_queues[0]->getRate().m_type;
    r=IRCQueue::EmptyingRate::RateType(v);
}

void QueueTuner::slowIntervalChanged(int v)
{
    if (!m_server) return;
    int &r=m_server->m_queues[0]->getRate().m_interval;
    r=v*1000;
}

void QueueTuner::normalRateChanged(int v)
{
    if (!m_server) return;
    int &r=m_server->m_queues[1]->getRate().m_rate;
    r=v;
}

void QueueTuner::normalTypeChanged(int v)
{
    if (!m_server) return;
    IRCQueue::EmptyingRate::RateType &r=m_server->m_queues[1]->getRate().m_type;
    r=IRCQueue::EmptyingRate::RateType(v);
}

void QueueTuner::normalIntervalChanged(int v)
{
    if (!m_server) return;
    int &r=m_server->m_queues[1]->getRate().m_interval;
    r=v*1000;
}

void QueueTuner::fastRateChanged(int v)
{
    if (!m_server) return;
    int &r=m_server->m_queues[2]->getRate().m_rate;
    r=v;
}

void QueueTuner::fastTypeChanged(int v)
{
    if (!m_server) return;
    IRCQueue::EmptyingRate::RateType &r=m_server->m_queues[2]->getRate().m_type;
    r=IRCQueue::EmptyingRate::RateType(v);
}

void QueueTuner::fastIntervalChanged(int v)
{
    if (!m_server) return;
    int &r=m_server->m_queues[2]->getRate().m_interval;
    r=v*1000;
}

void QueueTuner::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu p(this);
    p.addAction(i18n("Reset..."));
    QAction *action = p.exec(e->globalPos());
    if (action)
    {

        QString question(i18n("This cannot be undone, are you sure you wish to reset to default values?"));
        int x = KMessageBox::warningContinueCancel(
                this, question, i18n("Reset Values"),
                KStandardGuiItem::reset(), KStandardGuiItem::cancel(),
                QString(), KMessageBox::Dangerous
        );
        if (x == KMessageBox::Continue)
        {
            Application::instance()->resetQueueRates();
            getRates();
        }
    }
    e->accept();
}

#include "moc_queuetuner.cpp"
