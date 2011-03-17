/***************************************************************************
    begin                : Sat Jun 15 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "highlightviewitem.h"

#include <KUrl>


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
}

HighlightViewItem::~HighlightViewItem()
{
}

void HighlightViewItem::setID(const int itemID)
{
    m_itemID = itemID;
}

int HighlightViewItem::getID()
{
    return m_itemID;
}

void HighlightViewItem::setPattern(const QString& pattern)
{
    m_pattern = pattern;
    setText(1, m_pattern);
}

QString HighlightViewItem::getPattern()
{
    return m_pattern;
}

void HighlightViewItem::setRegExp(const bool regexp)
{
    setCheckState(0, regexp ? Qt::Checked : Qt::Unchecked);
}

bool HighlightViewItem::getRegExp()
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

QColor HighlightViewItem::getColor()
{
    return m_color;
}

void HighlightViewItem::setSoundURL(const KUrl& soundURL)
{
    m_soundURL = soundURL;
    setText(2, m_soundURL.prettyUrl());
}

KUrl HighlightViewItem::getSoundURL()
{
    return m_soundURL;
}

void HighlightViewItem::setAutoText(const QString& autoText)
{
    m_autoText = autoText;
    setText(3, m_autoText);
}

QString HighlightViewItem::getAutoText()
{
    return m_autoText;
}

void HighlightViewItem::setChatWindows(const QString& chatWindows)
{
    m_chatWindows = chatWindows;
    setText(4, m_chatWindows);
}

QString HighlightViewItem::getChatWindows()
{
    return m_chatWindows;
}
