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

#include "nicklistbehavior_config.h"
#include "valuelistviewitem.h"
#include "preferences.h"

#include <kapplication.h>
#include <klocale.h>


NicklistBehavior_Config::NicklistBehavior_Config(QWidget *parent, const char *name)
 : QWidget(parent)
{
  setObjectName(QString::fromLatin1(name));
  setupUi(this);

  // get page widget and populate listview
  loadSettings();
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
  setNickList(Preferences::self()->sortOrder());
  m_oldSortingOrder=currentSortingOrder();
}

void NicklistBehavior_Config::setNickList(const QString &sortingOrder)
{
  sortOrder->clear();
  for(int index = 0; index < sortingOrder.length() ; ++index)
  {
    // get next mode char
    QChar mode=sortingOrder[index];
    QTreeWidgetItem *item = 0;
    // find appropriate description
    if(mode=='-') item = new QTreeWidgetItem(sortOrder, QStringList() << mode << i18n("Normal Users"));
    if(mode=='v') item = new QTreeWidgetItem(sortOrder, QStringList() << mode << i18n("Voice (+v)"));
    if(mode=='h') item = new QTreeWidgetItem(sortOrder, QStringList() << mode << i18n("Halfops (+h)"));
    if(mode=='o') item = new QTreeWidgetItem(sortOrder, QStringList() << mode << i18n("Operators (+o)"));
    if(mode=='p') item = new QTreeWidgetItem(sortOrder, QStringList() << mode << i18n("Channel Admins (+p)"));
    if(mode=='q') item = new QTreeWidgetItem(sortOrder, QStringList() << mode << i18n("Channel Owners (+q)"));
    item->setFlags(item->flags() &~ Qt::ItemIsDropEnabled);
  }
}

QString NicklistBehavior_Config::currentSortingOrder()
{
  // get the uppermost entry of the sorting list
  QTreeWidgetItem* item=sortOrder->topLevelItem(0);
  // prepare the new sorting order string
  QString currentSortingOrder;
  // iterate through all items of the listview
  while(item)
  {
    // add mode char to the sorting order string
    currentSortingOrder+=item->text(0);
    // go to next item in the listview
    item=sortOrder->itemBelow(item);
  } // while

  return currentSortingOrder;
}

// save settings permanently
void NicklistBehavior_Config::saveSettings()
{
  // get the current sorting order
  QString newSortingOrder=currentSortingOrder();

  // update sorting order on in-memory preferences
  Preferences::self()->setSortOrder(newSortingOrder);

  // save current sorting order as a reference to hasChanged()
  m_oldSortingOrder=currentSortingOrder();
}

bool NicklistBehavior_Config::hasChanged()
{
  return(m_oldSortingOrder!=currentSortingOrder());
}

#include "nicklistbehavior_config.moc"
