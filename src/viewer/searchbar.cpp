/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Renchi Raju <renchi@pooh.tam.uiuc.edu>
    SPDX-FileCopyrightText: 2006, 2016 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "searchbar.h"

#include "application.h"

#include <QFocusEvent>
#include <QTimer>
#include <QShortcut>

#include <KActionCollection>
#include <KColorScheme>
#include <KStandardAction>
#include <QMenu>

#include <algorithm>

SearchBar::SearchBar(QWidget* parent)
: QWidget(parent),
  m_goUpSearch(QIcon::fromTheme(QStringLiteral("go-up-search"))),
  m_goDownSearch(QIcon::fromTheme(QStringLiteral("go-down-search")))
{
    setupUi(this);

    const auto allChildWidgets = findChildren<QWidget*>();
    for (QWidget* widget : allChildWidgets)
        widget->installEventFilter(this);

    parent->installEventFilter(this); // HACK See notes in SearchBar::eventFilter

    m_flags = {};
    m_fromCursor = false;

    setFocusProxy(m_searchEdit);
    m_closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    m_findNextButton->setIcon(m_goUpSearch);
    m_findPreviousButton->setIcon(m_goDownSearch);
    Application* konvApp = Application::instance();
    konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindNext))->setIcon(m_goUpSearch);
    konvApp->getMainWindow()->actionCollection()->action(KStandardAction::name(KStandardAction::FindPrev))->setIcon(m_goDownSearch);
    m_optionsButton->setIcon(QIcon::fromTheme(QStringLiteral("settings-configure")));

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    m_closeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(hide()));
    m_closeShortcut->setEnabled(false);

    connect(m_timer, &QTimer::timeout, this, &SearchBar::slotFind);
    connect(m_searchEdit, &KLineEdit::textChanged, this, &SearchBar::slotTextChanged);
    connect(m_searchEdit, &KLineEdit::returnKeyPressed, this, &SearchBar::slotFindNext);
    connect(m_findNextButton, &QToolButton::clicked, this, &SearchBar::slotFindNext);
    connect(m_findPreviousButton, &QToolButton::clicked, this, &SearchBar::slotFindPrevious);
    connect(m_closeButton, &QToolButton::clicked, this, &SearchBar::hide);
    connect(m_optionsButton, &QToolButton::clicked, m_optionsButton, &QToolButton::showMenu);

    QAction *action = nullptr;
    m_optionsMenu = new QMenu(m_optionsButton);
    action = m_optionsMenu->addAction(i18n("Match Case"));
    action->setCheckable(true);
    connect(action, &QAction::toggled, this, &SearchBar::toggleMatchCase);
    action = m_optionsMenu->addAction(i18n("Whole Words Only"));
    action->setCheckable(true);
    connect(action, &QAction::toggled, this, &SearchBar::toggleWholeWords);

    m_optionsButton->setMenu(m_optionsMenu);
}

SearchBar::~SearchBar()
{
}

bool SearchBar::eventFilter(QObject* object, QEvent* e)
{
    QFocusEvent* focusEvent = nullptr;

    // HACK This event comes from the ViewContainer when
    // updateViewActions is called. ViewContainer can't
    // check the status of the search box, and so tramples
    // on the QAction we're managing - that is until the
    // ambiguous shortcut dialog box pops up, at which point
    // a focus event is sent to the searchbar and the other
    // stanza in this method is triggered.
    // 414 was the time of day when I named the event.
    bool hack = (object == parent() && e->type() == QEvent::User+414);
    if (hack && hasFocus())
        focusEvent = new QFocusEvent(QEvent::FocusIn);
    else
        focusEvent = dynamic_cast<QFocusEvent*>(e);

    if (focusEvent)
    {
        Application* konvApp = Application::instance();
        QAction * action = konvApp->getMainWindow()->actionCollection()->action(QStringLiteral("focus_input_box"));

        if (action->shortcut().matches(QKeySequence(Qt::Key_Escape)))
        {
            action->setEnabled(focusEvent->lostFocus());
            m_closeShortcut->setEnabled(focusEvent->gotFocus());
        }
    }

    if (hack)
        delete focusEvent;

    return hack;
}

void SearchBar::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    m_searchEdit->selectAll();
}

bool SearchBar::focusedChild() const
{
    const QList<QWidget*> l = findChildren<QWidget*>();

    return std::any_of(l.begin(), l.end(), [](QWidget* w) {
        return w->hasFocus();
    });
}

void SearchBar::hideEvent(QHideEvent* e)
{
    m_timer->stop();
    QWidget::hideEvent(e);

    if (focusedChild())
        Q_EMIT hidden();
}

void SearchBar::slotTextChanged()
{
    m_fromCursor = false;
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

    Q_EMIT signalSearchChanged(m_searchEdit->text());
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

    Q_EMIT signalSearchNext();
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

    Q_EMIT signalSearchPrevious();
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

QTextDocument::FindFlags SearchBar::flags() const
{
    return m_flags;
}

bool SearchBar::fromCursor() const
{
    return m_fromCursor;
}

void SearchBar::toggleMatchCase(bool value)
{
    if(value)
        m_flags |= QTextDocument::FindCaseSensitively;
    else
        m_flags &= ~QTextDocument::FindCaseSensitively;

    m_fromCursor = true;
    slotFind();
}

void SearchBar::toggleWholeWords(bool value)
{
    if(value)
        m_flags |= QTextDocument::FindWholeWords;
    else
        m_flags &= ~QTextDocument::FindWholeWords;

    m_fromCursor = true;
    slotFind();
}

#include "moc_searchbar.cpp"
