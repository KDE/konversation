/***************************************************************************
                          ledlistviewitem.cpp  -  A list view with led indicator
                             -------------------
    begin                : Thu Jul 25 2002
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

#include <kdebug.h>
#include <qobject.h>

#include "ledlistviewitem.h"
#include "konversationapplication.h"
#include "nicklistview.h"

LedListViewItem::LedListViewItem(KListView* parent,
                                 const QString& passed_label,
                                 const QString& passed_label2,
				 Nick *n) :
                   KListViewItem(parent,QString::null,passed_label,passed_label2)
{
  Q_ASSERT(n);
  nick = n;

  connect(nick->getChannelNick(), SIGNAL(channelNickChanged()), SLOT(refresh()));
  connect(nick->getNickInfo(), SIGNAL(nickInfoChanged()), SLOT(refresh()));
  refresh();
}

LedListViewItem::~LedListViewItem()
{
}

void LedListViewItem::refresh() {
  NickListView* lv = dynamic_cast<NickListView*>(listView());

  if(lv) {
    if(nick->isAdmin())
      setPixmap(0, lv->images().getLed(0,true).pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On));
    else if(nick->isOwner())
      setPixmap(0, lv->images().getLed(0, false).pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On));
    else if(nick->isOp())
      setPixmap(0, lv->images().getLed(KonversationApplication::preferences.getOpLedColor(),
        true).pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On));
    else if(nick->isHalfop())
      setPixmap(0, lv->images().getLed(KonversationApplication::preferences.getOpLedColor(),
        false).pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On));
    else if(nick->hasVoice())
      setPixmap(0, lv->images().getLed(KonversationApplication::preferences.getVoiceLedColor(),
        true).pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On));
    else
      setPixmap(0, lv->images().getLed(KonversationApplication::preferences.getNoRightsLedColor(),
        false).pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::Off));
  }
 
  setText(1,calculateLabel1());
  setText(2,calculateLabel2());
  repaint();
}
QString LedListViewItem::calculateLabel1() {
  NickInfoPtr nickinfo = nick->getNickInfo();
  KABC::Addressee addressee = nickinfo->getAddressee();
  if(!addressee.realName().isEmpty()) //if no addressee, realName will be empty
    return nick->getNickInfo()->getNickname() + " (" + addressee.realName() + ")";
  return nick->getNickInfo()->getNickname();
}
QString LedListViewItem::calculateLabel2() {
  return nick->getNickInfo()->getHostmask();
}
int LedListViewItem::compare(QListViewItem* item,int col,bool ascending) const
{
  LedListViewItem* otherItem=static_cast<LedListViewItem*>(item);

  int thisFlags=getFlags();
  int otherFlags=otherItem->getFlags();

  if(KonversationApplication::preferences.getSortByStatus())
  {
    if(thisFlags>otherFlags) return 1;
    if(thisFlags<otherFlags) return -1;
  }

  QString thisKey=key(col,ascending);
  QString otherKey=otherItem->key(col,ascending);

  if(KonversationApplication::preferences.getSortCaseInsensitive())
  {
    thisKey=thisKey.lower();
    otherKey=otherKey.lower();
  }

  return thisKey.compare(otherKey);
}

int LedListViewItem::getFlags() const
{
  int flags;

  if(nick->isAdmin())       flags=KonversationApplication::preferences.getAdminValue();
  else if(nick->isOwner())  flags=KonversationApplication::preferences.getOwnerValue();
  else if(nick->isOp())     flags=KonversationApplication::preferences.getOpValue();
  else if(nick->isHalfop()) flags=KonversationApplication::preferences.getHalfopValue();
  else if(nick->hasVoice())  flags=KonversationApplication::preferences.getVoiceValue();
  else                 flags=KonversationApplication::preferences.getNoRightsValue();

  return flags;
}

Nick *LedListViewItem::getNick() {
  return nick;
}

#include "ledlistviewitem.moc"

