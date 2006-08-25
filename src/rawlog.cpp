/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  RawLog.cpp  -  provides a view to the raw protocol
  begin:     Tue Mar 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <klocale.h>
#include <kdebug.h>

#include "rawlog.h"
#include "channel.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "server.h"
#include "konversationapplication.h"

RawLog::RawLog(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("Raw Log"));
    setType(ChatWindow::RawLog);
    IRCViewBox* ircBox = new IRCViewBox(this, 0);
    setTextView(ircBox->ircView());               // Server will be set later in setServer()

    connect(getTextView(), SIGNAL(updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT(morphNotification()));
}

RawLog::~RawLog()
{
}

void RawLog::childAdjustFocus()
{
}

void RawLog::updateAppearance()
{
    getTextView()->unsetPalette();

    if(Preferences::showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            Preferences::backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            QString::null);
    }

    if (Preferences::customTextFont())
        getTextView()->setFont(Preferences::textFont());
    else
        getTextView()->setFont(KGlobalSettings::generalFont());

    ChatWindow::updateAppearance();
}

void RawLog::morphNotification()
{
    activateTabNotification(Konversation::tnfSystem);
}

bool RawLog::closeYourself()
{
    // make the server delete us so server can reset the pointer to us
    m_server->closeRawLog();
    return true;
}

bool RawLog::searchView() { return true; }

#include "rawlog.moc"
