//
// C++ Implementation: nicklistbehavior_Config
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <kapplication.h>
#include <klocale.h>

#include "valuelistviewitem.h"

#include "config/preferences.h"

#include "nicklistbehavior_preferences.h"

NicklistBehavior_Config::NicklistBehavior_Config(QWidget *parent, const char *name)
 : NicklistBehavior_ConfigUI(parent, name)
{
  // get page widget and populate listview
  loadSettings();

  // make items react to drag & drop
  sortOrder->setSorting(-1,false);

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
  setNickList(Preferences::nicknameSortingOrder());
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

// save settings permanently
void NicklistBehavior_Config::saveSettings()
{
  // get the uppermost entry of the sorting list
  QListViewItem* item=sortOrder->firstChild();

  // prepare the new sorting order string
  QString newSortingOrder;
  // iterate through all items of the listview
  while(item)
  {
    // add mode char to the sorting order string
    newSortingOrder+=item->text(0);
    // go to next item in the listview
    item=item->itemBelow();
  } // while

  // get configuration object and group, write back the new sorting order
  KConfig* config=kapp->config();

  config->setGroup("Sort Nicknames");
  if(newSortingOrder != Preferences::defaultNicknameSortingOrder())
    config->writeEntry("SortOrder",newSortingOrder);
  else
    config->deleteEntry("SortOrder");

  // update sorting order on in-memory preferences
  Preferences::setNicknameSortingOrder(newSortingOrder);
}

#include "nicklistbehavior_preferences.moc"
