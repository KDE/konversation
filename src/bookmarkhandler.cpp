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

#include <kstandarddirs.h>
#include <KXMLGUIFactory>
#include <kbookmarkmenu.h>


KonviBookmarkHandler::KonviBookmarkHandler(KMenu *menu, KonversationMainWindow* mainWindow)
: QObject(mainWindow),
KBookmarkOwner(),
m_mainWindow(mainWindow)
{
    setObjectName("KonviBookmarkHandler");

    m_file = KStandardDirs::locate( "data", "konversation/bookmarks.xml" );

    if ( m_file.isEmpty() )
        m_file = KStandardDirs::locateLocal( "data", "konversation/bookmarks.xml" );

    KBookmarkManager *manager = KBookmarkManager::managerForFile( m_file, "konversation");
    manager->setEditorOptions(i18n("Konversation Bookmarks Editor"), false);
    manager->setUpdate( true );

    m_bookmarkMenu = new KBookmarkMenu(manager, this, menu, m_mainWindow->actionCollection());
}

KonviBookmarkHandler::~KonviBookmarkHandler()
{
    delete m_bookmarkMenu;
}

void KonviBookmarkHandler::openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km)
{
    Q_UNUSED(mb);
    Q_UNUSED(km);
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

#include "bookmarkhandler.moc"
