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

Highlight::Highlight(const QString& pattern, bool regExp, const QColor& color,
    const KUrl& soundURL, const QString& autoText, const QString& chatWindows)
{
    setPattern(pattern);
    setRegExp(regExp);
    setColor(color);
    setSoundURL(soundURL);
    setAutoText(autoText);
    setChatWindows(chatWindows);

    // unique ID for every Highlight
    m_itemID = s_id++;
}

int Highlight::getID() const
{
    return m_itemID;
}

void Highlight::setPattern(const QString& itemPattern)
{
    m_itemPattern = itemPattern;
}

QString Highlight::getPattern() const
{
    return m_itemPattern;
}

void Highlight::setRegExp(bool state)
{
    m_regExp=state;
}

bool Highlight::getRegExp() const
{
    return m_regExp;
}

void Highlight::setColor(const QColor& itemColor)
{
    m_itemColor = itemColor;
}

QColor Highlight::getColor() const
{
    return m_itemColor;
}

void Highlight::setAutoText(const QString& autoText)
{
    m_autoText = autoText;
}

QString Highlight::getAutoText() const
{
    return m_autoText;
}

void Highlight::setSoundURL(const KUrl& url)
{
    m_soundURL = url;
}

KUrl Highlight::getSoundURL() const
{
    return m_soundURL;
}

void Highlight::setChatWindows(const QString& chatWindows)
{
    m_chatWindows = chatWindows;

    // split string list of chat windows and trim all entries
    m_chatWindowList = m_chatWindows.split(QRegExp("[,;]"), QString::SkipEmptyParts);

    QMutableStringListIterator it(m_chatWindowList);
    while (it.hasNext())
    {
        it.setValue(it.next().trimmed());
    }
}

QString Highlight::getChatWindows() const
{
    return m_chatWindows;
}

QStringList Highlight::getChatWindowList() const
{
    return m_chatWindowList;
}
