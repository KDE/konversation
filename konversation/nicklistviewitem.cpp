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

#include <qbitmap.h>
#include <qobject.h>
#include <qpainter.h>

#include <kdebug.h>
#include <kiconloader.h>

#include "nicklistviewitem.h"
#include "konversationapplication.h"
#include "nickinfo.h"
#include "nicklistview.h"

NickListViewItem::NickListViewItem(KListView* parent,
                                 const QString& passed_label,
                                 const QString& passed_label2,
				 Nick *n) :
                   KListViewItem(parent,QString::null,passed_label,passed_label2)
{
  Q_ASSERT(n);
  nick = n;
  initializeIcons();
  
  connect(nick->getChannelNick(), SIGNAL(channelNickChanged()), SLOT(refresh()));
  connect(nick->getNickInfo(), SIGNAL(nickInfoChanged()), SLOT(refresh()));
  refresh();
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
  
  if(nick->isAdmin())
    setPixmap( 0, !away ? *s_pIconAdmin : *s_pIconAdminAway );
  else if(nick->isOwner())
    setPixmap( 0, !away ? *s_pIconOwner : *s_pIconOwnerAway );
  else if(nick->isOp())
    setPixmap( 0, !away ? *s_pIconOp : *s_pIconOpAway );
  else if(nick->isHalfop())
    setPixmap( 0, !away ? *s_pIconHalfOp : *s_pIconHalfOpAway );
  else if(nick->hasVoice())
    setPixmap( 0, !away ? *s_pIconVoice : *s_pIconVoiceAway );
  else
    setPixmap( 0, !away ? *s_pIconNormal : *s_pIconNormalAway );
 
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
  int flags;

  if(nick->isAdmin())       flags=KonversationApplication::preferences.getAdminValue();
  else if(nick->isOwner())  flags=KonversationApplication::preferences.getOwnerValue();
  else if(nick->isOp())     flags=KonversationApplication::preferences.getOpValue();
  else if(nick->isHalfop()) flags=KonversationApplication::preferences.getHalfopValue();
  else if(nick->hasVoice())  flags=KonversationApplication::preferences.getVoiceValue();
  else                 flags=KonversationApplication::preferences.getNoRightsValue();

  return flags;
}

Nick *NickListViewItem::getNick() {
  return nick;
}

//TODO: there's room for optimization as pahlibar said. (strm)

// the below two functions were taken from kopeteonlinestatus.cpp.
static QBitmap overlayMasks( const QBitmap *under, const QBitmap *over )
{
  if ( !under && !over ) return QBitmap();
  if ( !under ) return *over;
  if ( !over ) return *under;

  QBitmap result = *under;
  bitBlt( &result, 0, 0, over, 0, 0, over->width(), over->height(), Qt::OrROP );
  return result;
}

static QPixmap overlayPixmaps( const QPixmap &under, const QPixmap &over )
{
  if ( over.isNull() ) return under;

  QPixmap result = under;
  result.setMask( overlayMasks( under.mask(), over.mask() ) );

  QPainter p( &result );
  p.drawPixmap( 0, 0, over );
  return result;
}

void NickListViewItem::initializeIcons()  // static
{
  if ( s_bIconsInitialized )
    return;
  s_bIconsInitialized = true;
  
  KIconLoader* loader = KGlobal::instance()->iconLoader();
  
  QPixmap elementNormal = loader->loadIcon( "irc_normal", KIcon::Small, 16 );  // base
  QPixmap elementAway   = loader->loadIcon( "irc_away",   KIcon::Small, 16 );
  QPixmap elementVoice  = loader->loadIcon( "irc_voice",  KIcon::Small, 16 );
  QPixmap elementHalfOp = loader->loadIcon( "irc_halfop", KIcon::Small, 16 );
  QPixmap elementOp     = loader->loadIcon( "irc_op",     KIcon::Small, 16 );
  QPixmap elementOwner  = loader->loadIcon( "irc_owner",  KIcon::Small, 16 );
  QPixmap elementAdmin  = loader->loadIcon( "irc_admin",  KIcon::Small, 16 );
  
  s_pIconNormal = new QPixmap( elementNormal );
  s_pIconNormalAway = new QPixmap( overlayPixmaps( *s_pIconNormal, elementAway ) );
  
  s_pIconVoice = new QPixmap( overlayPixmaps( elementNormal, elementVoice ) );
  s_pIconVoiceAway = new QPixmap( overlayPixmaps( *s_pIconVoice, elementAway ) );
  
  s_pIconHalfOp = new QPixmap( overlayPixmaps( elementNormal, elementHalfOp ) );
  s_pIconHalfOpAway = new QPixmap( overlayPixmaps( *s_pIconHalfOp, elementAway ) );
  
  s_pIconOp = new QPixmap( overlayPixmaps( elementNormal, elementOp ) );
  s_pIconOpAway = new QPixmap( overlayPixmaps( *s_pIconOp, elementAway ) );
  
  s_pIconOwner = new QPixmap( overlayPixmaps( elementNormal, elementOwner ) );
  s_pIconOwnerAway = new QPixmap( overlayPixmaps( *s_pIconOwner, elementAway ) );
  
  s_pIconAdmin = new QPixmap( overlayPixmaps( elementNormal, elementAdmin ) );
  s_pIconAdminAway = new QPixmap( overlayPixmaps( *s_pIconAdmin, elementAway ) );
  
  /*
  // why doesn't it work?
  s_pIconOp = new QPixmap( iconNormal );
  bitBlt( s_pIconOp, 0, 0, &iconOp, 0, 0, -1, -1, Qt::CopyROP );
  s_pIconOpAway = new QPixmap( *s_pIconOp );
  bitBlt( s_pIconOpAway, 0, 0, &iconAway, 0, 0, -1, -1, Qt::CopyROP );
  */
}

bool NickListViewItem::s_bIconsInitialized = false;
// we can't create an instance of QPixmap before constructing that of QApplication
// so we use its pointers.
QPixmap *NickListViewItem::s_pIconNormal, *NickListViewItem::s_pIconNormalAway,
        *NickListViewItem::s_pIconVoice,  *NickListViewItem::s_pIconVoiceAway,
        *NickListViewItem::s_pIconHalfOp, *NickListViewItem::s_pIconHalfOpAway,
        *NickListViewItem::s_pIconOp,     *NickListViewItem::s_pIconOpAway,
        *NickListViewItem::s_pIconOwner,  *NickListViewItem::s_pIconOwnerAway,
        *NickListViewItem::s_pIconAdmin,  *NickListViewItem::s_pIconAdminAway;

#include "nicklistviewitem.moc"
