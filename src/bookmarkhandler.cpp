/*
  Copyright (c) 2005 by İsmail Dönmez <ismail@kde.org>

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

#include "bookmarkhandler.h" ////// header renamed
#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "connectionmanager.h"
// #include "bookmarkmenu.h" ////// header renamed

#include <qstring.h>

#include <kmenu.h>
#include <kstandarddirs.h>
#include <KXMLGUIFactory>
#include <kbookmarkmenu.h>


KonviBookmarkHandler::KonviBookmarkHandler(KonversationMainWindow* mainWindow)
: QObject( mainWindow, "KonviBookmarkHandler" ),
KBookmarkOwner(),
m_mainWindow(mainWindow)
{
    m_menu = static_cast<KMenu*>(mainWindow->factory()->container("bookmarks", mainWindow));

    m_file = KStandardDirs::locate( "data", "konversation/bookmarks.xml" );

    if ( m_file.isEmpty() )
        m_file = KStandardDirs::locateLocal( "data", "konversation/bookmarks.xml" );

    if(!m_menu)
    {
        m_bookmarkMenu = 0;
        return;
    }

    KBookmarkManager *manager = KBookmarkManager::managerForFile( m_file, "konversation");
    manager->setEditorOptions(i18n("Konversation Bookmarks Editor"), false);
    manager->setUpdate( true );
    //manager->setShowNSBookmarks( false );

    connect( manager, SIGNAL(changed(const QString &,const QString &)), SLOT(slotBookmarksChanged(const QString &,const QString &)));

    m_bookmarkMenu = new KBookmarkMenu(manager, this, m_menu, m_mainWindow->actionCollection());
}

KonviBookmarkHandler::~KonviBookmarkHandler()
{
    delete m_bookmarkMenu;
}

void KonviBookmarkHandler::slotBookmarksChanged( const QString &,
const QString &)
{
    // This is called when someone changes bookmarks in konversation
    m_bookmarkMenu->slotBookmarksChanged("");
}

void KonviBookmarkHandler::openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km)
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    konvApp->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, bm.url().url());
}

QString KonviBookmarkHandler::currentUrl() const
{
    return m_mainWindow->currentURL(true);
}

QString KonviBookmarkHandler::currentTitle() const
{
    return m_mainWindow->currentTitle();
}

bool KonviBookmarkHandler::enableOption(BookmarkOption option) const
{
    switch (option)
    {
        case ShowAddBookmark:
        case ShowEditBookmark:
            return true;
    }
    return false;
}

// #include "./bookmarkhandler.moc"
