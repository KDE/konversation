// nicklistviewitem.cpp
/***************************************************************************
                          LedListViewItem.cpp  -  A list view with led indicator
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

#include "nicklistviewitem.h"
#include "konversationapplication.h"
#include "nick.h"
#include "nickinfo.h"
#include "nicklistview.h"
#include "images.h"

NickListViewItem::NickListViewItem(KListView* parent,
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
  m_height = this->height();
}

NickListViewItem::~NickListViewItem()
{
}

void NickListViewItem::refresh()
{
  NickInfo* nickInfo = nick->getNickInfo();
  bool away = false;
  if ( nickInfo )
    away = nickInfo->isAway();
  
  //FIXME: remove KonversationApplication::preferences.getOpLedColor() and so on. (obsolete)
  
  Images* images = KonversationApplication::instance()->images();
  QPixmap icon;
  
  if ( nick->isAdmin() )
    icon = images->getNickIcon( Images::Admin, away );
  else if ( nick->isOwner() )
    icon = images->getNickIcon( Images::Owner, away );
  else if ( nick->isOp() )
    icon = images->getNickIcon( Images::Op, away );
  else if ( nick->isHalfop() )
    icon = images->getNickIcon( Images::HalfOp, away );
  else if ( nick->hasVoice() )
    icon = images->getNickIcon( Images::Voice, away );
  else
    icon = images->getNickIcon( Images::Normal, away );
  
  setPixmap( 0, icon );
  
  KABC::Picture pic = nickInfo->getAddressee().photo();
  if(!pic.isIntern())
    pic = nickInfo->getAddressee().logo();
  if(pic.isIntern())
  {
    QPixmap qpixmap(pic.data().scaleHeight(m_height));
    setPixmap(1,qpixmap);
  }
  setText(1,calculateLabel1());
  setText(2,calculateLabel2());
  repaint();
}

QString NickListViewItem::calculateLabel1() {
  NickInfoPtr nickinfo = nick->getNickInfo();
  KABC::Addressee addressee = nickinfo->getAddressee();
  if(!addressee.realName().isEmpty()) //if no addressee, realName will be empty
    return nick->getNickInfo()->getNickname() + " (" + addressee.realName() + ")";
  return nick->getNickInfo()->getNickname();
}
QString NickListViewItem::calculateLabel2() {
  return nick->getNickInfo()->getHostmask();
}
int NickListViewItem::compare(QListViewItem* item,int col,bool ascending) const
{
  NickListViewItem* otherItem=static_cast<NickListViewItem*>(item);

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

int NickListViewItem::getFlags() const
{
  NickInfo* nickInfo = nick->getNickInfo();
  int flags;

  if(nick->isAdmin())           flags=KonversationApplication::preferences.getAdminValue();
  else if(nick->isOwner())      flags=KonversationApplication::preferences.getOwnerValue();
  else if(nick->isOp())         flags=KonversationApplication::preferences.getOpValue();
  else if(nick->isHalfop())     flags=KonversationApplication::preferences.getHalfopValue();
  else if(nick->hasVoice())     flags=KonversationApplication::preferences.getVoiceValue();
  else if(nickInfo->isAway())   flags=KonversationApplication::preferences.getAwayValue();
  else                          flags=KonversationApplication::preferences.getNoRightsValue();

  return flags;
}

Nick *NickListViewItem::getNick() {
  return nick;
}

#include "nicklistviewitem.moc"
