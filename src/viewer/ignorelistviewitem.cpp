/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Die Jun 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "ignorelistviewitem.h"
#include "ignore.h"

#include <klocale.h>
#include <kdebug.h>


IgnoreListViewItem::IgnoreListViewItem(QTreeWidget* parent,const QString& name,int newFlags):
QTreeWidgetItem(parent, QStringList() << name)
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
    if(m_flags & Ignore::Channel) flagsStr += i18n("Channel") + ' ';
    if(m_flags & Ignore::Query) flagsStr += i18n("Query") + ' ';
    if(m_flags & Ignore::Notice) flagsStr += i18n("Notice") + ' ';
    if(m_flags & Ignore::CTCP) flagsStr += i18n("CTCP") + ' ';
    if(m_flags & Ignore::DCC) flagsStr += i18n("DCC") + ' ';
    if(m_flags & Ignore::Exception) flagsStr += i18n("Exception") + ' ';
    setText(1,flagsStr);
}

