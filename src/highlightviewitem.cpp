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
#include <k3listview.h>


HighlightViewItem::HighlightViewItem(K3ListView* parent, Highlight* passed_Highlight)
: Q3CheckListItem(parent, QString(), Q3CheckListItem::CheckBox)
{
    setText(1,passed_Highlight->getPattern());
    itemColor = passed_Highlight->getColor();
    itemID = passed_Highlight->getID();
    setSoundURL(passed_Highlight->getSoundURL());
    setAutoText(passed_Highlight->getAutoText());
    setOn(passed_Highlight->getRegExp());
    m_changed=false;
}

HighlightViewItem::~HighlightViewItem()
{
}

void HighlightViewItem::paintCell(QPainter* p, const QColorGroup &cg, int column, int width, int alignment)
{
    // copy all colors from cg and only then change needed colors
    itemColorGroup=cg;
    itemColorGroup.setColor(QColorGroup::Text, itemColor);
    Q3CheckListItem::paintCell(p, itemColorGroup, column, width, alignment);
}

HighlightViewItem* HighlightViewItem::itemBelow()
{
    return (HighlightViewItem*) Q3CheckListItem::itemBelow();
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
    return isOn();
}

QString HighlightViewItem::getAutoText()
{
    return autoText;
}

// override default method to store the change
void HighlightViewItem::stateChange(bool /* newState */)
{
  // remember that the check box has been changed
  m_changed=true;
}

// returns true, if the checkbox has been changed
bool HighlightViewItem::hasChanged()
{
  return m_changed;
}

// tells us that the program has seen us changing
void HighlightViewItem::changeAcknowledged()
{
  m_changed=false;
}
