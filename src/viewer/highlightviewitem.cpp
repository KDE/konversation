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

#include <kurl.h>


HighlightViewItem::HighlightViewItem(QTreeWidget* parent, Highlight* passed_Highlight)
: QTreeWidgetItem(parent)
{
    setCheckState(0, passed_Highlight->getRegExp() ? Qt::Checked : Qt::Unchecked);
    setText(1,passed_Highlight->getPattern());
    itemColor = passed_Highlight->getColor();
    itemID = passed_Highlight->getID();
    setSoundURL(passed_Highlight->getSoundURL());
    setAutoText(passed_Highlight->getAutoText());
    updateRowColor();
}

HighlightViewItem::~HighlightViewItem()
{
}

void HighlightViewItem::updateRowColor()
{
    const QBrush brush(itemColor);
    setForeground(1, brush);
    setForeground(2, brush);
    setForeground(3, brush);
}

void HighlightViewItem::setPattern(const QString& newPattern) { setText(1,newPattern); }
QString HighlightViewItem::getPattern()                       { return text(1); }

void HighlightViewItem::setSoundURL(const KUrl& url)
{
    soundURL = url;
    setText(2, soundURL.prettyUrl());
}

void HighlightViewItem::setAutoText(const QString& newAutoText)
{
    autoText = newAutoText;
    setText(3,newAutoText);
}

bool HighlightViewItem::getRegExp()
{
    return checkState(0) == Qt::Checked;
}

QString HighlightViewItem::getAutoText()
{
    return autoText;
}
