/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "trayicon.h"
#include "application.h"
#include "config/preferences.h"

namespace Konversation
{

    TrayIcon::TrayIcon(QWidget* parent) : KStatusNotifierItem(parent)
    {
        setCategory(Communications);
        setStatus(Active);

        m_notificationEnabled = false;
        
        updateAppearance();
	
	setToolTip("konversation", i18n("Konversation"), i18n("Konversation - IRC Client"));
    }

    TrayIcon::~TrayIcon()
    {
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
        setStatus(Active);
    }
    
    void TrayIcon::updateAppearance()
    {
        m_nomessagePix = "konversation";
        m_messagePix = "konv_message";
        
        setIconByName(m_nomessagePix);
        setAttentionIconByName(m_messagePix);
    }
}

#include "trayicon.moc"
