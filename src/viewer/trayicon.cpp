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
        hideAssociatedWidget();
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


