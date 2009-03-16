/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
  Copyright (C) 2008 Eike Hein <hein@kde.org>
*/

#include "awaymanager.h"
#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "connectionmanager.h"
#include "server.h"
#include "preferences.h"
#include "config-konversation.h"

#include <qvariant.h>
#include <qtimer.h>

#include <kaction.h>
#include <kselectaction.h>
#include <klocale.h>
#include <KActionCollection>
#include <KToggleAction>
#include <kdebug.h>

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

AwayManager::AwayManager(QObject* parent) : QObject(parent)
{
    int dummy = 0;
    dummy = dummy;

    d = new AwayManagerPrivate;

    d->mouseX = d->mouseY = 0;
    d->mouseMask = 0;
    d->useXidle = false;
    d->useMit = false;

    m_connectionManager = static_cast<KonversationApplication*>(kapp)->getConnectionManager();

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

    m_idleTime.start();
}

AwayManager::~AwayManager()
{
    delete d;
}

void AwayManager::identitiesChanged()
{
    QList<int> newIdentityList;

    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();

        if (identity && identity->getAutomaticAway() && server->isConnected())
            newIdentityList.append(identity->id());
    }

    m_identitiesOnAutoAway = newIdentityList;

    toggleTimer();
}

void AwayManager::identityOnline(int identityId)
{
    IdentityPtr identity = Preferences::identityById(identityId);

    if (identity && identity->getAutomaticAway() &&
        !m_identitiesOnAutoAway.contains(identityId))
    {
        m_identitiesOnAutoAway.append(identityId);

        toggleTimer();
    }
}

void AwayManager::identityOffline(int identityId)
{
    if (m_identitiesOnAutoAway.removeOne(identityId))
    {
        toggleTimer();
    }
}

void AwayManager::toggleTimer()
{
    if (m_identitiesOnAutoAway.count() > 0)
    {
        if (!m_activityTimer->isActive())
            m_activityTimer->start(Preferences::self()->autoAwayPollInterval() * 1000);
    }
    else if (m_activityTimer->isActive())
        m_activityTimer->stop();
}

void AwayManager::checkActivity()
{
    // Allow the event loop to be called, to avoid deadlock.
    static bool rentrencyProtection = false;
    if (rentrencyProtection) return;

    rentrencyProtection = true;
/*
    DCOPRef screenSaver("kdesktop", "KScreensaverIface");
    DCOPReply isBlanked = screenSaver.callExt("isBlanked", DCOPRef::UseEventLoop, 10);
*/
    rentrencyProtection = false;

    QVariant isBlanked(false);
    if (!(isBlanked.isValid() && isBlanked.type() == QVariant::Bool && (isBlanked.toBool())))
         implementIdleAutoAway(Xactivity());
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

void AwayManager::implementIdleAutoAway(bool activity)
{
    if (activity)
    {
        m_idleTime.start();

        const QList<Server*> serverList = m_connectionManager->getServerList();

        foreach (Server* server, serverList)
        {
            IdentityPtr identity = server->getIdentity();

            if (m_identitiesOnAutoAway.contains(identity->id()) && identity->getAutomaticUnaway()
                && server->isConnected() && server->isAway())
            {
                server->requestUnaway();
            }
        }
    }
    else
    {
        long int idleTime = m_idleTime.elapsed() / 1000;

        QList<int> identitiesIdleTimeExceeded;
        QList<int>::ConstIterator it;

        for (it = m_identitiesOnAutoAway.constBegin(); it != m_identitiesOnAutoAway.constEnd(); ++it)
        {
            if (idleTime >= Preferences::identityById((*it))->getAwayInactivity() * 60)
                identitiesIdleTimeExceeded.append((*it));
        }

        const QList<Server*> serverList = m_connectionManager->getServerList();

        foreach (Server* server, serverList)
        {
            int identityId = server->getIdentity()->id();

            if (identitiesIdleTimeExceeded.contains(identityId) && server->isConnected() && !server->isAway())
                server->requestAway();
        }
    }
}

void AwayManager::setManagedIdentitiesAway()
{
    // Used to skip X-based activity checking for one round, to avoid jumping
    // on residual mouse activity after manual screensaver activation.
    d->mouseX = -1;

    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        if (m_identitiesOnAutoAway.contains(server->getIdentity()->id()) && server->isConnected() && !server->isAway())
            server->requestAway();
    }
}

void AwayManager::setManagedIdentitiesUnaway()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
    {
        IdentityPtr identity = server->getIdentity();

        if (m_identitiesOnAutoAway.contains(identity->id()) && identity->getAutomaticUnaway()
            && server->isConnected() && server->isAway())
        {
            server->requestUnaway();
        }
    }
}

void AwayManager::requestAllAway(const QString& reason)
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
        if (server->isConnected()) server->requestAway(reason);
}

void AwayManager::requestAllUnaway()
{
    const QList<Server*> serverList = m_connectionManager->getServerList();

    foreach (Server* server, serverList)
        if (server->isConnected() && server->isAway()) server->requestUnaway();
}

void AwayManager::toggleGlobalAway(bool away)
{
    if (away)
        requestAllAway();
    else
        requestAllUnaway();
}

void AwayManager::updateGlobalAwayAction(bool away)
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    KToggleAction* awayAction = qobject_cast<KToggleAction*>(konvApp->getMainWindow()->actionCollection()->action("toggle_away"));

    if (!awayAction) return;

    if (away)
    {
        const QList<Server*> serverList = m_connectionManager->getServerList();
        int awayCount = 0;

        foreach (Server* server, serverList)
        {
            if (server->isAway())
                awayCount++;
        }

        if (awayCount == serverList.count())
        {
            awayAction->setChecked(true);
            awayAction->setIcon(KIcon("im-user-away"));
        }
    }
    else
    {
        awayAction->setChecked(false);
        awayAction->setIcon(KIcon("im-user"));
    }
}

#include "awaymanager.moc"
