/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#include "notificationhandler.h"

#include <knotifyclient.h>
#include <kstringhandler.h>
#include <klocale.h>

#include "common.h"
#include "chatwindow.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "trayicon.h"
#include "server.h"

namespace Konversation {

NotificationHandler::NotificationHandler(KonversationApplication* parent, const char* name)
 : QObject(parent, name)
{
  m_mainWindow = parent->getMainWindow();
}

NotificationHandler::~NotificationHandler()
{
}

void NotificationHandler::message(ChatWindow* chatWin, const QString& fromNick, const QString& message)
{
  if(!chatWin->notificationsEnabled()) {
    return;
  }
  
  QString cutup = KStringHandler::rsqueeze(Konversation::removeIrcMarkup(message), 50);
  KNotifyClient::event(m_mainWindow->winId(), "message", QString("<%1> %2").arg(fromNick).arg(cutup));
  
  if(!KonversationApplication::preferences.trayNotifyOnlyOwnNick()) {
    startTrayNotification(chatWin);
  }

  KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
  
  if(KonversationApplication::preferences.getOSDShowChannel()) {
    konvApp->osd->showOSD("(" + chatWin->getName() + ") <" + fromNick + "> " + message);
  }
}

void NotificationHandler::nick(ChatWindow* chatWin, const QString& fromNick, const QString& message)
{
  if(!chatWin->notificationsEnabled()) {
    return;
  }
  
  QString cutup = KStringHandler::rsqueeze(Konversation::removeIrcMarkup(message), 50);
  KNotifyClient::event(m_mainWindow->winId(), "nick", QString("<%1> %2").arg(fromNick).arg(cutup));
  
  startTrayNotification(chatWin);

  KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
  
  if(chatWin->getType() == ChatWindow::Channel) {
    if(KonversationApplication::preferences.getOSDShowChannel() ||
      KonversationApplication::preferences.getOSDShowOwnNick())
    {
      konvApp->osd->showOSD(i18n("[HighLight] (%1) <%2> %3").arg(chatWin->getName()).arg(fromNick).arg(message));
    }
  } else if(chatWin->getType() == ChatWindow::Query) {
    if(KonversationApplication::preferences.getOSDShowQuery()) {
      konvApp->osd->showOSD(i18n("(Query) <%1> %2").arg(fromNick).arg(message));
    }
  }
}

void NotificationHandler::startTrayNotification(ChatWindow* chatWin)
{
  if(!m_mainWindow->isActiveWindow() && chatWin->getServer() && chatWin->getServer()->connected())
  {
    m_mainWindow->systemTrayIcon()->startNotification();
  }
}

}

#include "notificationhandler.moc"
