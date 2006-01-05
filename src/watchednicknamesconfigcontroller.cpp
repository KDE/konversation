//
// C++ Implementation: watchednicknamesconfigcontroller
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
#include "watchednicknamesconfigcontroller.h"

WatchedNicknamesConfigController::WatchedNicknamesConfigController(WatchedNicknames_Config* watchedNicknamesPage,QObject *parent, const char *name)
 : QObject(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  m_watchedNicknamesPage=watchedNicknamesPage;
  populateWatchedNicksList();

  connect(m_watchedNicknamesPage->kcfg_UseNotify,SIGNAL (toggled(bool)),this,SLOT (checkIfEmptyListview(bool)) );
  connect(m_watchedNicknamesPage->newButton,SIGNAL (clicked()),this,SLOT (newNotify()) );
  connect(m_watchedNicknamesPage->removeButton,SIGNAL (clicked()),this,SLOT (removeNotify()) );
  connect(m_watchedNicknamesPage->notifyListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(m_watchedNicknamesPage->notifyListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );

  connect(m_watchedNicknamesPage->networkDropdown,SIGNAL (activated(const QString&)),this,SLOT (networkChanged(const QString&)) );
  connect(m_watchedNicknamesPage->nicknameInput,SIGNAL (textChanged(const QString&)),this,SLOT (nicknameChanged(const QString&)) );
}

WatchedNicknamesConfigController::~WatchedNicknamesConfigController()
{
}

// fill in the notify listview with groups and nicknames
void WatchedNicknamesConfigController::populateWatchedNicksList()
{
  // get the current notify list and an iterator
  QMap<QString, QStringList> notifyList = Preferences::notifyList();
  QMapConstIterator<QString, QStringList> groupItEnd = notifyList.constEnd();

  // get list of server networks
  Konversation::ServerGroupList serverGroupList = Preferences::serverGroupList();
  Konversation::ServerGroupList::iterator it;

  // check if there is a network that is not in the notify group list
  for(it=serverGroupList.begin();it!=serverGroupList.end();++it)
  {
    if(!notifyList.contains((*it)->name()))
    {
      // add server network to the notify listview so we can add notify items
      // to networks we haven't used yet
      new KListViewItem(m_watchedNicknamesPage->notifyListView,(*it)->name());
    }
  }

  // iterate over all items
  for (QMapConstIterator<QString, QStringList> groupIt = notifyList.constBegin();
      groupIt != groupItEnd; ++groupIt)
  {
    // get list of nicks for the current group
    QStringList nicks=groupIt.data();
    // create group branch
    KListViewItem* groupItem=new KListViewItem(m_watchedNicknamesPage->notifyListView,groupIt.key());
    // add group to dropdown list
    m_watchedNicknamesPage->networkDropdown->insertItem(groupIt.key(),-1);
    // add nicknames to group branch
    for(unsigned int index=0;index<nicks.count();index++)
    {
      new KListViewItem(groupItem,nicks[index]);
    } // for
    // unfold group branch
    m_watchedNicknamesPage->notifyListView->setOpen(groupItem,true);
  } // for
}

// save list of notifies permanently, taken from the listview
void WatchedNicknamesConfigController::saveSettings()
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
  KListView* listView=m_watchedNicknamesPage->notifyListView;
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
    // write nick list to in-memory notify list
    notifyList.insert(group->text(0),QStringList::split(' ',nicks.stripWhiteSpace()));
    // get next group
    group=group->nextSibling();
  } // while

  // update in-memory notify list
  Preferences::setNotifyList(notifyList);
}

// slots

// helper function to disable "New" button on empty listview
void WatchedNicknamesConfigController::checkIfEmptyListview(bool state)
{
  // only enable "New" button if there is at least one group in the list
  if(!m_watchedNicknamesPage->notifyListView->childCount()) state=false;
  m_watchedNicknamesPage->newButton->setEnabled(state);
}

// add new notify entry
void WatchedNicknamesConfigController::newNotify()
{
  // get listview object and possible first selected item
  KListView* listView=m_watchedNicknamesPage->notifyListView;
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
void WatchedNicknamesConfigController::removeNotify()
{
  // get listview pointer and the selected item
  KListView* listView=m_watchedNicknamesPage->notifyListView;
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
void WatchedNicknamesConfigController::entrySelected(QListViewItem* notifyEntry)
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
      m_watchedNicknamesPage->nicknameInput->setText(notifyEntry->text(0));
      m_watchedNicknamesPage->networkDropdown->setCurrentText(group->text(0));
      // all signals will now emit the modified() signal again
      newItemSelected=false;
    }
  }

  // enable/disable edit widgets
  m_watchedNicknamesPage->removeButton->setEnabled(enabled);
  m_watchedNicknamesPage->networkLabel->setEnabled(enabled);
  m_watchedNicknamesPage->networkDropdown->setEnabled(enabled);
  m_watchedNicknamesPage->nicknameLabel->setEnabled(enabled);
  m_watchedNicknamesPage->nicknameInput->setEnabled(enabled);
}

// user changed the network this nickname is on
void WatchedNicknamesConfigController::networkChanged(const QString& newNetwork)
{
  // get listview pointer and selected entry
  KListView* listView=m_watchedNicknamesPage->notifyListView;
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
void WatchedNicknamesConfigController::nicknameChanged(const QString& newNickname)
{
  // get listview pointer and selected item
  KListView* listView=m_watchedNicknamesPage->notifyListView;
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

#include "watchednicknamesconfigcontroller.moc"
