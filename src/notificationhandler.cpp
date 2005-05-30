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
#include "popup.h"
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
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }

  QString cleanedMessage = Konversation::removeIrcMarkup(message);
  QString cutup = addLineBreaks(cleanedMessage);

  KNotifyClient::event(winId(), "message", QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(cutup));
//   Popup *pop = new Popup(m_mainWindow,chatWin,
//     QString("<qt>&lt;%2&gt; %3</qt>").arg(chatWin->getName()).arg(fromNick).arg(cutup));

  if(!KonversationApplication::preferences.trayNotifyOnlyOwnNick()) {
    startTrayNotification(chatWin);
  }

  if(KonversationApplication::preferences.getOSDShowChannel() &&
     (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->frontView())))
  {
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    konvApp->osd->showOSD("(" + chatWin->getName() + ") <" + fromNick + "> " + cleanedMessage);
  }
}

void NotificationHandler::nick(ChatWindow* chatWin, const QString& fromNick, const QString& message)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }

  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }

  QString cleanedMessage = Konversation::removeIrcMarkup(message);
  QString cutup = addLineBreaks(cleanedMessage);

  KNotifyClient::event(winId(), "nick", QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(cutup));
//   Popup *pop = new Popup(m_mainWindow,chatWin,
//     QString("<qt>&lt;%2&gt; %3</qt>").arg(chatWin->getName()).arg(fromNick).arg(cutup));

  startTrayNotification(chatWin);

  KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
  
  if(chatWin->getType() == ChatWindow::Channel) {
    if((KonversationApplication::preferences.getOSDShowChannel() ||
       KonversationApplication::preferences.getOSDShowOwnNick()) &&
       (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->frontView())))
    {
      konvApp->osd->showOSD(i18n("[HighLight] (%1) <%2> %3").arg(chatWin->getName()).arg(fromNick).arg(cleanedMessage));
    }
  } else if(chatWin->getType() == ChatWindow::Query) {
    if(KonversationApplication::preferences.getOSDShowQuery() &&
       (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->frontView())))
    {
      konvApp->osd->showOSD(i18n("(Query) <%1> %2").arg(fromNick).arg(cleanedMessage));
    }
  }
}

void NotificationHandler::startTrayNotification(ChatWindow* chatWin)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  if(!m_mainWindow->isActiveWindow() && chatWin->getServer() && chatWin->getServer()->connected())
  {
    m_mainWindow->systemTrayIcon()->startNotification();
  }
}

void NotificationHandler::join(ChatWindow* chatWin, const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "join", i18n("%1 joined %2").arg(nick, chatWin->getName()));

  // OnScreen Message
  if(KonversationApplication::preferences.getOSDShowChannelEvent() &&
     (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->frontView())))
  {
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    konvApp->osd->showOSD(i18n("%1 joined %2").arg(nick, chatWin->getName()));
  }
}

void NotificationHandler::part(ChatWindow* chatWin, const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "part", i18n("%1 parted %2").arg(nick, chatWin->getName()));

  // OnScreen Message
  if(KonversationApplication::preferences.getOSDShowChannelEvent() &&
     (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->frontView())))
  {
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    konvApp->osd->showOSD(i18n("%1 parted %2").arg(nick, chatWin->getName()));
  }
}

int NotificationHandler::winId() const
{
  if(m_mainWindow->systemTrayIcon() && m_mainWindow->systemTrayIcon()->isShown()) {
    return m_mainWindow->systemTrayIcon()->winId();
  }
  
  return m_mainWindow->winId();
}

void NotificationHandler::quit(ChatWindow* chatWin, const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "part", i18n("%1 quit %2").arg(nick, chatWin->getServer()->getServerName()));
}

void NotificationHandler::nickChange(ChatWindow* chatWin, const QString& oldNick, const QString& newNick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "nickchange", i18n("%1 changed nickname to %2").arg(oldNick, newNick));
}

void NotificationHandler::dccIncoming(ChatWindow* chatWin, const QString& fromNick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "dcc_incoming", i18n("%1 wants to send a file to you").arg(fromNick));
}

void NotificationHandler::mode(ChatWindow* chatWin, const QString& /*nick*/)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "mode");
}

void NotificationHandler::query(ChatWindow* chatWin, const QString& fromNick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "query",
    i18n("%1 has started a conversation (query) with you.").arg(fromNick));
}

void NotificationHandler::nickOnline(ChatWindow* chatWin, const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "notify",
    i18n("%1 is online (%2).").arg(nick).arg(chatWin->getServer()->getServerName()));
}

void NotificationHandler::nickOffline(ChatWindow* chatWin, const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  if(KonversationApplication::preferences.disableNotifyWhileAway() && chatWin->getServer()->isAway()) {
    return;
  }
  
  KNotifyClient::event(winId(), "notify",
    i18n("%1 went offline (%2).").arg(nick).arg(chatWin->getServer()->getServerName()));
}

void NotificationHandler::kick(ChatWindow* chatWin, const QString& channel,const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }

  KNotifyClient::event(winId(), "kick",
    i18n("You are kicked by %1 from %2").arg(nick).arg(channel));
}

void NotificationHandler::dccChat(ChatWindow* chatWin, const QString& nick)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }
  
  KNotifyClient::event(winId(), "dccChat",
    i18n("%1 started a dcc chat with you").arg(nick));
}

void NotificationHandler::highlight(ChatWindow* chatWin, const QString& fromNick, const QString& message)
{
  if(!chatWin || !chatWin->notificationsEnabled()) {
    return;
  }

  if(KonversationApplication::preferences.getOSDShowOwnNick() &&
     (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->frontView())))
  {
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    konvApp->osd->showOSD(i18n("[HighLight] (%1) <%2> %3").arg(chatWin->getName()).arg(fromNick).arg(message));
  }
}

QString NotificationHandler::addLineBreaks(const QString& string)
{
  QString cutup = string;
  int offset = 0;

  for(int i = 0; i < string.length(); i += 50) {
    cutup.insert(i + (offset * 4), "<br>");
    ++offset;
  }

  return cutup;
}

}

#include "notificationhandler.moc"
