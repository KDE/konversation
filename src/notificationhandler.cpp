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
#include "common.h"
#include "chatwindow.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "viewcontainer.h"
#include "trayicon.h"
#include "server.h"

#include <q3stylesheet.h>

#include <knotification.h>
#include <kstringhandler.h>
#include <klocale.h>
#include <QTextDocument>


namespace Konversation
{

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
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        QString cleanedMessage = Qt::escape(Konversation::removeIrcMarkup(message));
        QString cutup = addLineBreaks(cleanedMessage);

        KNotification::event(QString::fromLatin1("message"), QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(cutup), QPixmap(), m_mainWindow);

        if(!Preferences::trayNotifyOnlyOwnNick())
        {
            startTrayNotification(chatWin);
        }

        if(Preferences::oSDShowChannel() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
            konvApp->osd->showOSD('(' + chatWin->getName() + ") <" + fromNick + "> " + cleanedMessage);
        }
    }

    void NotificationHandler::nick(ChatWindow* chatWin, const QString& fromNick, const QString& message)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        QString cleanedMessage = Qt::escape(Konversation::removeIrcMarkup(message));
        QString cutup = addLineBreaks(cleanedMessage);

        KNotification::event(QString::fromLatin1("nick"), QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(cutup), QPixmap(), m_mainWindow);

        startTrayNotification(chatWin);

        KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

        if((Preferences::oSDShowChannel() || Preferences::oSDShowOwnNick()) &&
            (!m_mainWindow->isActiveWindow() ||
            (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            konvApp->osd->showOSD(i18n("[HighLight] (%1) <%2> %3").arg(chatWin->getName()).arg(fromNick).arg(cleanedMessage));
        }
    }

    void NotificationHandler::queryMessage(ChatWindow* chatWin,
                                           const QString& fromNick, const QString& message)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        QString cleanedMessage = Qt::escape(Konversation::removeIrcMarkup(message));
        QString cutup = addLineBreaks(cleanedMessage);

        KNotification::event(QString::fromLatin1("queryMessage"), QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(cutup), QPixmap(), m_mainWindow);

        startTrayNotification(chatWin);

        KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

        if(Preferences::oSDShowQuery() && (!m_mainWindow->isActiveWindow() ||
           (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            konvApp->osd->showOSD(i18n("[Query] <%1> %2").arg(fromNick).arg(cleanedMessage));
        }
    }

    void NotificationHandler::startTrayNotification(ChatWindow* chatWin)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (!chatWin->getServer() || (Preferences::disableNotifyWhileAway() && chatWin->getServer()->isAway()))
            return;

        if (!m_mainWindow->isActiveWindow() && chatWin->getServer()->isConnected())
            m_mainWindow->systemTrayIcon()->startNotification();
    }

    void NotificationHandler::join(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("join"), i18n("%1 joined %2").arg(nick, chatWin->getName()), QPixmap(), m_mainWindow);

        // OnScreen Message
        if(Preferences::oSDShowChannelEvent() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
            konvApp->osd->showOSD(i18n("%1 joined %2").arg(nick, chatWin->getName()));
        }
    }

    void NotificationHandler::part(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("part"), i18n("%1 parted %2").arg(nick, chatWin->getName()), QPixmap(), m_mainWindow);

        // OnScreen Message
        if(Preferences::oSDShowChannelEvent() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
            konvApp->osd->showOSD(i18n("%1 parted %2").arg(nick, chatWin->getName()));
        }
    }

    void NotificationHandler::quit(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("part"), i18n("%1 quit %2").arg(nick, chatWin->getServer()->getServerName()), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::nickChange(ChatWindow* chatWin, const QString& oldNick, const QString& newNick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("nickchange"), i18n("%1 changed nickname to %2").arg(oldNick, newNick), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::dccIncoming(ChatWindow* chatWin, const QString& fromNick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("dcc_incoming"), i18n("%1 wants to send a file to you").arg(fromNick), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::mode(ChatWindow* chatWin, const QString& /*nick*/)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(m_mainWindow->winId(), "mode");
    }

    void NotificationHandler::query(ChatWindow* chatWin, const QString& fromNick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        startTrayNotification(chatWin);

        KNotification::event(m_mainWindow->winId(), "query",
            i18n("%1 has started a conversation (query) with you.").arg(fromNick));
    }

    void NotificationHandler::nickOnline(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(m_mainWindow->winId(), "notify",
            i18n("%1 is online (%2).").arg(nick).arg(chatWin->getServer()->getServerName()));
    }

    void NotificationHandler::nickOffline(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(m_mainWindow->winId(), "notify",
            i18n("%1 went offline (%2).").arg(nick).arg(chatWin->getServer()->getServerName()));
    }

    void NotificationHandler::kick(ChatWindow* chatWin, const QString& channel,const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(m_mainWindow->winId(), "kick",
            i18n("You are kicked by %1 from %2").arg(nick).arg(channel));
    }

    void NotificationHandler::dccChat(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(m_mainWindow->winId(), "dccChat",
            i18n("%1 started a dcc chat with you").arg(nick));
    }

    void NotificationHandler::highlight(ChatWindow* chatWin, const QString& fromNick, const QString& message)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        startTrayNotification(chatWin);

        if(Preferences::oSDShowOwnNick() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
            // if there was no nick associated, this must be a command message, so don't try displaying
            // an empty nick in <>
            if(fromNick.isEmpty())
                konvApp->osd->showOSD(i18n("[HighLight] (%1) *** %2").arg(chatWin->getName()).arg(message));
            // normal highlight message
            else
                konvApp->osd->showOSD(i18n("[HighLight] (%1) <%2> %3").arg(chatWin->getName()).arg(fromNick).arg(message));
        }
    }

    void NotificationHandler::connectionFailure(ChatWindow* chatWin, const QString& server)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        KNotification::event(m_mainWindow->winId(), "connectionFailure",
            i18n("Failed to connect to %1").arg(server));
    }

    void NotificationHandler::channelJoin(ChatWindow* chatWin, const QString& channel)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("channelJoin"), i18n("You have joined %1.").arg(channel), QPixmap(), m_mainWindow);
    }

    QString NotificationHandler::addLineBreaks(const QString& string)
    {
        QString cutup = string;
        int offset = 0;

        for(uint i = 0; i < string.length(); i += 50)
        {
            cutup.insert(i + (offset * 4), "<br>");
            ++offset;
        }

        return cutup;
    }

}

#include "notificationhandler.moc"
