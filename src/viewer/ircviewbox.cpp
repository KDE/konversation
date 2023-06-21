/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
    SPDX-FileCopyrightText: 2016 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "ircviewbox.h"
#include "ircview.h"
#include "searchbar.h"

#include <QVBoxLayout>

IRCViewBox::IRCViewBox(QWidget* parent)
: QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
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

    if (!match && m_matchedOnce)
    {
        if(reversed)
        {
            m_ircView->moveCursor(QTextCursor::Start);
        }
        else
        {
            m_ircView->moveCursor(QTextCursor::End);
        }

        match = m_ircView->searchNext(reversed);
    }

    m_searchBar->setHasMatch(match);
}

void IRCViewBox::slotSearchChanged(const QString& pattern)
{
    bool match = m_ircView->search(pattern,
        m_searchBar->flags(),
        m_searchBar->fromCursor());

    m_searchBar->setHasMatch(match);
    m_matchedOnce = match;
}

#include "moc_ircviewbox.cpp"
