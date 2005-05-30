/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Sun May 29 2005
  copyright: (C) 2005 by Isaac Clerencia
  email:     isaac@warp.es
*/

#include <klocale.h>
#include <kwin.h>
#include <kiconloader.h>
#include <kactivelabel.h>
#include <qpixmap.h>
#include <qvbox.h>

#include "popup.h"
#include "trayicon.h"

Popup::Popup(KonversationMainWindow* mainWindow,ChatWindow* view,
  QString message):
  KPassivePopup(mainWindow->systemTrayIcon()->winId()),m_mainWindow(mainWindow),
  m_view(view)
{
  QPixmap pix = KGlobal::iconLoader()->loadIcon("konversation",KIcon::Panel);
  QVBox *vbox = standardView("Konversation",message,pix);

  QString name = m_view->getName();
  QString linktext;
  if(name[0] == '#'){
    linktext = QString(i18n("<a href=\"Chat\">Chat in %1</a>")).arg(name);
  } else {
    linktext = QString(i18n("<a href=\"Chat\">Chat with %1</a>")).arg(name);
  }
  KActiveLabel *link = new KActiveLabel(linktext,vbox);
  QObject::disconnect(link,SIGNAL(linkClicked(const QString &)),
    link,SLOT(openLink(const QString &)));
  QObject::connect(link,SIGNAL(linkClicked(const QString &)),
    this,SLOT(focusTab()));
  QObject::connect(link,SIGNAL(linkClicked(const QString &)),
    this,SLOT(hide()));
  setView(vbox);
  setAutoDelete(true);
  show();
}

Popup::~Popup()
{
}

void Popup::focusTab(){
  if(m_mainWindow->isHidden())
    m_mainWindow->show();
  m_mainWindow->showView(m_view);
  KWin::WindowInfo winInfo = KWin::windowInfo(m_mainWindow->winId());
  int desktop = winInfo.desktop();
  KWin::setCurrentDesktop(winInfo.desktop());
  KWin::activateWindow(m_mainWindow->winId());
}

#include "popup.moc"
