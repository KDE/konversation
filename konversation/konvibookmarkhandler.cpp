/*
  Copyright (c) 2005 by İsmail Dönmez <ismail@kde.org.tr>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************
  
  Based on the code by:
  Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

*/

#include "konvibookmarkhandler.h"

#include <qstring.h>

#include <kpopupmenu.h>
#include <kstandarddirs.h>

#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "konvibookmarkmenu.h"

KonviBookmarkHandler::KonviBookmarkHandler(KonversationMainWindow* mainWindow)
    : QObject( mainWindow, "KonviBookmarkHandler" ),
      KBookmarkOwner(),
      m_mainWindow(mainWindow)
{
  m_menu = static_cast<KPopupMenu*>(mainWindow->factory()->container("bookmarks", mainWindow));

  m_file = locate( "data", "konversation/bookmarks.xml" );
  if ( m_file.isEmpty() )
    m_file = locateLocal( "data", "konversation/bookmarks.xml" );
  
  KBookmarkManager *manager = KBookmarkManager::managerForFile( m_file, false);
  manager->setEditorOptions(kapp->caption(), false);
  manager->setUpdate( true );
  manager->setShowNSBookmarks( false );
  
  connect( manager, SIGNAL(changed(const QString &,const QString &)), SLOT(slotBookmarksChanged(const QString &,const QString &)));
  
  m_bookmarkMenu = new KonviBookmarkMenu( manager, this, m_menu,  mainWindow->actionCollection(), true );
}

KonviBookmarkHandler::~KonviBookmarkHandler()
{
    delete m_bookmarkMenu;
}

void KonviBookmarkHandler::slotEditBookmarks()
{
    KProcess proc;
    proc << QString::fromLatin1("keditbookmarks");
    proc << "--nobrowser";
    proc << "--caption" << i18n("Konversation Bookmarks Editor");
    proc << m_file;
    proc.start(KProcess::DontCare);
}

void KonviBookmarkHandler::slotBookmarksChanged( const QString &,
						 const QString &)
{
    // This is called when someone changes bookmarks in konversation
    m_bookmarkMenu->slotBookmarksChanged("");
}

void KonviBookmarkHandler::openBookmarkURL(const QString& url, const QString& title)
{
  emit openURL(url,title);
}

QString KonviBookmarkHandler::currentURL() const
{
  return m_mainWindow->currentURL();
}

QString KonviBookmarkHandler::currentTitle() const
{
  return m_mainWindow->currentTitle();
}

#include "konvibookmarkhandler.moc"
