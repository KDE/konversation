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

#include "bookmarkmenu.h" ////// header renamed
#include "bookmarkhandler.h" ////// header renamed

#include <qregexp.h>

#include <kmenu.h>
#include <kaction.h>


KonviBookmarkMenu::KonviBookmarkMenu( KBookmarkManager* mgr,
KonviBookmarkHandler * _owner, KMenu * _parentMenu,
KActionCollection *collec, bool _isRoot, bool _add,
const QString & parentAddress )
: KBookmarkMenu( mgr, _owner, _parentMenu, collec, _isRoot, _add, parentAddress),
m_kOwner(_owner)
{
    /*
     * First, we disconnect KBookmarkMenu::slotAboutToShow()
     * Then,  we connect    KonviBookmarkMenu::slotAboutToShow().
     * They are named differently because the SLOT() macro thinks we want
     * KonviBookmarkMenu::KBookmarkMenu::slotAboutToShow()
     * Could this be solved if slotAboutToShow() is virtual in KBookmarMenu?
     */
    disconnect(_parentMenu, SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));
    connect(_parentMenu, SIGNAL(aboutToShow()), SLOT(slotAboutToShow2()));
}

/*
 * Duplicate this exactly because KBookmarkMenu::slotBookmarkSelected can't
 * be overrided.  I would have preferred to NOT have to do this.
 *
 * Why did I do this?
 *   - when KBookmarkMenu::fillbBookmarkMenu() creates sub-KBookmarkMenus.
 *   - when ... adds KActions, it uses KBookmarkMenu::slotBookmarkSelected()
 *     instead of KonviBookmarkMenu::slotBookmarkSelected().
 */
void KonviBookmarkMenu::slotAboutToShow2()
{
    // Did the bookmarks change since the last time we showed them ?
    if ( m_bDirty )
    {
        m_bDirty = false;
        refill();
    }
}

void KonviBookmarkMenu::refill()
{
    m_lstSubMenus.clear();
    Q3PtrListIterator<KAction> it( m_actions );
    for (; it.current(); ++it )
        it.current()->unplug( m_parentMenu );
    m_parentMenu->clear();
    m_actions.clear();
    fillBookmarkMenu();
    m_parentMenu->adjustSize();
}

void KonviBookmarkMenu::fillBookmarkMenu()
{
    if ( m_bIsRoot )
    {
        if ( m_bAddBookmark )
            addAddBookmark();

        addEditBookmarks();

        if ( m_bAddBookmark )
            addNewFolder();
    }

    KBookmarkGroup parentBookmark = m_pManager->findByAddress( m_parentAddress ).toGroup();
    Q_ASSERT(!parentBookmark.isNull());
    bool separatorInserted = false;
    for ( KBookmark bm = parentBookmark.first(); !bm.isNull();
        bm = parentBookmark.next(bm) )
    {
        QString text = bm.text();
        text.replace( '&', "&&" );
        if ( !separatorInserted && m_bIsRoot)     // inserted before the first konq bookmark, to avoid the separator if no konq bookmark
        {
            m_parentMenu->insertSeparator();
            separatorInserted = true;
        }
        if ( !bm.isGroup() )
        {
            if ( bm.isSeparator() )
            {
                m_parentMenu->insertSeparator();
            }
            else
            {
                // kDebug(1203) << "Creating URL bookmark menu item for " << bm.text() << endl;
                // create a normal URL item, with ID as a name
                KAction * action = new KAction( text, bm.icon(), 0,
                    this, SLOT( slotBookmarkSelected() ),
                    m_actionCollection, bm.url().url().utf8() );

                action->setStatusText( bm.url().prettyUrl() );

                action->plug( m_parentMenu );
                m_actions.append( action );
            }
        }
        else
        {
            // kDebug(1203) << "Creating bookmark submenu named " << bm.text() << endl;
            KActionMenu * actionMenu = new KActionMenu( text, bm.icon(),
                m_actionCollection, 0L );
            actionMenu->plug( m_parentMenu );
            m_actions.append( actionMenu );
            KonviBookmarkMenu *subMenu = new KonviBookmarkMenu( m_pManager,
                m_kOwner, actionMenu->popupMenu(),
                m_actionCollection, false,
                m_bAddBookmark, bm.address() );
            m_lstSubMenus.append( subMenu );
        }
    }

    if ( !m_bIsRoot && m_bAddBookmark )
    {
        if ( m_parentMenu->count() > 0 )
            m_parentMenu->insertSeparator();
        addAddBookmark();
        addNewFolder();
    }
}

void KonviBookmarkMenu::slotBookmarkSelected()
{
    if ( !m_pOwner ) return;                      // this view doesn't handle bookmarks...
                                                  /* URL */
    m_kOwner->openBookmarkURL( QString::fromUtf8(sender()->name()),
        ( (KAction *)sender() )->text() /* Title */ );
}

// #include "./bookmarkmenu.moc"
