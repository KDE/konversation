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
#include "application.h"
#include "mainwindow.h"
#include "viewcontainer.h"
#include "trayicon.h"
#include "server.h"

#include <QTextDocument>

#include <KNotification>


namespace Konversation
{

    NotificationHandler::NotificationHandler(Application* parent)
        : QObject(parent)
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

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        QString cleanedMessage = removeIrcMarkup(message);
        QString forKNotify = Qt::escape(cleanedMessage);

        KNotification::event(QString::fromLatin1("message"), QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(forKNotify), QPixmap(), m_mainWindow);

        if(!Preferences::self()->trayNotifyOnlyOwnNick())
        {
            startTrayNotification(chatWin);
        }


        if(Preferences::self()->oSDShowChannel() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            Application* konvApp = static_cast<Application*>(kapp);
            konvApp->osd->show('(' + chatWin->getName() + ") <" + fromNick + "> " + cleanedMessage);
        }

    }

    void NotificationHandler::nick(ChatWindow* chatWin, const QString& fromNick, const QString& message)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        QString cleanedMessage = removeIrcMarkup(message);
        QString forKNotify = Qt::escape(cleanedMessage);

        KNotification::event(QString::fromLatin1("nick"), QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(forKNotify), QPixmap(), m_mainWindow);

        startTrayNotification(chatWin);

        Application* konvApp = static_cast<Application*>(kapp);

        if((Preferences::self()->oSDShowChannel() || Preferences::self()->oSDShowOwnNick()) &&
            (!m_mainWindow->isActiveWindow() ||
            (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            konvApp->osd->show(i18n("[HighLight] (%1) &lt;%2&gt; %3",chatWin->getName(),fromNick,cleanedMessage));
        }
    }

    void NotificationHandler::queryMessage(ChatWindow* chatWin,
                                           const QString& fromNick, const QString& message)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        QString cleanedMessage = removeIrcMarkup(message);
        QString forKNotify = Qt::escape(cleanedMessage);

        KNotification::event(QString::fromLatin1("queryMessage"), QString("<qt>&lt;%1&gt; %2</qt>").arg(fromNick).arg(forKNotify), QPixmap(), m_mainWindow);

        startTrayNotification(chatWin);

        Application* konvApp = static_cast<Application*>(kapp);

        if(Preferences::self()->oSDShowQuery() && (!m_mainWindow->isActiveWindow() ||
           (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            konvApp->osd->show(i18n("[Query] &lt;%1&gt; %2",fromNick,cleanedMessage));
        }
    }

    void NotificationHandler::startTrayNotification(ChatWindow* chatWin)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (!chatWin->getServer() || (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer()->isAway()))
            return;

        if (!m_mainWindow->isActiveWindow() && chatWin->getServer()->isConnected() && m_mainWindow->systemTrayIcon())
            m_mainWindow->systemTrayIcon()->startNotification();

    }

    void NotificationHandler::join(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("join"), i18n("%1 joined %2",nick, chatWin->getName()), QPixmap(), m_mainWindow);

        // OnScreen Message
        if(Preferences::self()->oSDShowChannelEvent() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            Application* konvApp = static_cast<Application*>(kapp);
            konvApp->osd->show(i18n("%1 joined %2",nick, chatWin->getName()));
        }
    }

    void NotificationHandler::part(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("part"), i18n("%1 parted %2",nick, chatWin->getName()), QPixmap(), m_mainWindow);

        // OnScreen Message
        if(Preferences::self()->oSDShowChannelEvent() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            Application* konvApp = static_cast<Application*>(kapp);
            konvApp->osd->show(i18n("%1 parted %2",nick, chatWin->getName()));
        }
    }

    void NotificationHandler::quit(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("part"), i18n("%1 quit %2",nick, chatWin->getServer()->getServerName()), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::nickChange(ChatWindow* chatWin, const QString& oldNick, const QString& newNick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("nickchange"), i18n("%1 changed nickname to %2",oldNick, newNick), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::dccIncoming(ChatWindow* chatWin, const QString& fromNick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("dcc_incoming"), i18n("%1 wants to send a file to you",fromNick), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::dccError(ChatWindow* chatWin, const QString& error)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("dcc_error"), i18n("An Error has occurred in a DCC transfer: %1",error), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::dccTransferDone(ChatWindow* chatWin, const QString& file)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("dcctransfer_done"), i18nc("%1 - filename","%1 File Transfer is complete",file), QPixmap(), m_mainWindow);
    }

    void NotificationHandler::mode(ChatWindow* chatWin, const QString& /*nick*/)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification *ev=new KNotification("mode", m_mainWindow);
        ev->sendEvent();
    }

    void NotificationHandler::query(ChatWindow* chatWin, const QString& fromNick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        startTrayNotification(chatWin);

        KNotification *ev=new KNotification("query", m_mainWindow);
        ev->setText(i18n("%1 has started a conversation (query) with you.",fromNick));
        ev->sendEvent();
    }

    void NotificationHandler::nickOnline(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification *ev=new KNotification("notify", m_mainWindow);
        ev->setText(i18n("%1 is online (%2).", nick, chatWin->getServer()->getServerName()));
        ev->sendEvent();

    }

    void NotificationHandler::nickOffline(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification *ev=new KNotification("notify", m_mainWindow);
        ev->setText(i18n("%1 went offline (%2).", nick, chatWin->getServer()->getServerName()));
        ev->sendEvent();

    }

    void NotificationHandler::kick(ChatWindow* chatWin, const QString& channel,const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification *ev=new KNotification("kick", m_mainWindow);
        ev->setText(i18n("You are kicked by %1 from %2", nick, channel));
        ev->sendEvent();

    }

    void NotificationHandler::dccChat(ChatWindow* chatWin, const QString& nick)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification *ev=new KNotification("dccChat", m_mainWindow);
        ev->setText(i18n("%1 started a dcc chat with you", nick));
        ev->sendEvent();

    }

    void NotificationHandler::highlight(ChatWindow* chatWin, const QString& fromNick, const QString& message)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        startTrayNotification(chatWin);

        QString cleanedMessage = removeIrcMarkup(message);
        QString forKNotify = Qt::escape(cleanedMessage);

        if(fromNick.isEmpty())
            KNotification::event(QString::fromLatin1("highlight"), QString("<qt>(%1) *** %2</qt>").arg(chatWin->getName()).arg(forKNotify), QPixmap(), m_mainWindow);
        else
            KNotification::event(QString::fromLatin1("highlight"), QString("<qt>(%1) &lt;%2&gt; %3</qt>").arg(chatWin->getName()).arg(fromNick).arg(forKNotify), QPixmap(), m_mainWindow);

        if(Preferences::self()->oSDShowOwnNick() &&
            (!m_mainWindow->isActiveWindow() || (chatWin != m_mainWindow->getViewContainer()->getFrontView())))
        {
            Application* konvApp = static_cast<Application*>(kapp);
            // if there was no nick associated, this must be a command message, so don't try displaying
            // an empty nick in <>
            if(fromNick.isEmpty())
                konvApp->osd->show(i18n("[HighLight] (%1) *** %2",chatWin->getName(),cleanedMessage));
            // normal highlight message
            else
                konvApp->osd->show(i18n("[HighLight] (%1) &lt;%2&gt; %3",chatWin->getName(),fromNick,cleanedMessage));
        }
    }

    void NotificationHandler::connectionFailure(ChatWindow* chatWin, const QString& server)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        KNotification *ev=new KNotification("connectionFailure", m_mainWindow);
        ev->setText(i18n("Failed to connect to %1", server));
        ev->sendEvent();

    }

    void NotificationHandler::channelJoin(ChatWindow* chatWin, const QString& channel)
    {
        if (!chatWin || !chatWin->notificationsEnabled())
            return;

        if (Preferences::self()->disableNotifyWhileAway() && chatWin->getServer() && chatWin->getServer()->isAway())
            return;

        KNotification::event(QString::fromLatin1("channelJoin"), i18n("You have joined %1.",channel), QPixmap(), m_mainWindow);
    }

}

#include "notificationhandler.moc"
