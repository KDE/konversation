/***************************************************************************
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
				   QListViewItem *after,
				   const QString& passed_label,
				   const QString& passed_label2,
				   Nick *n) :
  KListViewItem(parent,after,QString::null,passed_label,passed_label2)
{
  Q_ASSERT(n);
  nick = n;
  m_flags = 0;
  
  m_height = height();
  connect(nick->getChannelNick(), SIGNAL(channelNickChanged()), SLOT(refresh()));
  connect(nick->getNickInfo(), SIGNAL(nickInfoChanged()), SLOT(refresh()));
  
  refresh();
}

NickListViewItem::~NickListViewItem()
{
}

void NickListViewItem::refresh()
{
  bool needResort = false;
  int flags = 0;
  NickInfo* nickInfo = nick->getNickInfo();
  bool away = false;
  
  if ( nickInfo ) {
    away = nickInfo->isAway();
  }

  if(away)
    flags=1;

  Images* images = KonversationApplication::instance()->images();
  QPixmap icon;
  
  if ( nick->isAdmin() ) {
    flags += 64;
    icon = images->getNickIcon( Images::Admin, away );
  } else if ( nick->isOwner() ) {
    flags += 32;
    icon = images->getNickIcon( Images::Owner, away );
  } else if ( nick->isOp() ) {
    flags += 16;
    icon = images->getNickIcon( Images::Op, away );
  } else if ( nick->isHalfop() ) {
    flags += 8;
    icon = images->getNickIcon( Images::HalfOp, away );
  } else if ( nick->hasVoice() ) {
    flags += 4;
    icon = images->getNickIcon( Images::Voice, away );
  } else {
    flags += 2;
    icon = images->getNickIcon( Images::Normal, away );
  }

  if(flags != m_flags)
    {
      needResort = true;
      m_flags = flags;
    }
  
  setPixmap( 0, icon );
  
  KABC::Picture pic = nickInfo->getAddressee().photo();

  if(!pic.isIntern()) {
    pic = nickInfo->getAddressee().logo();
  }

  if(pic.isIntern())
  {
    QPixmap qpixmap(pic.data().scaleHeight(m_height));
    setPixmap(1,qpixmap);
  }

  QString newtext1 = calculateLabel1();
  if(newtext1 != text(1))
    setText(1,calculateLabel1());

  setText(2,calculateLabel2());
  repaint();

  if(needResort)
    emit refreshed(); // Resort nick list
}

QString NickListViewItem::calculateLabel1() {
  NickInfoPtr nickinfo = nick->getNickInfo();
  KABC::Addressee addressee = nickinfo->getAddressee();

  if(!addressee.realName().isEmpty()) {//if no addressee, realName will be empty
    return nick->getNickInfo()->getNickname() + " (" + addressee.realName() + ")";
  }

  return nick->getNickInfo()->getNickname();
}

QString NickListViewItem::calculateLabel2() {
  return nick->getNickInfo()->getHostmask();
}

int NickListViewItem::compare(QListViewItem* item,int col,bool ascending) const
{
  NickListViewItem* otherItem = static_cast<NickListViewItem*>(item);

  if(KonversationApplication::preferences.getSortByStatus())
  {
    int thisFlags = getFlags();
    int otherFlags = otherItem->getFlags();

    if(thisFlags > otherFlags) {
      return 1;
    }
    if(thisFlags < otherFlags) {
      return -1;
    }
  }

  QString thisKey;
  QString otherKey;

  if(col > 1) {
    if(KonversationApplication::preferences.getSortCaseInsensitive())
    {
      thisKey = thisKey.lower();
      otherKey = otherKey.lower();
    } else {
      thisKey = key(col, ascending);
      otherKey = otherItem->key(col, ascending);
    }
  } else if(col == 1) {
    if(KonversationApplication::preferences.getSortCaseInsensitive())
    {
      thisKey = nick->loweredNickname();
      otherKey = otherItem->getNick()->loweredNickname();
    } else {
      thisKey = key(col, ascending);
      otherKey = otherItem->key(col, ascending);
    }
  }
 
  return thisKey.compare(otherKey);
}

int NickListViewItem::getFlags() const
{
  NickInfo* nickInfo = nick->getNickInfo();
  int flags;

  if(nick->isAdmin()) {
    flags = KonversationApplication::preferences.getAdminValue();
  } else if(nick->isOwner()) {
    flags = KonversationApplication::preferences.getOwnerValue();
  } else if(nick->isOp()) {
    flags = KonversationApplication::preferences.getOpValue();
  } else if(nick->isHalfop()) {
    flags = KonversationApplication::preferences.getHalfopValue();
  } else if(nick->hasVoice()) {
    flags = KonversationApplication::preferences.getVoiceValue();
  } else if(nickInfo->isAway()) {
    flags = KonversationApplication::preferences.getAwayValue();
  } else {
    flags = KonversationApplication::preferences.getNoRightsValue();
  }

  return flags;
}

Nick *NickListViewItem::getNick() {
  return nick;
}

#include "nicklistviewitem.moc"
