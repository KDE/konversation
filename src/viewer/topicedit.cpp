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
#include "channel.h"
#include "irccharsets.h"

#include <KColorScheme>
#include <KLocale>
#include <KMessageWidget>

#include <QAction>
#include <QEvent>
#include <QTextCodec>


#define MARGIN 4


TopicEdit::TopicEdit(QWidget* parent) : KTextEdit(parent)
{
    m_channel = 0;

    m_maximumLength = -1;
    m_maxCursorPos = -1;

    m_warning = 0;
    m_warningUndercarriage = 0;

    viewport()->installEventFilter(this);

    setAcceptRichText(false);
}

TopicEdit::~TopicEdit()
{
}

Channel* TopicEdit::channel() const
{
    return m_channel;
}

void TopicEdit::setChannel(Channel* channel)
{
    m_channel = channel;
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

    // If there are no undo steps, the change probably occurred via setText() rather
    // than user interaction.
    if (!document()->availableUndoSteps())
    {
        resetTextColorization();

        if (m_warning && m_warning->isVisible())
            hideWarning();

        document()->clearUndoRedoStacks();

        QMetaObject::invokeMethod(this, "moveCursorToEnd", Qt::QueuedConnection);

        return;
    }

    // Nothing we care about occurred.
    if (charsRemoved == 0 && charsAdded == 0) return;

    if (colorizeExcessText())
    {
        showWarning();
    }
    else if (m_warning && m_warning->isVisible())
    {
        resetTextColorization();
        hideWarning();
    }
}

bool TopicEdit::colorizeExcessText()
{
    if (!m_channel) {
        return false;
    }

    QString channelCodecName = m_channel->getChannelEncoding();
    QTextCodec* codec = channelCodecName.isEmpty() ? m_channel->getServer()->getIdentity()->getCodec()
        : Konversation::IRCCharsets::self()->codecForName(channelCodecName);

    QTextCursor cursor(document());

    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    if (codec->fromUnicode(Konversation::doVarExpansion(cursor.selectedText())).length() <= m_maximumLength)
    {
        m_maxCursorPos = -1;
        return false;
    }

    int end = cursor.position();
    cursor.setPosition(0);

    for (int i = 1; i <= end; ++i)
    {
        cursor.setPosition(i, QTextCursor::KeepAnchor);

        int length = codec->fromUnicode(Konversation::doVarExpansion(cursor.selectedText())).length();

        if (length <= m_maximumLength)
        {
            m_maxCursorPos = i;
        }
        else
        {
            break;
        }
    }

    cursor.setPosition(0);

    KColorScheme colors(QPalette::Active);

    QTextCharFormat format = cursor.charFormat();
    format.setForeground(colors.foreground(KColorScheme::NormalText));

    cursor.joinPreviousEditBlock();

    cursor.setPosition(m_maxCursorPos, QTextCursor::KeepAnchor);
    cursor.setCharFormat(format);

    cursor.setPosition(m_maxCursorPos, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    format.setForeground(colors.foreground(KColorScheme::NegativeText));
    cursor.setCharFormat(format);

    cursor.endEditBlock();

    return true;
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

    // FIXME: This should read bytes instead of characters, but we're in string freeze.
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

void TopicEdit::trimExcessText()
{
    if (m_maxCursorPos == -1)
    {
        return;
    }

    QTextCursor cursor(document());

    cursor.setPosition(m_maxCursorPos, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    cursor.removeSelectedText();
}

void TopicEdit::moveCursorToEnd()
{
    moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
}
