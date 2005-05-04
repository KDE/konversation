/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  RawLog.cpp  -  provides a view to the raw protocol
  begin:     Die Mär 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <klocale.h>
#include <kdebug.h>

#include "rawlog.h"
#include "channel.h"
#include "ircview.h"
#include "server.h"
#include "konversationapplication.h"

#ifdef USE_MDI
RawLog::RawLog(QString caption) : ChatWindow(caption)
#else
RawLog::RawLog(QWidget* parent) : ChatWindow(parent)
#endif
{
  setName(i18n("Raw Log"));
  setType(ChatWindow::RawLog);
  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer()
}

RawLog::~RawLog()
{
}

void RawLog::childAdjustFocus()
{
}

void RawLog::updateFonts()
{
  getTextView()->setFont(KonversationApplication::preferences.getTextFont());

  if(KonversationApplication::preferences.getShowBackgroundImage()) {
    getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                  KonversationApplication::preferences.getBackgroundImageName());
  } else {
    getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
      QString::null);
  }
}

bool RawLog::closeYourself()
{
#ifndef USE_MDI
  // make the server delete us so server can reset the pointer to us
  m_server->closeRawLog();
#endif
  return true;
}

#ifdef USE_MDI
void RawLog::closeYourself(ChatWindow*)
{
  // make the server delete us so server can reset the pointer to us
  m_server->closeRawLog();
  emit chatWindowCloseRequest(this);
}
#endif

void RawLog::serverQuit(const QString&)
{
#ifdef USE_MDI
  closeYourself(this);
#endif
}

bool RawLog::searchView() { return true; }

#include "rawlog.moc"
