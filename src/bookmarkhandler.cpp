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

#include "bookmarkhandler.h"
#include "application.h"
#include "mainwindow.h"
#include "connectionmanager.h"
#include "viewer/viewcontainer.h"

#include <KBookmarkMenu>
#include <QStandardPaths>


KonviBookmarkHandler::KonviBookmarkHandler(QMenu *menu, MainWindow* mainWindow)
: QObject(mainWindow),
KBookmarkOwner(),
m_mainWindow(mainWindow)
{
    setObjectName(QStringLiteral("KonviBookmarkHandler"));

    m_file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/bookmarks.xml") );

    if ( m_file.isEmpty() )
        m_file = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + QStringLiteral("konversation/bookmarks.xml") ;

    KBookmarkManager *manager = KBookmarkManager::managerForFile( m_file, QStringLiteral("konversation"));
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
    Application* konvApp = Application::instance();
    konvApp->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, bm.url().url());
}

QUrl KonviBookmarkHandler::currentUrl() const
{
    return QUrl(m_mainWindow->getViewContainer()->currentViewURL(true));
}

QString KonviBookmarkHandler::currentTitle() const
{
    return m_mainWindow->getViewContainer()->currentViewTitle();
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

bool KonviBookmarkHandler::supportsTabs() const
{
    return true;
}

QList<KBookmarkOwner::FutureBookmark> KonviBookmarkHandler::currentBookmarkList() const
{
    QList<KBookmarkOwner::FutureBookmark> list;
    QPair<QString, QString> uri;

    foreach (uri, m_mainWindow->getViewContainer()->getChannelsURI()) {
        list << KBookmarkOwner::FutureBookmark(uri.first, QUrl(uri.second), QString());
    }

    return list;
}

void KonviBookmarkHandler::openFolderinTabs(const KBookmarkGroup &group)
{
    const QList<QUrl> list = group.groupUrlList();

    Application* konvApp = Application::instance();
    konvApp->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, list);
}


