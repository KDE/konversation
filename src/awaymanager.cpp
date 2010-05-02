/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
  Copyright (C) 2008 Eike Hein <hein@kde.org>
  Copyright (C) 2010 Martin Blumenstingl <darklight.xdarklight@googlemail.com>
*/

#include "awaymanager.h"
#include "application.h"
#include "mainwindow.h"
#include "connectionmanager.h"
#include "server.h"
#include "preferences.h"
#include <config-konversation.h>

#include <QTimer>
#include <QDBusInterface>
#include <QDBusReply>

#include <KActionCollection>
#include <KToggleAction>

#ifdef Q_WS_X11

#if defined(HAVE_X11) && defined(HAVE_XUTIL)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#define HasXHeaders
#endif

#ifdef HAVE_XSCREENSAVER
#define HasScreenSaver
#include <X11/extensions/scrnsaver.h>
#include <QX11Info>
#endif

// Don't use XIdle for now, it's experimental.
#undef HAVE_XIDLE
#undef HasXidle

#include <fixx11h.h>
#endif


struct AwayManagerPrivate
{
    int mouseX;
    int mouseY;
    unsigned int mouseMask;
#ifdef HasXHeaders
    Window root;
    Screen* screen;
    Time xIdleTime;
#endif
    bool useXidle;
    bool useMit;
};

AwayManager::AwayManager(QObject* parent) : AbstractAwayManager(parent)
{
    int dummy = 0;
    dummy = dummy;

    d = new AwayManagerPrivate;

    d->mouseX = d->mouseY = 0;
    d->mouseMask = 0;
    d->useXidle = false;
    d->useMit = false;

#ifdef HasXHeaders
    Display* display = QX11Info::display();
    d->root = DefaultRootWindow(display);
    d->screen = ScreenOfDisplay(display, DefaultScreen (display));

    d->xIdleTime = 0;
#endif

#ifdef HasXidle
    d->useXidle = XidleQueryExtension(QX11Info::display(), &dummy, &dummy);
#endif

#ifdef HasScreenSaver
    if (!d->useXidle)
        d->useMit = XScreenSaverQueryExtension(QX11Info::display(), &dummy, &dummy);
#endif

    m_activityTimer = new QTimer(this);
    m_activityTimer->setObjectName("AwayTimer");
    connect(m_activityTimer, SIGNAL(timeout()), this, SLOT(checkActivity()));

    // Initially reset the idle status.
    resetIdle();
}

AwayManager::~AwayManager()
{
    delete d;
}

void AwayManager::simulateUserActivity()
{
    resetIdle();
}

void AwayManager::resetIdle()
{
    // Set the time of the idleTimer to the current time.
    m_idleTime.start();
}

int AwayManager::idleTime()
{
    // Calculate the idle time in milliseconds.
    return m_idleTime.elapsed();
}

void AwayManager::setManagedIdentitiesAway()
{
    // Used to skip X-based activity checking for one round, to avoid jumping
    // on residual mouse activity after manual screensaver activation.
    d->mouseX = -1;
    
    // Call the base implementation.
    AbstractAwayManager::setManagedIdentitiesAway();
}

void AwayManager::identitiesOnAutoAwayChanged()
{
    if (m_idetitiesWithIdleTimesOnAutoAway.count() > 0)
    {
        if (!m_activityTimer->isActive())
            m_activityTimer->start(Preferences::self()->autoAwayPollInterval() * 1000);
    }
    else if (m_activityTimer->isActive())
        m_activityTimer->stop();
}

void AwayManager::identityOnAutoAwayWentOnline(int identityId)
{
    Q_UNUSED(identityId);

    identitiesOnAutoAwayChanged();
}

void AwayManager::identityOnAutoAwayWentOffline(int identityId)
{
    Q_UNUSED(identityId);

    identitiesOnAutoAwayChanged();
}

void AwayManager::checkActivity()
{
    // Allow the event loop to be called, to avoid deadlock.
    static bool rentrencyProtection = false;
    if (rentrencyProtection) return;

    rentrencyProtection = true;
    QDBusInterface screenSaver("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver");
    QDBusReply<bool> isBlanked = screenSaver.call("GetActive");
    rentrencyProtection = false;

    if (!isBlanked.isValid() || !isBlanked.value())
    {
        // If there was activity we un-away all identities.
        // Otherwise we set all identities to away (if they
        // were idle long enough).
        if (Xactivity())
            setManagedIdentitiesUnaway();
        else
            implementIdleAutoAway();
    }
}

bool AwayManager::Xactivity()
{
    bool activity = false;

#ifdef HasXHeaders
    Display* display = QX11Info::display();
    Window dummyW;
    int dummyC;
    unsigned int mask;
    int rootX;
    int rootY;

    if (!XQueryPointer (display, d->root, &(d->root), &dummyW, &rootX, &rootY,
            &dummyC, &dummyC, &mask))
    {
        // Figure out which screen the pointer has moved to.
        for (int i = 0; i < ScreenCount(display); i++)
        {
            if (d->root == RootWindow(display, i))
            {
                d->screen = ScreenOfDisplay (display, i);

                break;
            }
        }
    }

    Time xIdleTime = 0;

    #ifdef HasXidle
    if (d->useXidle)
        XGetIdleTime(display, &xIdleTime);
    else
    #endif
    {
    #ifdef HasScreenSaver
        if (d->useMit)
        {
            static XScreenSaverInfo* mitInfo = 0;
            if (!mitInfo) mitInfo = XScreenSaverAllocInfo();
            XScreenSaverQueryInfo (display, d->root, mitInfo);
            xIdleTime = mitInfo->idle;
        }
    #endif
    }

    if (rootX != d->mouseX || rootY != d->mouseY || mask != d->mouseMask
        || ((d->useXidle || d->useMit) && xIdleTime < d->xIdleTime + 2000))
    {
        // Set by setManagedIdentitiesAway() to skip X-based activity checking for one
        // round, to avoid jumping on residual mouse activity after manual screensaver
        // activation.
        if (d->mouseX != -1) activity = true;

        d->mouseX = rootX;
        d->mouseY = rootY;
        d->mouseMask = mask;
        d->xIdleTime = xIdleTime;
    }
#endif

    return activity;
}

void AwayManager::implementIdleAutoAway()
{
    QHash<int, int>::ConstIterator it;

    for (it = m_idetitiesWithIdleTimesOnAutoAway.constBegin(); it != m_idetitiesWithIdleTimesOnAutoAway.constEnd(); ++it)
    {
        // Check if the auto-away timeout (which the user has configured for the given identity)
        // has already elapsed - if it has we mark the identity as away.
        if (idleTime() >= it.value())
            implementManagedAway(it.key());
    }
}

void AwayManager::implementManagedUnaway(const QList<int>& identityList)
{
    // Call the base implementation (which does all workflow logic).
    AbstractAwayManager::implementManagedUnaway(identityList);

    // Then reset the idle status as the user is not idle anymore.
    resetIdle();
}

#include "awaymanager.moc"
