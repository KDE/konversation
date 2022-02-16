/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Matthias Gierlings <gismore@users.sourceforge.net>
*/

#include "highlightviewitem.h"

#include <QUrl>


HighlightViewItem::HighlightViewItem(QTreeWidget* parent, Highlight* highlight)
: QTreeWidgetItem(parent)
{
    setID(highlight->getID());
    setPattern(highlight->getPattern());
    setRegExp(highlight->getRegExp());
    setColor(highlight->getColor());
    setSoundURL(highlight->getSoundURL());
    setAutoText(highlight->getAutoText());
    setChatWindows(highlight->getChatWindows());
    setNotify(highlight->getNotify());
}

HighlightViewItem::~HighlightViewItem()
{
}

void HighlightViewItem::setID(const int itemID)
{
    m_itemID = itemID;
}

int HighlightViewItem::getID() const
{
    return m_itemID;
}

void HighlightViewItem::setPattern(const QString& pattern)
{
    m_pattern = pattern;
    setText(1, m_pattern);
}

QString HighlightViewItem::getPattern() const
{
    return m_pattern;
}

void HighlightViewItem::setRegExp(const bool regexp)
{
    setCheckState(0, regexp ? Qt::Checked : Qt::Unchecked);
}

bool HighlightViewItem::getRegExp() const
{
    return checkState(0) == Qt::Checked;
}

void HighlightViewItem::setColor(const QColor color)
{
    m_color = color;

    const QBrush brush(m_color);
    setForeground(1, brush);
    setForeground(2, brush);
    setForeground(3, brush);
    setForeground(4, brush);
}

QColor HighlightViewItem::getColor() const
{
    return m_color;
}

void HighlightViewItem::setSoundURL(const QUrl &soundURL)
{
    m_soundURL = soundURL;
    setText(2, m_soundURL.toLocalFile());
}

QUrl HighlightViewItem::getSoundURL() const
{
    return m_soundURL;
}

void HighlightViewItem::setAutoText(const QString& autoText)
{
    m_autoText = autoText;
    setText(3, m_autoText);
}

QString HighlightViewItem::getAutoText() const
{
    return m_autoText;
}

void HighlightViewItem::setChatWindows(const QString& chatWindows)
{
    m_chatWindows = chatWindows;
    setText(4, m_chatWindows);
}

QString HighlightViewItem::getChatWindows() const
{
    return m_chatWindows;
}

void HighlightViewItem::setNotify(bool doNotify)
{
    m_notify = doNotify;
}

bool HighlightViewItem::getNotify() const
{
    return m_notify;
}
