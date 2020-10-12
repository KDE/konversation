/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "ignorelistviewitem.h"
#include "ignore.h"

#include <KLocalizedString>


IgnoreListViewItem::IgnoreListViewItem(QTreeWidget* parent,const QString& name,int newFlags):
QTreeWidgetItem(parent, QStringList { name })
{
    setFlags(newFlags);
}

IgnoreListViewItem::~IgnoreListViewItem()
{
}

void IgnoreListViewItem::setFlag(int flag,bool active)
{
    if(active) m_flags|=flag;
    else m_flags &= ~flag; //any bits that are set in flag will cause those bits in flags to be set to 0
    setFlags(m_flags);
}

void IgnoreListViewItem::setFlags(int newFlags)
{
    m_flags=newFlags;

    QString flagsStr;
    if(m_flags & Ignore::Channel) flagsStr += i18n("Channel") + QLatin1Char(' ');
    if(m_flags & Ignore::Query) flagsStr += i18n("Query") + QLatin1Char(' ');
    if(m_flags & Ignore::Notice) flagsStr += i18n("Notice") + QLatin1Char(' ');
    if(m_flags & Ignore::CTCP) flagsStr += i18n("CTCP") + QLatin1Char(' ');
    if(m_flags & Ignore::DCC) flagsStr += i18n("DCC") + QLatin1Char(' ');
    if(m_flags & Ignore::Invite) flagsStr += i18n("Invite") + QLatin1Char(' ');
    if(m_flags & Ignore::Exception) flagsStr += i18n("Exception") + QLatin1Char(' ');
    setText(1,flagsStr);
}

