/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Peter Simonsson <psn@linux.se>
*/

#include "trayicon.h"
#include "application.h"
#include "config/preferences.h"

namespace Konversation
{

    TrayIcon::TrayIcon(QWidget* parent) : KStatusNotifierItem(parent)
    {
        setCategory(Communications);

        m_notificationEnabled = false;
        m_away = false;

        updateAppearance();

        setToolTip(QStringLiteral("konversation"), i18n("Konversation"), i18n("IRC Client"));
    }

    TrayIcon::~TrayIcon()
    {
    }

    void TrayIcon::hideWindow()
    {
        QWidget *window = associatedWidget();
        if (window->isHidden())
            return;
        // hiding via the KStatusNotifierItem also stores any window system info, like "Show on all desktops"
        // TODO: KStatusNotifierItem only hides if not minimized, needs new API in KStatusNotifierItem
        // unminimizing instead as work-around needs to wait until the state is reached, not simple to do
        // For now just doing a plain hide and losing any such info, as after all
        // hiding a minimized window might not be done by many users
        if (window->isMinimized())
        {
            window->hide();
            return;
        }

        // activating when the window is visible hides it
        activate(QPoint());
    }

    void TrayIcon::restoreWindow()
    {
        if (associatedWidget()->isVisible())
            return;
        activate(QPoint());
    }

    void TrayIcon::startNotification()
    {
        if (!m_notificationEnabled)
        {
            return;
        }

        setStatus(NeedsAttention);
    }

    void TrayIcon::endNotification()
    {
        setStatus(Passive);
    }

    void TrayIcon::setAway(bool away)
    {
        m_away = away;

        updateAppearance();
    }

    void TrayIcon::updateAppearance()
    {
        setIconByName(QStringLiteral("konversation"));
        setAttentionIconByName(QStringLiteral("konv_message"));
        setOverlayIconByName(m_away ? QStringLiteral("user-away") : QString());
    }
}


