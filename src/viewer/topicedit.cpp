/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#include "topicedit.h"
#include "channel.h"
#include "irccharsets.h"

#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageWidget>

#include <QAction>
#include <QEvent>
#include <QTextCodec>


constexpr int MARGIN = 4;


TopicEdit::TopicEdit(QWidget* parent) : KTextEdit(parent)
{
    m_channel = nullptr;

    m_maximumLength = -1;
    m_maxCursorPos = -1;

    m_warning = nullptr;
    m_warningUndercarriage = nullptr;

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
        connect(document(), &QTextDocument::contentsChange, this, &TopicEdit::contentsChanged);
    else
        disconnect(document(), &QTextDocument::contentsChange, this, &TopicEdit::contentsChanged);
}

void TopicEdit::contentsChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position)

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
        m_warning->setWordWrap(true);

        auto* trimExcessAction = new QAction(i18n("Delete excess text"), m_warning);
        connect(trimExcessAction, &QAction::triggered, this, &TopicEdit::trimExcessText);

        m_warning->addAction(trimExcessAction);

        // NOTE: This makes certain assumptions about the widget hierarchy the
        // edit is placed in. Try window() if you need to be more generic.
        m_warningUndercarriage = new QWidget(parentWidget()->parentWidget());
        m_warningUndercarriage->lower();
        m_warningUndercarriage->setBackgroundRole(QPalette::Base);
        m_warningUndercarriage->setPalette(viewport()->palette());
        m_warningUndercarriage->setAutoFillBackground(true);
    }

    m_warning->setText(i18n("Text past the server limit of %1 bytes is shown in color.", m_maximumLength));

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
    int heightForWidth = m_warning->heightForWidth(width);
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
    if ((event->type() == QEvent::Resize || event->type() == QEvent::Move)
        && m_warning && m_warning->isVisible())
        updateWarningGeometry();

    return KTextEdit::eventFilter(watched, event);
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

#include "moc_topicedit.cpp"
