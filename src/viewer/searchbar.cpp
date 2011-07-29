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

#include "application.h"

#include <QFocusEvent>
#include <QTimer>
#include <QShortcut>

#include <KActionCollection>
#include <KColorScheme>
#include <KStandardAction>
#include <KMenu>


SearchBar::SearchBar(QWidget* parent)
: QWidget(parent), m_goUpSearch("go-up-search"), m_goDownSearch("go-down-search")
{
    setupUi(this);

    foreach(QWidget* widget, findChildren<QWidget*>())
        widget->installEventFilter(this);

    m_searchFoward = false;
    m_matchCase = false;
    m_wholeWords = false;
    m_fromCursor = false;

    setFocusProxy(m_searchEdit);
    m_closeButton->setIcon(KIcon("dialog-close"));
    m_findNextButton->setIcon(m_goUpSearch);
    m_findPreviousButton->setIcon(m_goDownSearch);
    Application* konvApp = static_cast<Application*>(kapp);
    konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindNext))->setIcon(m_goUpSearch);
    konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindPrev))->setIcon(m_goDownSearch);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    m_closeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(hide()));
    m_closeShortcut->setEnabled(false);

    connect(m_timer, SIGNAL(timeout()), SLOT(slotFind()));
    connect(m_searchEdit, SIGNAL(textChanged(QString)), SLOT(slotTextChanged()));
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
    action = m_optionsMenu->addAction(i18n("Match Case"));
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

bool SearchBar::eventFilter(QObject* object, QEvent* e)
{
    Q_UNUSED(object);

    QFocusEvent* focusEvent = dynamic_cast<QFocusEvent*>(e);

    if (focusEvent)
    {
        Application* konvApp = static_cast<Application*>(kapp);
        KAction* action = static_cast<KAction*>(konvApp->getMainWindow()->actionCollection()->action("focus_input_box"));

        if (action->shortcut().contains(QKeySequence(Qt::Key_Escape)))
        {
            action->setEnabled(focusEvent->lostFocus());
            m_closeShortcut->setEnabled(focusEvent->gotFocus());
        }
    }

    return false;
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
        return;
    }

    emit signalSearchPrevious();
}

void SearchBar::setHasMatch(bool value)
{
    QPalette pal = m_searchEdit->palette();
    KColorScheme::adjustBackground(pal, value ? KColorScheme::PositiveBackground : KColorScheme::NegativeBackground);
    m_searchEdit->setPalette(pal);

    m_findNextButton->setEnabled(value);
    m_findPreviousButton->setEnabled(value);
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
    Application* konvApp = static_cast<Application*>(kapp);
    if (value) {
      m_findNextButton->setIcon(m_goDownSearch);
      m_findPreviousButton->setIcon(m_goUpSearch);
      konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindNext))->setIcon(m_goDownSearch);
      konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindPrev))->setIcon(m_goUpSearch);
    }
    else
    {
      m_findNextButton->setIcon(m_goUpSearch);
      m_findPreviousButton->setIcon(m_goDownSearch);
      konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindNext))->setIcon(m_goUpSearch);
      konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindPrev))->setIcon(m_goDownSearch);
    }
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
