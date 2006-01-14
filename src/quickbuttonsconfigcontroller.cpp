//
// C++ Implementation: quickbuttonsconfigcontroller
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

#include <kapplication.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klistview.h>

#include "config/preferences.h"

#include "quickbuttons_preferences.h"
#include "quickbuttonsconfigcontroller.h"

QuickButtonsConfigController::QuickButtonsConfigController(QWidget* parent, const char* name)
 : QuickButtons_Config(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  // populate listview
  loadSettings();

  // make items react to drag & drop
  buttonListView->setSorting(-1,false);

  connect(buttonListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(buttonListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(buttonListView,SIGNAL (moved()),this,SIGNAL (modified()) );

  connect(nameInput,SIGNAL (textChanged(const QString&)),this,SLOT (nameChanged(const QString&)) );
  connect(actionInput,SIGNAL (textChanged(const QString&)),this,SLOT (actionChanged(const QString&)) );
}

QuickButtonsConfigController::~QuickButtonsConfigController()
{
}

// fill listview with button definitions
void QuickButtonsConfigController::loadSettings()
{
  // clear listView
  buttonListView->clear();
  // get list of quick buttons from preferences
  QStringList buttonList(Preferences::quickButtonList());
  // go through the list
  for(unsigned int index=buttonList.count();index!=0;index--)
  {
    // get button definition
    QString definition=buttonList[index-1];
    // cut definition apart in name and action, and create a new listview item
    new KListViewItem(buttonListView,definition.section(',',0,0),definition.section(',',1));
  } // for
}

// save quick buttons to configuration
void QuickButtonsConfigController::saveSettings()
{
  // get configuration object
  KConfig* config=kapp->config();

  config->setGroup("Button List");

  // get first item of the button listview
  QListViewItem* item=buttonListView->firstChild();
  // create empty list
  QStringList newList;

  // go through all items and save them into the configuration
  unsigned int index=0;
  while(item)
  {
    // write the current button's name and definition
    config->writeEntry(QString("Button%1").arg(index),item->text(0)+","+item->text(1));
    // remember button in internal list
    newList.append(item->text(0)+","+item->text(1));
    // increment number of button
    index++;
    // get next item in the listview
    item=item->itemBelow();
  } // while

  // set internal button list
  Preferences::setQuickButtonList(newList);
}

void QuickButtonsConfigController::restorePageToDefaults()
{
  // FIXME:
}

// slots

// what to do when the user selects an item
void QuickButtonsConfigController::entrySelected(QListViewItem* quickButtonEntry)
{
  // play it safe, assume disabling all widgets first
  bool enabled=false;

  // check if there really was an item selected
  if(quickButtonEntry)
  {
    // remember to enable the editing widgets
    enabled=true;

    // tell the editing widgets not to emit modified() on singals now
    newItemSelected=true;
    // update editing widget contents
    nameInput->setText(quickButtonEntry->text(0));
    actionInput->setText(quickButtonEntry->text(1));
    // re-enable modified() signal on text changes in edit widgets
    newItemSelected=false;
  }
  // enable or disable editing widgets
  nameLabel->setEnabled(enabled);
  nameInput->setEnabled(enabled);
  actionLabel->setEnabled(enabled);
  actionInput->setEnabled(enabled);
}

// what to do when the user change the name of a quick button
void QuickButtonsConfigController::nameChanged(const QString& newName)
{
  // get possible first selected item
  QListViewItem* item=buttonListView->selectedItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(0,newName);
    // tell the config system that something has changed
    if(!newItemSelected) emit modified();
  }
}

// what to do when the user change the action definition of a quick button
void QuickButtonsConfigController::actionChanged(const QString& newAction)
{
  // get possible first selected item
  QListViewItem* item=buttonListView->selectedItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(1,newAction);
    // tell the config system that something has changed
    if(!newItemSelected) emit modified();
  }
}

#include "quickbuttonsconfigcontroller.moc"
