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

#include "ledlistviewitem.h"
#include "konversationapplication.h"

LedListViewItem::LedListViewItem(KListView* parent,
                                 const QString& passed_label,
                                 const QString& passed_label2,
                                 bool admin,
                                 bool owner,
                                 bool op,
                                 bool halfop,
                                 bool voice) :
                   KListViewItem(parent,passed_label,passed_label2)
{
  adminState=admin;
  ownerState=owner;
  opState=op;
  halfopState=halfop;
  voiceState=voice;

  currentLeds=leds.getLed(0,true);
  adminLedOn =currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

  currentLeds=leds.getLed(0,false);
  ownerLedOff=currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

  currentLeds=leds.getLed(KonversationApplication::preferences.getOpLedColor(),false);
  opLedOff   =currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

  currentLeds=leds.getLed(KonversationApplication::preferences.getOpLedColor(),true);
  opLedOn    =currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

  currentLeds=leds.getLed(KonversationApplication::preferences.getNoRightsLedColor(),false);
  voiceLedOff=currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::Off);

  currentLeds=leds.getLed(KonversationApplication::preferences.getVoiceLedColor(),true);
  voiceLedOn =currentLeds.pixmap(QIconSet::Automatic, QIconSet::Active, QIconSet::On);

  // separate LED from Text a little more
  listView()->setColumnWidth(0,opLedOn.width()+2);
  listView()->setColumnAlignment(0,Qt::AlignHCenter);
  listView()->setColumnAlignment(1,Qt::AlignLeft);
  listView()->setColumnAlignment(2,Qt::AlignLeft);

  setText(0,QString::null);
  setState(admin,owner,op,halfop,voice);
}

LedListViewItem::~LedListViewItem()
{
}

void LedListViewItem::setState(bool admin,bool owner,bool op,bool halfop,bool voice)
{
  adminState=admin;
  ownerState=owner;
  opState=op;
  halfopState=halfop;
  voiceState=voice;

  if(admin)
    setPixmap(0,adminLedOn);
  else if(owner)
    setPixmap(0,ownerLedOff);
  else if(op)
    setPixmap(0,opLedOn);
  else if(halfop)
    setPixmap(0,opLedOff);
  else if(voiceState)
    setPixmap(0,voiceLedOn);
  else
    setPixmap(0,voiceLedOff);

  repaint();
}

void LedListViewItem::toggleOpState()
{
  setState(adminState,ownerState,!opState,halfopState,voiceState);
  repaint();
}

void LedListViewItem::toggleVoiceState()
{
  setState(adminState,ownerState,opState,halfopState,!voiceState);
  repaint();
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

bool LedListViewItem::getAdminState()  { return adminState; }
bool LedListViewItem::getOwnerState()  { return ownerState; }
bool LedListViewItem::getOpState()     { return opState; }
bool LedListViewItem::getHalfopState() { return halfopState; }
bool LedListViewItem::getVoiceState()  { return voiceState; }

int LedListViewItem::getFlags() const
{
  int flags;

  if(adminState)       flags=KonversationApplication::preferences.getAdminValue();
  else if(ownerState)  flags=KonversationApplication::preferences.getOwnerValue();
  else if(opState)     flags=KonversationApplication::preferences.getOpValue();
  else if(halfopState) flags=KonversationApplication::preferences.getHalfopValue();
  else if(voiceState)  flags=KonversationApplication::preferences.getVoiceValue();
  else                 flags=KonversationApplication::preferences.getNoRightsValue();

  return flags;
}
