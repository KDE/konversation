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
#include <qlabel.h>
#include <qpixmap.h>
#include <qobject.h>
#include <qshortcut.h>
#include <qtoolbutton.h>

#include <kdebug.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmenu.h>
#include <kpushbutton.h>
#include <klocale.h>


SearchBar::SearchBar(QWidget* parent)
: QWidget(parent)
{
    setupUi(this);

    m_searchFoward = false;
    m_matchCase = false;
    m_wholeWords = false;
    m_fromCursor = false;

    setFocusProxy(m_searchEdit);
    m_closeButton->setIcon(KIcon("process-stop"));
    m_findNextButton->setIcon(KIcon("arrow-up"));
    m_findPreviousButton->setIcon(KIcon("arrow-down"));
    m_statusPixLabel->hide();
    m_statusTextLabel->hide();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(hide()));

    connect(m_timer, SIGNAL(timeout()), SLOT(slotFind()));
    connect(m_searchEdit, SIGNAL(textChanged(const QString&)), SLOT(slotTextChanged()));
    connect(m_searchEdit, SIGNAL(returnPressed()), SLOT(slotFindNext()));
    connect(m_findNextButton, SIGNAL(clicked()), SLOT(slotFindNext()));
    connect(m_findPreviousButton, SIGNAL(clicked()), SLOT(slotFindPrevious()));
    connect(m_closeButton, SIGNAL(clicked()), SLOT(hide()));
    connect(m_optionsButton, SIGNAL(clicked()), this, SLOT(showOptionsMenu()));

    QAction *action = 0;
    m_optionsMenu = new KMenu(m_optionsButton);
    action = m_optionsMenu->addAction(i18n("Find Forward"));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSearchFoward(bool)));
    action = m_optionsMenu->addAction(i18n("Case Sensitive"));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleMatchCase(bool)));
    action = m_optionsMenu->addAction(i18n("Whole Words Only"));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleWholeWords(bool)));
    action = m_optionsMenu->addAction(i18n("From Cursor"));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleFromCursor(bool)));

    m_optionsButton->setMenu(m_optionsMenu);
}

SearchBar::~SearchBar()
{
}

void SearchBar::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
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

void SearchBar::hideEvent(QHideEvent* e)
{
    m_timer->stop();
    QWidget::hideEvent(e);

    if (focusedChild())
        emit hidden();
}

void SearchBar::slotTextChanged()
{
    m_timer->start(50);
}

void SearchBar::slotFind()
{
    if (m_searchEdit->text().isEmpty())
    {
        m_searchEdit->setPalette(QPalette());
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
        m_searchEdit->setPalette(QPalette()); 
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
        m_searchEdit->setPalette(QPalette());
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
    pal.setColor(QPalette::Active, QPalette::Base, value ? Qt::green : Qt::red);
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

void SearchBar::toggleSearchFoward(bool value)
{
    m_searchFoward = value;
    slotTextChanged();
}

void SearchBar::toggleMatchCase(bool value)
{
    m_matchCase = value;
    slotTextChanged();
}

void SearchBar::toggleWholeWords(bool value)
{
    m_wholeWords = value;
    slotTextChanged();
}

void SearchBar::toggleFromCursor(bool value)
{
    m_fromCursor = value;
    slotTextChanged();
}

void SearchBar::showOptionsMenu()
{
  m_optionsButton->showMenu();
}

#include "searchbar.moc"
