/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Matthias Gierlings <gismore@users.sourceforge.net>
*/

#include "highlight.h"


unsigned int Highlight::s_id = 0;                 // static

Highlight::Highlight(const QString& pattern, bool regExp, const QColor& color,
    const QUrl &soundURL, const QString& autoText, const QString& chatWindows, bool notify)
{
    setPattern(pattern);
    setRegExp(regExp);
    setColor(color);
    setSoundURL(soundURL);
    setAutoText(autoText);
    setChatWindows(chatWindows);
    setNotify(notify);

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

void Highlight::setSoundURL(const QUrl &url)
{
    m_soundURL = url;
}

QUrl Highlight::getSoundURL() const
{
    return m_soundURL;
}

void Highlight::setChatWindows(const QString& chatWindows)
{
    m_chatWindows = chatWindows;

    // split string list of chat windows and trim all entries
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    m_chatWindowList = m_chatWindows.split(QRegExp(QStringLiteral("[,;]")), QString::SkipEmptyParts);
#else
    m_chatWindowList = m_chatWindows.split(QRegExp(QStringLiteral("[,;]")), Qt::SkipEmptyParts);
#endif

    for (QString& chatWindow : m_chatWindowList) {
        chatWindow = chatWindow.trimmed();
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

bool Highlight::getNotify() const
{
    return m_notify;
}

void Highlight::setNotify(bool notify)
{
    m_notify = notify;
}

