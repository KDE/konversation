/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2012 Eike Hein <hein@kde.org>
*/

#include "topicedit.h"

#if KDE_IS_VERSION(4, 7, 0)
#include <KColorScheme>
#include <KLocale>
#include <KMessageWidget>

#include <QAction>
#include <QEvent>


#define MARGIN 4
#endif


TopicEdit::TopicEdit(QWidget* parent) : KTextEdit(parent)
{
    m_maximumLength = -1;
    m_lastEditPastMaximumLength = false;

#if KDE_IS_VERSION(4, 7, 0)
    m_warning = 0;
    m_warningUndercarriage = 0;

    viewport()->installEventFilter(this);
#endif

    setAcceptRichText(false);
}

TopicEdit::~TopicEdit()
{
}

int TopicEdit::maximumLength() const
{
    return m_maximumLength;
}

void TopicEdit::setMaximumLength(int length)
{
    m_maximumLength = length;

    if (m_maximumLength != -1)
        connect(document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    else
        disconnect(document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
}

void TopicEdit::contentsChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position);

    // If there are no undo steps, the change probably occured via setText() rather
    // than user interaction.
    if (!document()->availableUndoSteps())
    {
        resetTextColorization();

#if KDE_IS_VERSION(4, 7, 0)
        if (m_warning && m_warning->isVisible())
            hideWarning();
#endif

        document()->clearUndoRedoStacks();

        QMetaObject::invokeMethod(this, "moveCursorToEnd", Qt::QueuedConnection);

        return;
    }

    // Nothing we care about occured.
    if (charsRemoved == 0 && charsAdded == 0) return;

    if (document()->characterCount() > m_maximumLength)
    {
        m_lastEditPastMaximumLength = true;
        colorizeExcessText();
#if KDE_IS_VERSION(4, 7, 0)
        showWarning();
#endif
    }
    else if (document()->characterCount() <= m_maximumLength)
    {
        if (m_lastEditPastMaximumLength)
        {
            resetTextColorization();
            m_lastEditPastMaximumLength = false;
        }

#if KDE_IS_VERSION(4, 7, 0)
        if (m_warning && m_warning->isVisible())
            hideWarning();
#endif
    }
}

void TopicEdit::colorizeExcessText()
{
    QTextCursor cursor(document());

    KColorScheme colors(QPalette::Active);

    QTextCharFormat format = cursor.charFormat();
    format.setForeground(colors.foreground(KColorScheme::NormalText));

    cursor.joinPreviousEditBlock();

    cursor.setPosition(m_maximumLength - 1, QTextCursor::KeepAnchor);
    cursor.setCharFormat(format);

    cursor.setPosition(m_maximumLength - 1, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    format.setForeground(colors.foreground(KColorScheme::NegativeText));
    cursor.setCharFormat(format);

    cursor.endEditBlock();
}

void TopicEdit::resetTextColorization()
{
    QTextCursor cursor(document());

    KColorScheme colors(QPalette::Active);

    QTextCharFormat format = cursor.charFormat();
    format.setForeground(colors.foreground(KColorScheme::NormalText));

    cursor.joinPreviousEditBlock();

    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.setCharFormat(format);

    cursor.endEditBlock();
}

#if KDE_IS_VERSION(4, 7, 0)
void TopicEdit::showWarning()
{
    if (!m_warning)
    {
        m_warning = new KMessageWidget(this);
        m_warning->setMessageType(KMessageWidget::Warning);
        m_warning->setCloseButtonVisible(false);
#if KDE_IS_VERSION(4, 8, 4)
        m_warning->setWordWrap(true);
#endif

        QAction* trimExcessAction = new QAction(i18n("Delete excess text"), m_warning);
        connect(trimExcessAction, SIGNAL(triggered(bool)), this, SLOT(trimExcessText()));

        m_warning->addAction(trimExcessAction);

        // NOTE: This makes certain assumptions about the widget hierarchy the
        // edit is placed in. Try window() if you need to be more generic.
        m_warningUndercarriage = new QWidget(parentWidget()->parentWidget());
        m_warningUndercarriage->lower();
        m_warningUndercarriage->setBackgroundRole(QPalette::Base);
        m_warningUndercarriage->setPalette(viewport()->palette());
        m_warningUndercarriage->setAutoFillBackground(true);
    }

    m_warning->setText(i18n("Text past the server limit of %1 characters is shown in color.", m_maximumLength));

    updateWarningGeometry();

    m_warning->show();

    // Our minimum size hint may have changed.
    updateGeometry();
}

void TopicEdit::hideWarning()
{
    m_warning->hide();
    m_warningUndercarriage->hide();
    setViewportMargins(0, 0, 0, 0);
}

void TopicEdit::updateWarningGeometry()
{
    QRect rect = viewport()->geometry();

    int width = viewport()->width() - (2 * MARGIN);
#if KDE_IS_VERSION(4, 8, 4)
    int heightForWidth = m_warning->heightForWidth(width);
#else
    int heightForWidth = m_warning->minimumSizeHint().height();
#endif
    int viewportMargin = heightForWidth + MARGIN + 1;

    setViewportMargins(0, 0, 0, viewportMargin);

    if (viewport()->geometry() != rect)
    {
        updateWarningGeometry();

        return;
    }

    rect.setLeft(frameWidth() + MARGIN);
    rect.setTop(height() - (heightForWidth + frameWidth() + MARGIN));
    rect.setWidth(width);
    rect.setHeight(heightForWidth);

    m_warning->setGeometry(rect);

    if (viewport()->geometry().height() == 0)
        m_warningUndercarriage->hide();
    else
    {
        if (m_warningUndercarriage->isHidden())
            m_warningUndercarriage->show();

        QPoint topLeft = parentWidget()->mapTo(m_warningUndercarriage->parentWidget(),
            geometry().bottomLeft());
        topLeft.setX(topLeft.x() + frameWidth());
        topLeft.setY(topLeft.y() - frameWidth() - viewportMargin);

        rect.setTopLeft(topLeft);
        rect.setWidth(viewport()->width());
        rect.setHeight(viewportMargin);

        m_warningUndercarriage->setGeometry(rect);
    }
}

QSize TopicEdit::minimumSizeHint() const
{
    QSize size = KTextEdit::minimumSizeHint();

    if (m_warning && m_warning->isVisible())
    {
        int minHeight = m_warning->height() + (2 * MARGIN) + (2 * frameWidth());

        if (minHeight > size.height())
            size.setHeight(minHeight);
    }

    return size;
}

bool TopicEdit::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);

    if ((event->type() == QEvent::Resize || event->type() == QEvent::Move)
        && m_warning && m_warning->isVisible())
        updateWarningGeometry();

    return false;
}

void TopicEdit::moveEvent(QMoveEvent* event)
{
    if (m_warningUndercarriage && m_warningUndercarriage->isVisible())
        updateWarningGeometry();

    QWidget::moveEvent(event);
}
#endif

void TopicEdit::trimExcessText()
{
    QTextCursor cursor(document());

    cursor.setPosition(m_maximumLength - 1, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    cursor.removeSelectedText();
}

void TopicEdit::moveCursorToEnd()
{
    moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
}
