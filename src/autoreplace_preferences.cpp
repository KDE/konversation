//
// C++ Implementation: Autoreplace_Config
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
#include <qpushbutton.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>

#include "config/preferences.h"

#include "autoreplace_preferences.h"

Autoreplace_Config::Autoreplace_Config(QWidget* parent, const char* name)
 : Autoreplace_ConfigUI(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  m_newItemSelected=false;

  // populate listview
  loadSettings();

  // make items react to drag & drop
  patternListView->setSorting(-1,false);

  connect(patternListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(patternListView,SIGNAL (clicked(QListViewItem*)),SIGNAL (modified()));
  connect(patternListView,SIGNAL (moved()),SIGNAL (modified()) );

  connect(patternInput,SIGNAL (textChanged(const QString&)),this,SLOT (patternChanged(const QString&)) );
  connect(replacementInput,SIGNAL (textChanged(const QString&)),this,SLOT (replacementChanged(const QString&)) );

  connect(newButton,SIGNAL (clicked()),this,SLOT (addEntry()));
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeEntry()));
}

Autoreplace_Config::~Autoreplace_Config()
{
}

void Autoreplace_Config::loadSettings()
{
  setAutoreplaceListView(Preferences::autoreplaceList());

  // remember autoreplace list for hasChanged()
  m_oldAutoreplaceList=Preferences::autoreplaceList();
}

// fill listview with autoreplace definitions
void Autoreplace_Config::setAutoreplaceListView(const QStringList &autoreplaceList)
{
  // clear listView
  patternListView->clear();
  // go through the list
  for(unsigned int index=autoreplaceList.count();index!=0;index--)
  {
    // get autoreplace definition
    QString definition=autoreplaceList[index-1];
    // cut definition apart in name and action, and create a new listview item
    QCheckListItem* newItem=new QCheckListItem(patternListView,QString::null,QCheckListItem::CheckBox);
    if(definition.section(',',0,0)=="1") newItem->setOn(true);
    newItem->setText(1,definition.section(',',1,1));
    newItem->setText(2,definition.section(',',2));
  } // for
  patternListView->setSelected(patternListView->firstChild(), true);
}

// save autoreplace entries to configuration
void Autoreplace_Config::saveSettings()
{
  // get configuration object
  KConfig* config=kapp->config();

  // delete all patterns
  config->deleteGroup("Autoreplace List");
  // create new empty autoreplace group
  config->setGroup("Autoreplace List");

  // create empty list
  QStringList newList=currentAutoreplaceList();

  // check if there are any patterns in the list view
  if(newList.count())
  {
    // go through all patterns and save them into the configuration
    for(unsigned int index=0;index<newList.count();index++)
    {
      // write the current entry's pattern and replacement
      config->writeEntry(QString("Autoreplace%1").arg(index),newList[index]);
    } // for
  }
  // if there were no entries at all, write a dummy entry to prevent KConfigXT from "optimizing"
  // the group out, which would in turn make konvi restore the default entries
  else
    config->writeEntry("Empty List",QString::null);

  // set internal autoreplace list
  Preferences::setAutoreplaceList(newList);

  // remember autoreplace list for hasChanged()
  m_oldAutoreplaceList=newList;
}

void Autoreplace_Config::restorePageToDefaults()
{
  setAutoreplaceListView(Preferences::defaultAutoreplaceList());
}

QStringList Autoreplace_Config::currentAutoreplaceList()
{
  // get first item of the autoreplace listview
  QListViewItem* item=patternListView->firstChild();
  // create empty list
  QStringList newList;

  // go through all items and save them into the configuration
  while(item)
  {
    QString checked="0";
    if(static_cast<QCheckListItem*>(item)->isOn()) checked="1";

    // remember entry in internal list
    newList.append(checked+","+item->text(1)+","+item->text(2));
    // get next item in the listview
    item=item->itemBelow();
  } // while

  // return list
  return newList;
}

bool Autoreplace_Config::hasChanged()
{
  return(m_oldAutoreplaceList!=currentAutoreplaceList());
}

// slots

// what to do when the user selects an item
void Autoreplace_Config::entrySelected(QListViewItem* autoreplaceEntry)
{
  // play it safe, assume disabling all widgets first
  bool enabled=false;

  // check if there really was an item selected
  if(autoreplaceEntry)
  {
    // remember to enable the editing widgets
    enabled=true;

    // tell the editing widgets not to emit modified() on signals now
    m_newItemSelected=true;
    // update editing widget contents
    patternInput->setText(autoreplaceEntry->text(1));
    replacementInput->setText(autoreplaceEntry->text(2));
    // re-enable modified() signal on text changes in edit widgets
    m_newItemSelected=false;
  }
  // enable or disable editing widgets
  removeButton->setEnabled(enabled);
  patternLabel->setEnabled(enabled);
  patternInput->setEnabled(enabled);
  replacementLabel->setEnabled(enabled);
  replacementInput->setEnabled(enabled);
}

// what to do when the user change the pattern of an entry
void Autoreplace_Config::patternChanged(const QString& newPattern)
{
  // get possible first selected item
  QListViewItem* item=patternListView->selectedItem();

  // sanity check
  if(item)
  {
    // rename pattern
    item->setText(1,newPattern);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// what to do when the user change the replacement of an entry
void Autoreplace_Config::replacementChanged(const QString& newReplacement)
{
  // get possible first selected item
  QListViewItem* item=patternListView->selectedItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(2,newReplacement);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// add button pressed
void Autoreplace_Config::addEntry()
{
  // add new item at the bottom of list view
  KListViewItem* newItem=new KListViewItem(patternListView,patternListView->lastChild(),i18n("New"),QString::null);
  // if successful ...
  if(newItem)
  {
    // select new item and make it the current one
    patternListView->setSelected(newItem,true);
    patternListView->setCurrentItem(newItem);
    // set input focus on item pattern edit
    patternInput->setFocus();
    // select all text to make overwriting easier
    patternInput->selectAll();
    // tell the config system that something has changed
    emit modified();
  }
}

// remove button pressed
void Autoreplace_Config::removeEntry()
{
  // get possible first selected item
  QListViewItem* item=patternListView->selectedItem();

  // sanity check
  if(item)
  {
    // get item below the current one
    QListViewItem* nextItem=item->itemBelow();
    // if there was none, get the one above
    if(!nextItem) nextItem=item->itemAbove();

    // remove the item from the list
    delete item;

    // check if we found the next item
    if(nextItem)
    {
      // select the item and make it the current item
      patternListView->setSelected(nextItem,true);
      patternListView->setCurrentItem(nextItem);
    }
    else
    {
      // no next item found, this means the list is empty
      entrySelected(0);
    }
    // tell the config system that somethig has changed
    emit modified();
  }
}

#include "autoreplace_preferences.moc"
