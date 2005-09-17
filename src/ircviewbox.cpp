/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2005-06-02
 * Description :
 *
 * Copyright 2005 by Renchi Raju

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include <klocale.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <qpixmap.h>

#include "ircview.h"
#include "searchbar.h"
#include "ircviewbox.h"

static QPixmap getIcon(const QString& name)
{
    KIconLoader* iconLoader = kapp->iconLoader();
    return iconLoader->loadIcon(name, KIcon::Toolbar, 16);
}

IRCViewBox::IRCViewBox(QWidget* parent, Server* newServer)
: QVBox(parent)
{
    m_searchBar = new SearchBar(this);
    m_searchBar->hide();
    m_ircView = new IRCView(this, newServer);
    m_searchBar->hide();
    m_matchedOnce = false;

    connect(m_searchBar, SIGNAL(signalSearchChanged(const QString&)),
        this, SLOT(slotSearchChanged(const QString&)));
    connect(m_searchBar, SIGNAL(signalSearchNext()),
        this, SLOT(slotSearchNext()));
    connect(m_ircView, SIGNAL(doSearch()),
        SLOT(slotSearch()));
}

IRCViewBox::~IRCViewBox()
{
    delete m_ircView;
    delete m_searchBar;
}

IRCView* IRCViewBox::ircView() const
{
    return m_ircView;
}

void IRCViewBox::slotSearch()
{
    if (m_searchBar->isVisible())
      {
	m_searchBar->hide();
	return;
      }
    
    m_searchBar->show();
    m_searchBar->setFocus();
}

void IRCViewBox::slotSearchNext()
{
    bool match = m_ircView->searchNext();

    if (match)
    {
        m_searchBar->setHasMatch(true);
        m_searchBar->setStatus(QPixmap(), "");
        return;
    }

    if (!m_matchedOnce)
    {
        m_searchBar->setHasMatch(false);
        m_searchBar->setStatus(getIcon("messagebox_warning"),
            i18n("Phrase not found"));
        return;
    }

    match = m_ircView->search(m_searchBar->pattern(),
        m_searchBar->caseSensitive(),
        false,
        m_searchBar->searchForward(),
        false);

    if (!match)
    {
        m_searchBar->setHasMatch(false);
        m_searchBar->setStatus(getIcon("messagebox_warning"),
            i18n("Phrase not found"));
        return;
    }

    m_searchBar->setHasMatch(true);
    m_searchBar->setStatus(getIcon("messagebox_info"),
        i18n("Wrapped search"));

}

void IRCViewBox::slotSearchChanged(const QString& pattern)
{
    bool match = m_ircView->search(pattern,
        m_searchBar->caseSensitive(),
        false,
        m_searchBar->searchForward(),
        false);

    if (match)
    {
        m_searchBar->setHasMatch(true);
        m_searchBar->setStatus(QPixmap(), "");
    }
    else
    {
        m_searchBar->setHasMatch(false);
        m_searchBar->setStatus(getIcon("messagebox_warning"),
            i18n("Phrase not found"));
    }

    m_matchedOnce = match;
}

#include "ircviewbox.moc"
