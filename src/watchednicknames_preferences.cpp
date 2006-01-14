//
// C++ Implementation: WatchedNicknames_Config
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>

#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>

#include "config/preferences.h"

#include "watchednicknames_preferences.h"
#include "watchednicknames_preferencesui.h"

WatchedNicknames_Config::WatchedNicknames_Config(QWidget *parent, const char *name)
 : WatchedNicknames_ConfigUI(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  loadSettings();

  connect(kcfg_UseNotify,SIGNAL (toggled(bool)),this,SLOT (checkIfEmptyListview(bool)) );
  connect(newButton,SIGNAL (clicked()),this,SLOT (newNotify()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeNotify()) );
  connect(notifyListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(notifyListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );

  connect(networkDropdown,SIGNAL (activated(const QString&)),this,SLOT (networkChanged(const QString&)) );
  connect(nicknameInput,SIGNAL (textChanged(const QString&)),this,SLOT (nicknameChanged(const QString&)) );
}

WatchedNicknames_Config::~WatchedNicknames_Config()
{
}

void WatchedNicknames_Config::restorePageToDefaults()
{
}

// fill in the notify listview with groups and nicknames
void WatchedNicknames_Config::loadSettings()
{
  // get the current notify list and an iterator
  QMap<QString, QStringList> notifyList = Preferences::notifyList();
  QMapConstIterator<QString, QStringList> groupItEnd = notifyList.constEnd();

  // get list of server networks
  Konversation::ServerGroupList serverGroupList = Preferences::serverGroupList();
  Konversation::ServerGroupList::iterator it;

  notifyListView->clear();
  // check if there is a network that is not in the notify group list
  for(it=serverGroupList.begin();it!=serverGroupList.end();++it)
  {
    if(!notifyList.contains((*it)->name()))
    {
      // add server network to the notify listview so we can add notify items
      // to networks we haven't used yet
      new KListViewItem(notifyListView,(*it)->name());
    }
  }

  // iterate over all items
  for (QMapConstIterator<QString, QStringList> groupIt = notifyList.constBegin();
      groupIt != groupItEnd; ++groupIt)
  {
    // get list of nicks for the current group
    QStringList nicks=groupIt.data();
    // create group branch
    KListViewItem* groupItem=new KListViewItem(notifyListView,groupIt.key());
    // add group to dropdown list
    networkDropdown->insertItem(groupIt.key(),-1);
    // add nicknames to group branch
    for(unsigned int index=0;index<nicks.count();index++)
    {
      new KListViewItem(groupItem,nicks[index]);
    } // for
    // unfold group branch
    notifyListView->setOpen(groupItem,true);
  } // for
}

// save list of notifies permanently, taken from the listview
void WatchedNicknames_Config::saveSettings()
{
  // get configuration object
  KConfig* config=kapp->config();
  // remove all old notify entries
  config->deleteGroup("Notify Groups List");
  // add new notify section
  config->setGroup("Notify Group Lists");

  // create new in-memory notify structure
  QMap<QString,QStringList> notifyList;

  // get first notify group
  KListView* listView=notifyListView;
  QListViewItem* group=listView->firstChild();

  // loop as long as there are more groups in the listview
  while(group)
  {
    // later contains all nicks separated by blanks
    QString nicks;
    // get first nick in the group
    QListViewItem* nick=group->firstChild();
    // loop as long as there are still nicks in this group
    while(nick)
    {
      // add nick to string container and add a blank
      nicks+=nick->text(0)+" ";
      // get next nick in the group
      nick=nick->nextSibling();
    }
    // write nick list to config, strip all unnecessary blanks
    config->writeEntry(group->text(0),nicks.stripWhiteSpace());
    // write nick list to in-memory notify qstringlist
    notifyList.insert(group->text(0),QStringList::split(' ',nicks.stripWhiteSpace()));
    // get next group
    group=group->nextSibling();
  } // while

  // update in-memory notify list
  Preferences::setNotifyList(notifyList);
}

// slots

// helper function to disable "New" button on empty listview
void WatchedNicknames_Config::checkIfEmptyListview(bool state)
{
  // only enable "New" button if there is at least one group in the list
  if(!notifyListView->childCount()) state=false;
  newButton->setEnabled(state);
}

// add new notify entry
void WatchedNicknames_Config::newNotify()
{
  // get listview object and possible first selected item
  KListView* listView=notifyListView;
  QListViewItem* item=listView->selectedItem();

  // if there was an item selected, try to find the group it belongs to,
  // so the newly created item will go into the same group, otherwise
  // just create the new entry inside of the first group
  if(item)
  {
    if(item->parent()) item=item->parent();
  }
  else
    item=listView->firstChild();

  // finally insert new item
  item=new QListViewItem(item,i18n("New"));
  // make this item the current and selected item
  item->setSelected(true);
  listView->setCurrentItem(item);
  // update network and nickname inputs
  entrySelected(item);
  // unfold group branch in case it was empty before
  listView->setOpen(item->parent(),true);
  // tell the config system that something has changed
  emit modified();
}

// remove a notify entry from the listview
void WatchedNicknames_Config::removeNotify()
{
  // get listview pointer and the selected item
  KListView* listView=notifyListView;
  QListViewItem* item=listView->selectedItem();

  // sanity check
  if(item)
  {
    // check which item to highlight after we deleted the chosen one
    QListViewItem* itemAfter=item->itemBelow();
    if(!itemAfter) itemAfter=item->itemAbove();
    delete(item);

    // if there was an item, highlight it
    if(itemAfter)
    {
      itemAfter->setSelected(true);
      listView->setCurrentItem(itemAfter);
      // update input widgets
      entrySelected(itemAfter);
    }
    // tell the config system that something has changed
    emit modified();
  }
}

// what to do when the user selects an entry
void WatchedNicknames_Config::entrySelected(QListViewItem* notifyEntry)
{
  // play it safe, assume disabling all widgets first
  bool enabled=false;

  // if there actually was an entry selected ...
  if(notifyEntry)
  {
    // is this entry a nickname?
    QListViewItem* group=notifyEntry->parent();
    if(group)
    {
      // all edit widgets may be enabled
      enabled=true;
      // tell all now emitted signals that we just clicked on a new item, so they should
      // not emit the modified() signal.
      newItemSelected=true;
      // copy network name and nickname to edit widgets
      nicknameInput->setText(notifyEntry->text(0));
      networkDropdown->setCurrentText(group->text(0));
      // all signals will now emit the modified() signal again
      newItemSelected=false;
    }
  }

  // enable/disable edit widgets
  removeButton->setEnabled(enabled);
  networkLabel->setEnabled(enabled);
  networkDropdown->setEnabled(enabled);
  nicknameLabel->setEnabled(enabled);
  nicknameInput->setEnabled(enabled);
}

// user changed the network this nickname is on
void WatchedNicknames_Config::networkChanged(const QString& newNetwork)
{
  // get listview pointer and selected entry
  KListView* listView=notifyListView;
  QListViewItem* item=listView->selectedItem();

  // sanity check
  if(item)
  {
    // get group the nickname is presently associated to
    QListViewItem* group=item->parent();
    // did the user actually change anything?
    if(group && group->text(0)!=newNetwork)
    {
      // find the branch the new network is in
      QListViewItem* lookGroup=listView->firstChild();
      while(lookGroup && (lookGroup->text(0)!=newNetwork)) lookGroup=lookGroup->nextSibling();
      // if it was found (should never fail)
      if(lookGroup)
      {
        // deselect nickname, unlink it and relink it to the new network
        item->setSelected(false);
        group->takeItem(item);
        lookGroup->insertItem(item);
        // make the moved nickname current and selected item
        item->setSelected(true);
        listView->setCurrentItem(item);
        // unfold group branch in case it was empty before
        listView->setOpen(lookGroup,true);
        // tell the config system that something has changed
        if(!newItemSelected) emit modified();
      }
    }
  }
}

// the user edited the nickname
void WatchedNicknames_Config::nicknameChanged(const QString& newNickname)
{
  // get listview pointer and selected item
  KListView* listView=notifyListView;
  QListViewItem* item=listView->selectedItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(0,newNickname);
    // tell the config system that something has changed
    if(!newItemSelected) emit modified();
  }
}

#include "watchednicknames_preferences.moc"
