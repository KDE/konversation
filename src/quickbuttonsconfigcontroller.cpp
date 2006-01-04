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

QuickButtonsConfigController::QuickButtonsConfigController(QuickButtons_Config* quickButtonsPage,QObject *parent, const char *name)
 : QObject(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  // get page widget and populate listview
  m_quickButtonsPage=quickButtonsPage;
  populateQuickButtonsList();

  // make items react to drag & drop
  m_quickButtonsPage->buttonListView->setSorting(-1,false);

  connect(m_quickButtonsPage->buttonListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(m_quickButtonsPage->buttonListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (entrySelected(QListViewItem*)) );
  connect(m_quickButtonsPage->buttonListView,SIGNAL (moved()),this,SIGNAL (modified()) );

  connect(m_quickButtonsPage->nameInput,SIGNAL (textChanged(const QString&)),this,SLOT (nameChanged(const QString&)) );
  connect(m_quickButtonsPage->actionInput,SIGNAL (textChanged(const QString&)),this,SLOT (actionChanged(const QString&)) );
}

QuickButtonsConfigController::~QuickButtonsConfigController()
{
}

// save quick buttons to configuration
void QuickButtonsConfigController::saveSettings()
{
  // get configuration object
  KConfig* config=kapp->config();

  config->setGroup("Button List");

  // get first item of the button listview
  QListViewItem* item=m_quickButtonsPage->buttonListView->firstChild();
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

// fill listview with button definitions
void QuickButtonsConfigController::populateQuickButtonsList()
{
  // get list of quick buttons from preferences
  QStringList buttonList(Preferences::quickButtonList());
  // go through the list
  for(unsigned int index=buttonList.count();index!=0;index--)
  {
    // get button definition
    QString definition=buttonList[index-1];
    // cut definition apart in name and action, and create a new listview item
    new KListViewItem(m_quickButtonsPage->buttonListView,definition.section(',',0,0),definition.section(',',1));
  } // for
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
    m_quickButtonsPage->nameInput->setText(quickButtonEntry->text(0));
    m_quickButtonsPage->actionInput->setText(quickButtonEntry->text(1));
    // re-enable modified() signal on text changes in edit widgets
    newItemSelected=false;
  }
  // enable or disable editing widgets
  m_quickButtonsPage->nameLabel->setEnabled(enabled);
  m_quickButtonsPage->nameInput->setEnabled(enabled);
  m_quickButtonsPage->actionLabel->setEnabled(enabled);
  m_quickButtonsPage->actionInput->setEnabled(enabled);
}

// what to do when the user change the name of a quick button
void QuickButtonsConfigController::nameChanged(const QString& newName)
{
  // get listview object and possible first selected item
  KListView* listView=m_quickButtonsPage->buttonListView;
  QListViewItem* item=listView->selectedItem();

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
  // get listview object and possible first selected item
  KListView* listView=m_quickButtonsPage->buttonListView;
  QListViewItem* item=listView->selectedItem();

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
