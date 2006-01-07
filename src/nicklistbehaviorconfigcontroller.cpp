//
// C++ Implementation: nicklistbehaviorconfigcontroller
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
#include "nicklistbehaviorconfigcontroller.h"

NicklistBehaviorConfigController::NicklistBehaviorConfigController(NicklistBehavior_Config* nicklistBehaviorPage,QObject *parent, const char *name)
 : QObject(parent, name)
{
  // get page widget and populate listview
  m_nicklistBehaviorPage=nicklistBehaviorPage;
  populateSortingList();

  // make items react to drag & drop
  m_nicklistBehaviorPage->sortOrder->setSorting(-1,false);

  connect(m_nicklistBehaviorPage->sortOrder,SIGNAL (moved()),this,SIGNAL (modified()) );
}

NicklistBehaviorConfigController::~NicklistBehaviorConfigController()
{
}

void NicklistBehaviorConfigController::populateSortingList()
{
  // get pointer to sorting order listview
  KListView* sortOrderListview=m_nicklistBehaviorPage->sortOrder;

  // get sorting order string from preferences
  QString sortingOrder=Preferences::nicknameSortingOrder();

  // loop through the sorting order string, insert the matching descriptions in reverse order
  // to keep the correct sorting
  for(unsigned int index=sortingOrder.length();index!=0;index--)
  {
    // get next mode char
    QChar mode=sortingOrder[index-1];
    // find appropriate description
    if(mode=='-') new KListViewItem(sortOrderListview,mode,i18n("Normal Users"));
    if(mode=='v') new KListViewItem(sortOrderListview,mode,i18n("Voice (+v)"));
    if(mode=='h') new KListViewItem(sortOrderListview,mode,i18n("Halfops (+h)"));
    if(mode=='o') new KListViewItem(sortOrderListview,mode,i18n("Operators (+o)"));
    if(mode=='p') new KListViewItem(sortOrderListview,mode,i18n("Channel Admins (+p)"));
    if(mode=='q') new KListViewItem(sortOrderListview,mode,i18n("Channel Owners (+q)"));
  }
}

// save settings permanently
void NicklistBehaviorConfigController::saveSettings()
{
  // get the uppermost entry of the sorting list
  QListViewItem* item=m_nicklistBehaviorPage->sortOrder->firstChild();

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
  config->writeEntry("SortOrder",newSortingOrder);

  // update sorting order on in-memory preferences
  Preferences::setNicknameSortingOrder(newSortingOrder);
}

#include "nicklistbehaviorconfigcontroller.moc"
