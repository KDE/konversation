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

#include "highlight.h"


unsigned int Highlight::s_id = 0;                 // static

Highlight::Highlight(const QString& itemPattern,
bool regExp,
const QColor& itemColor,
const KUrl& soundURL,
const QString& autoText)
{
    m_itemPattern = itemPattern;
    m_autoText = autoText;
    m_itemColor = itemColor;
    m_soundURL = soundURL;
    m_regExp = regExp;

    // unique ID for every Highlight
    m_itemID = s_id++;
}

int Highlight::getID() const { return m_itemID; }
QString Highlight::getPattern() const { return m_itemPattern; }
QString Highlight::getAutoText() const { return m_autoText; }
QColor Highlight::getColor() const { return m_itemColor; }
KUrl Highlight::getSoundURL() const { return m_soundURL; }

void Highlight::setPattern(const QString& itemPattern) { m_itemPattern = itemPattern; }
void Highlight::setAutoText(const QString& autoText) { m_autoText = autoText; }
void Highlight::setColor(const QColor& itemColor) { m_itemColor = itemColor; }
void Highlight::setSoundURL(const KUrl& url) { m_soundURL = url; }

void Highlight::setRegExp(bool state)  { m_regExp=state; }
bool Highlight::getRegExp() const { return m_regExp; }
