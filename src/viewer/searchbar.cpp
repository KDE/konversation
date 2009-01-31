/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
  Copyright (C) 2006 Peter Simonsson <psn@linux.se>
*/

#include "searchbar.h"

#include <qcheckbox.h>
#include <qtimer.h>
#include <qpalette.h>
#include <q3accel.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qobject.h>
#include <qtoolbutton.h>
#include <q3popupmenu.h>
#include <q3widgetstack.h>
//Added by qt3to4:
#include <QShowEvent>

#include <kdebug.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <klocale.h>

#define SEARCH_FORWARD_MENU 1
#define MATCH_CASE_MENU 2
#define WHOLE_WORDS_ONLY_MENU 3
#define FROM_CURSOR_MENU 4


SearchBar::SearchBar(QWidget* parent)
: SearchBarBase(parent)
{
    m_searchFoward = false;
    m_matchCase = false;
    m_wholeWords = false;
    m_fromCursor = false;

    setFocusProxy(m_searchEdit);
    KIconLoader* iconLoader = KIconLoader::global();
    m_closeButton->setIcon(KIcon("process-stop"));
    m_findNextButton->setIcon(KIcon("arrow-up"));
    m_findPreviousButton->setIcon(KIcon("arrow-down"));
    m_statusPixLabel->hide();
    m_statusTextLabel->hide();

    m_timer = new QTimer(this);

    Q3Accel* accel = new Q3Accel(this);
    accel->connectItem( accel->insertItem(Qt::Key_Escape), this, SLOT(hide()));

    connect(m_timer, SIGNAL(timeout()), SLOT(slotFind()));
    connect(m_searchEdit, SIGNAL(textChanged(const QString&)), SLOT(slotTextChanged()));
    connect(m_searchEdit, SIGNAL(returnPressed()), SLOT(slotFindNext()));
    connect(m_findNextButton, SIGNAL(clicked()), SLOT(slotFindNext()));
    connect(m_findPreviousButton, SIGNAL(clicked()), SLOT(slotFindPrevious()));
    connect(m_closeButton, SIGNAL(clicked()), SLOT(hide()));
    connect(m_optionsButton, SIGNAL(clicked()), this, SLOT(showOptionsMenu()));

    m_optionsMenu = new Q3PopupMenu(m_optionsButton, "options_menu");
    m_optionsMenu->setCheckable(true);
    m_optionsMenu->insertItem(i18n("Find Forward"), this, SLOT(toggleSearchFoward()), 0, SEARCH_FORWARD_MENU);
    m_optionsMenu->insertItem(i18n("Case Sensitive"), this, SLOT(toggleMatchCase()), 0, MATCH_CASE_MENU);
    m_optionsMenu->insertItem(i18n("Whole Words Only"), this, SLOT(toggleWholeWords()), 0, WHOLE_WORDS_ONLY_MENU);
    m_optionsMenu->insertItem(i18n("From Cursor"), this, SLOT(toggleFromCursor()), 0, FROM_CURSOR_MENU);

    m_optionsButton->setPopup(m_optionsMenu);
}

SearchBar::~SearchBar()
{
}

void SearchBar::showEvent(QShowEvent *e)
{
    SearchBarBase::showEvent(e);
    m_searchEdit->selectAll();
}

bool SearchBar::focusedChild()
{
    QList<QWidget *> l = findChildren<QWidget *>();

    for (int i=0; i < l.size(); i++)
        if (l.at(i)->hasFocus())
            return true;
    return false;
}

void SearchBar::hide()
{
    m_timer->stop();
    SearchBarBase::hide();

    if (focusedChild())
        emit hidden();
}

void SearchBar::slotTextChanged()
{
    m_timer->start(50, true);
}

void SearchBar::slotFind()
{
    if (m_searchEdit->text().isEmpty())
    {
        m_searchEdit->unsetPalette();
        m_findNextButton->setEnabled(false);
        m_findPreviousButton->setEnabled(false);
        setStatus(QPixmap(), "");
        return;
    }

    emit signalSearchChanged(m_searchEdit->text());
}

void SearchBar::slotFindNext()
{
    if (m_searchEdit->text().isEmpty())
    {
        m_searchEdit->unsetPalette();
        m_findNextButton->setEnabled(false);
        m_findPreviousButton->setEnabled(false);
        setStatus(QPixmap(), "");
        return;
    }

    emit signalSearchNext();
}

void SearchBar::slotFindPrevious()
{
    if (m_searchEdit->text().isEmpty())
    {
        m_searchEdit->unsetPalette();
        m_findNextButton->setEnabled(false);
        m_findPreviousButton->setEnabled(false);
        setStatus(QPixmap(), "");
        return;
    }

    emit signalSearchPrevious();
}

void SearchBar::setHasMatch(bool value)
{
    QPalette pal = m_searchEdit->palette();
    pal.setColor(QPalette::Active, QColorGroup::Base, value ? Qt::green : Qt::red);
    m_searchEdit->setPalette(pal);
    m_findNextButton->setEnabled(value);
    m_findPreviousButton->setEnabled(value);
}

void SearchBar::setStatus(const QPixmap& pix, const QString& text)
{
    if(!text.isEmpty()) {
        m_statusPixLabel->show();
        m_statusTextLabel->show();
    } else {
        m_statusPixLabel->hide();
        m_statusTextLabel->hide();
    }

    m_statusPixLabel->setPixmap(pix);
    m_statusTextLabel->setText(text);
}

QString SearchBar::pattern() const
{
    return m_searchEdit->text();
}

bool SearchBar::searchForward() const
{
    return m_searchFoward;
}

bool SearchBar::caseSensitive() const
{
    return m_matchCase;
}

bool SearchBar::wholeWords() const
{
    return m_wholeWords;
}

bool SearchBar::fromCursor() const
{
    return m_fromCursor;
}

void SearchBar::toggleSearchFoward()
{
    m_searchFoward = !m_searchFoward;
    m_optionsMenu->setItemChecked(SEARCH_FORWARD_MENU, m_searchFoward);
    slotTextChanged();
}

void SearchBar::toggleMatchCase()
{
    m_matchCase = !m_matchCase;
    m_optionsMenu->setItemChecked(MATCH_CASE_MENU, m_matchCase);
    slotTextChanged();
}

void SearchBar::toggleWholeWords()
{
    m_wholeWords = !m_wholeWords;
    m_optionsMenu->setItemChecked(WHOLE_WORDS_ONLY_MENU, m_wholeWords);
    slotTextChanged();
}

void SearchBar::toggleFromCursor()
{
    m_fromCursor = !m_fromCursor;
    m_optionsMenu->setItemChecked(FROM_CURSOR_MENU, m_fromCursor);
    slotTextChanged();
}

void SearchBar::showOptionsMenu()
{
  m_optionsButton->openPopup();
}

// #include "./viewer/searchbar.moc"
