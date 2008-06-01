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
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "connectionmanager.h"
#include "server.h"
#include "preferences.h"

#include <qtimer.h>

#include <dcopref.h>
#include <kaction.h>
#include <klocale.h>

#include <kdebug.h>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef HAVE_XSCREENSAVER
#define HasScreenSaver
#include <X11/extensions/scrnsaver.h>
#endif
#endif

// Don't use XIdle for now, it's experimental.
#undef HAVE_XIDLE
#undef HasXidle


struct AwayManagerPrivate
{
    int mouseX;
    int mouseY;
    unsigned int mouseMask;
#ifdef Q_WS_X11
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

#ifdef Q_WS_X11
    Display* display = qt_xdisplay();
    d->root = DefaultRootWindow(display);
    d->screen = ScreenOfDisplay(display, DefaultScreen (display));

    d->xIdleTime = 0;
#endif

#ifdef HasXidle
    d->useXidle = XidleQueryExtension(qt_xdisplay(), &dummy, &dummy);
#endif

#ifdef HasScreenSaver
    if (!d->useXidle)
        d->useMit = XScreenSaverQueryExtension(qt_xdisplay(), &dummy, &dummy);
#endif

    m_activityTimer = new QTimer(this, "AwayTimer");
    connect(m_activityTimer, SIGNAL(timeout()), this, SLOT(checkActivity()));

    m_idleTime.start();
}

AwayManager::~AwayManager()
{
}

void AwayManager::identitiesChanged()
{
    kdDebug() << "AwayManager - identitiesChanged() - Updating list of active identities with auto-away enabled." << endl;

    QValueList<int> newIdentityList;

    QPtrList<Server> serverList = m_connectionManager->getServerList();
    Server* server = 0;

    for (server = serverList.first(); server; server = serverList.next())
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
    kdDebug() << "AwayManager - identityOnline() - Id: " << identityId << endl;

    IdentityPtr identity = Preferences::identityById(identityId);

    if (identity && identity->getAutomaticAway() &&
        !m_identitiesOnAutoAway.contains(identityId))
    {
        kdDebug() << "AwayManager - identityOnline() - Adding identity " << identityId << " to list." << endl;

        m_identitiesOnAutoAway.append(identityId);

        toggleTimer();
    }
    else
        kdDebug() << "AwayManager - identityOnline() - Identity " << identityId << " is not managed." << endl;
}

void AwayManager::identityOffline(int identityId)
{
    kdDebug() << "AwayManager - identityOffline() - Id: " << identityId << endl;

    if (m_identitiesOnAutoAway.contains(identityId))
    {
        kdDebug() << "AwayManager - identityOnline() - Removing identity " << identityId << " from list." << endl;

        m_identitiesOnAutoAway.remove(identityId);

        toggleTimer();
    }
    else
        kdDebug() << "AwayManager - identityOnline() - Identity " << identityId << " was not managed." << endl;
}

void AwayManager::toggleTimer()
{
    kdDebug() << "AwayManager - toggleTimer() - (Re-)checking whether to start or stop activity polling." << endl;

    if (m_identitiesOnAutoAway.count() > 0)
    {
        if (!m_activityTimer->isActive())
        {
            kdDebug() << "AwayManager - toggleTimer() - Starting timer." << endl;

            m_activityTimer->start(Preferences::autoAwayPollInterval() * 1000);
        }
        else
            kdDebug() << "AwayManager - toggleTimer() - Timer already running." << endl;
    }
    else if (m_activityTimer->isActive())
    {
        kdDebug() << "AwayManager - toggleTimer() - Stopping timer." << endl;

        m_activityTimer->stop();
    }
}

void AwayManager::checkActivity()
{
    kdDebug() << "AwayManager - checkActivity() - Checking for activity." << endl;

    // Allow the event loop to be called, to avoid deadlock.
    static bool rentrencyProtection = false;
    if (rentrencyProtection) return;

    rentrencyProtection = true;

    DCOPRef screenSaver("kdesktop", "KScreensaverIface");
    DCOPReply isBlanked = screenSaver.callExt("isBlanked", DCOPRef::UseEventLoop, 10);

    rentrencyProtection = false;

    if (!(isBlanked.isValid() && isBlanked.type == "bool" && ((bool)isBlanked)))
    {
         kdDebug() << "AwayManager - checkActivity() - Using X method (screenserver not blanked, or DCOP error)." << endl;

         implementIdleAutoAway(Xactivity());
    }
    else
        kdDebug() << "AwayManager - checkActivity() - Screensaver is blanked: Should be away already." << endl;
}

