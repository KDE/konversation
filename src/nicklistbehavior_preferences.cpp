/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include "nicklistbehavior_preferences.h"
#include "valuelistviewitem.h"
#include "config/preferences.h"

#include <q3header.h>

#include <kapplication.h>
#include <klocale.h>


NicklistBehavior_Config::NicklistBehavior_Config(QWidget *parent, const char *name)
 : NicklistBehavior_ConfigUI(parent, name)
{
  // get page widget and populate listview
  loadSettings();

  // make items react to drag & drop
  sortOrder->setSorting(-1,false);
  sortOrder->header()->setMovingEnabled(false);

  connect(sortOrder,SIGNAL (moved()),this,SIGNAL (modified()) );
}

NicklistBehavior_Config::~NicklistBehavior_Config()
{
}

void NicklistBehavior_Config::restorePageToDefaults()
{
  setNickList(Preferences::defaultNicknameSortingOrder());
}

void NicklistBehavior_Config::loadSettings()
{
  // get sorting order string from preferences
  setNickList(Preferences::sortOrder());
  m_oldSortingOrder=currentSortingOrder();
}

void NicklistBehavior_Config::setNickList(const QString &sortingOrder)
{
  sortOrder->clear();
  // loop through the sorting order string, insert the matching descriptions in reverse order
  // to keep the correct sorting
  for(unsigned int index=sortingOrder.length();index!=0;index--)
  {
    // get next mode char
    QChar mode=sortingOrder[index-1];
    // find appropriate description
    if(mode=='-') new KListViewItem(sortOrder,mode,i18n("Normal Users"));
    if(mode=='v') new KListViewItem(sortOrder,mode,i18n("Voice (+v)"));
    if(mode=='h') new KListViewItem(sortOrder,mode,i18n("Halfops (+h)"));
    if(mode=='o') new KListViewItem(sortOrder,mode,i18n("Operators (+o)"));
    if(mode=='p') new KListViewItem(sortOrder,mode,i18n("Channel Admins (+p)"));
    if(mode=='q') new KListViewItem(sortOrder,mode,i18n("Channel Owners (+q)"));
  }
}

QString NicklistBehavior_Config::currentSortingOrder()
{
  // get the uppermost entry of the sorting list
  Q3ListViewItem* item=sortOrder->firstChild();
  // prepare the new sorting order string
  QString currentSortingOrder;
  // iterate through all items of the listview
  while(item)
  {
    // add mode char to the sorting order string
    currentSortingOrder+=item->text(0);
    // go to next item in the listview
    item=item->itemBelow();
  } // while

  return currentSortingOrder;
}

// save settings permanently
void NicklistBehavior_Config::saveSettings()
{
  // get the current sorting order
  QString newSortingOrder=currentSortingOrder();

  // update sorting order on in-memory preferences
  Preferences::setSortOrder(newSortingOrder);

  // save current sorting order as a reference to hasChanged()
  m_oldSortingOrder=currentSortingOrder();
}

bool NicklistBehavior_Config::hasChanged()
{
  return(m_oldSortingOrder!=currentSortingOrder());
}

#include "nicklistbehavior_preferences.moc"
