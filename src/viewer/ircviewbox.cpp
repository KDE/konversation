/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
*/

#include "ircviewbox.h"
#include "ircview.h"
#include "searchbar.h"

#include <KIconLoader>
#include <QPixmap>
#include <QVBoxLayout>

IRCViewBox::IRCViewBox(QWidget* parent)
: QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    m_ircView = new IRCView(this);
    mainLayout->addWidget(m_ircView);
    m_searchBar = new SearchBar(this);
    mainLayout->addWidget(m_searchBar);
    m_searchBar->hide();
    m_matchedOnce = false;

    connect(m_searchBar, &SearchBar::signalSearchChanged, this, &IRCViewBox::slotSearchChanged);
    connect(m_searchBar, &SearchBar::signalSearchNext, this, &IRCViewBox::slotSearchNext);
    connect(m_searchBar, &SearchBar::signalSearchPrevious, this, &IRCViewBox::slotSearchPrevious);
    connect(m_ircView, &IRCView::doSearch, this, &IRCViewBox::slotSearch);
    connect(m_ircView, &IRCView::doSearchNext, this, &IRCViewBox::slotSearchNext);
    connect(m_ircView, &IRCView::doSearchPrevious, this, &IRCViewBox::slotSearchPrevious);
    connect(m_searchBar, &SearchBar::hidden, m_ircView, &IRCView::gotFocus);
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
    searchNext(false);
}

void IRCViewBox::slotSearchPrevious()
{
    searchNext(true);
}

void IRCViewBox::searchNext(bool reversed)
{
    bool match = m_ircView->searchNext(reversed);

    if (match)
    {
        m_searchBar->setHasMatch(true);
        return;
    }

    if (!m_matchedOnce)
    {
        m_searchBar->setHasMatch(false);
        return;
    }

    if((m_searchBar->searchForward() && !reversed) || (!m_searchBar->searchForward() && reversed))
    {
        m_ircView->moveCursor(QTextCursor::Start);
    }
    else
    {
        m_ircView->moveCursor(QTextCursor::End);
    }

    match = m_ircView->searchNext(reversed);

    if (!match)
    {
        m_searchBar->setHasMatch(false);
        return;
    }

    m_searchBar->setHasMatch(true);
}

void IRCViewBox::slotSearchChanged(const QString& pattern)
{
    bool match = m_ircView->search(pattern,
        m_searchBar->caseSensitive(),
        m_searchBar->wholeWords(),
        m_searchBar->searchForward(),
        m_searchBar->fromCursor());

    if (match)
    {
        m_searchBar->setHasMatch(true);
    }
    else
    {
        m_searchBar->setHasMatch(false);
    }

    m_matchedOnce = match;
}