bool AwayManager::Xactivity()
{
    bool activity = false;

#ifdef Q_WS_X11
    Display* display = qt_xdisplay();
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

    kdDebug() << "AwayManager - Xactivity() - Activity: " << activity << "." << endl;

    return activity;
}

void AwayManager::implementIdleAutoAway(bool activity)
{
    if (activity)
    {
        kdDebug() << "AwayManager - implementIdleAutoAway() - Setting all managed identities with auto-return enabled unaway as necessary." << endl;

        m_idleTime.start();

        QPtrList<Server> serverList = m_connectionManager->getServerList();
        Server* server = 0;

        for (server = serverList.first(); server; server = serverList.next())
        {
            IdentityPtr identity = server->getIdentity();

            if (m_identitiesOnAutoAway.contains(identity->id()) && identity->getAutomaticUnaway()
                && server->isConnected() && server->isAway())
            {
                kdDebug() << "AwayManager - implementIdleAutoAway() - Setting connection " << server->connectionId() << " (identity: " << identity->id() << ") unaway." << endl;

                server->requestUnaway();
            }
        }
    }
    else
    {
        kdDebug() << "AwayManager - implementIdleAutoAway() - Setting managed identities with exceeded time limits away as necessary." << endl;

        long int idleTime = m_idleTime.elapsed() / 1000;

        kdDebug() << "AwayManager - implementIdleAutoAway() - Idle time: " << idleTime << " seconds." << endl;

        QValueList<int> identitiesIdleTimeExceeded;
        QValueList<int>::ConstIterator it;

        for (it = m_identitiesOnAutoAway.begin(); it != m_identitiesOnAutoAway.end(); ++it)
        {
            if (idleTime >= Preferences::identityById((*it))->getAwayInactivity() * 60)
                identitiesIdleTimeExceeded.append((*it));
        }

        QPtrList<Server> serverList = m_connectionManager->getServerList();
        Server* server = 0;

        for (server = serverList.first(); server; server = serverList.next())
        {
            int identityId = server->getIdentity()->id();

            if (identitiesIdleTimeExceeded.contains(identityId) && server->isConnected() && !server->isAway())
            {
                kdDebug() << "AwayManager - implementIdleAutoAway() - Time limit for identity " << identityId << " exceeded; setting connection " << server->connectionId() << " away." << endl;

                server->requestAway();
            }
        }
    }
}

void AwayManager::setManagedIdentitiesAway()
{
    kdDebug() << "AwayManager - setManagedIdentitiesAway() - Setting all managed identities away as necessary." << endl;

    // Used to skip X-based activity checking for one round, to avoid jumping
    // on residual mouse activity after manual screensaver activation.
    d->mouseX = -1;

    QPtrList<Server> serverList = m_connectionManager->getServerList();
    Server* server = 0;

    for (server = serverList.first(); server; server = serverList.next())
    {
        if (m_identitiesOnAutoAway.contains(server->getIdentity()->id()) && server->isConnected() && !server->isAway())
            server->requestAway();
    }
}

void AwayManager::setManagedIdentitiesUnaway()
{
    kdDebug() << "AwayManager - setManagedIdentitiesUnaway() - Setting all managed identities with auto-return enabled unaway as necessary." << endl;

    QPtrList<Server> serverList = m_connectionManager->getServerList();
    Server* server = 0;

    for (server = serverList.first(); server; server = serverList.next())
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
    QPtrList<Server> serverList = m_connectionManager->getServerList();
    Server* server = 0;

    for (server = serverList.first(); server; server = serverList.next())
        if (server->isConnected()) server->requestAway(reason);
}

void AwayManager::requestAllUnaway()
{
    QPtrList<Server> serverList = m_connectionManager->getServerList();
    Server* server = 0;

    for (server = serverList.first(); server; server = serverList.next())
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
    KToggleAction* awayAction = static_cast<KToggleAction*>(konvApp->getMainWindow()->actionCollection()->action("toggle_away"));

    if (!awayAction) return;

    if (away)
    {
        QPtrList<Server> serverList = m_connectionManager->getServerList();
        Server* server = 0;
        uint awayCount = 0;

        for (server = serverList.first(); server; server = serverList.next())
        {
            if (server->isAway())
                awayCount++;
        }

        if (awayCount == serverList.count())
        {
            awayAction->setChecked(true);
            awayAction->setIcon("konversationaway");
        }
    }
    else
    {
        awayAction->setChecked(false);
        awayAction->setIcon("konversationavailable");
    }
}

#include "awaymanager.moc"
