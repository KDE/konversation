/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  trayicon.cpp  -  This class handles the system tray icon
  begin:     Sun Nov 9 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#include "trayicon.h"
#include "konversationapplication.h"

#include <qtimer.h>

#include <kglobal.h>
#include <kiconloader.h>
#include <kwin.h>

#include "server.h"
#include "chatwindow.h"

TrayIcon::TrayIcon(QWidget* parent) : KSystemTray(parent)
{
  m_parent = parent;
  m_notificationEnabled = false;
  m_nomessagePix = loadIcon("konversation");
  m_messagePix = loadIcon("konv_message");
  setPixmap(m_nomessagePix);
  m_widgets.setAutoDelete(false);
  m_blinkTimer = new QTimer(this);
  connect(m_blinkTimer, SIGNAL(timeout()), SLOT(blinkTimeout()));
  if(KonversationApplication::preferences.getShowTrayIcon() &&
     KonversationApplication::preferences.getSystrayOnly())
    KWin::setState(m_parent->winId(), NET::SkipTaskbar);
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::startNotification(QWidget* view)
{
  if(!m_notificationEnabled) {
    return;
  }
  
  if(m_widgets.find(view) == -1) {
    m_widgets.append(view);
  } else {
    return;
  }
  
  if(!m_blinkTimer->isActive() && !m_widgets.isEmpty()) {
    setPixmap(m_messagePix);
    m_blinkOn = true;
    m_blinkTimer->start(500);
  }
}

void TrayIcon::endNotification(QWidget* view)
{
  m_widgets.remove(view);
  
  if(m_blinkTimer->isActive() && m_widgets.isEmpty()) {
    m_blinkTimer->stop();
    setPixmap(m_nomessagePix);
  }
}

void TrayIcon::blinkTimeout()
{
  m_blinkOn = !m_blinkOn;
  
  if(m_blinkOn) {
    setPixmap(m_messagePix);
  } else {
    setPixmap(m_nomessagePix);
  }
}

void TrayIcon::removeServer(Server* server)
{
  for(QWidget* w = m_widgets.first(); w; w = m_widgets.next()) {
    if(static_cast<ChatWindow*>(w)->getServer() == server) {
      endNotification(w);
    }
  }
}

void TrayIcon::mousePressEvent(QMouseEvent *e)
{
  if(KonversationApplication::preferences.getShowTrayIcon() &&
     KonversationApplication::preferences.getSystrayOnly())
    KWin::setState(m_parent->winId(), NET::SkipTaskbar);
  KSystemTray::mousePressEvent(e);
}

#include "trayicon.moc"
