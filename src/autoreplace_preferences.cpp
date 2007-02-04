/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include <qlabel.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qtooltip.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>

#include "config/preferences.h"

#include "autoreplace_preferences.h"

#define DIRECTION_OUTPUT 0
#define DIRECTION_INPUT  1
#define DIRECTION_BOTH   2

Autoreplace_Config::Autoreplace_Config(QWidget* parent, const char* name)
 : Autoreplace_ConfigUI(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  m_newItemSelected=false;

  //Check if the regexp editor is installed
  bool installed = !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty();

  if(installed)
  {
      regExpEditorButton->setEnabled(true);
      QToolTip::add(regExpEditorButton, i18n("Click to run Regular Expression Editor (KRegExpEditor)"));
  }
  else
  {
      regExpEditorButton->setEnabled(false);
      QToolTip::add(regExpEditorButton, i18n("The Regular Expression Editor (KRegExpEditor) is not installed"));
  }

  // populate combobox
  directionCombo->insertItem(i18n("Outgoing"),DIRECTION_OUTPUT);
  directionCombo->insertItem(i18n("Incoming"),DIRECTION_INPUT);
  directionCombo->insertItem(i18n("Both"),DIRECTION_BOTH);

  // make items react to drag & drop
  patternListView->setSorting(-1,false);
  patternListView->setShowSortIndicator(true);
  patternListView->setShadeSortColumn(true);
  patternListView->header()->setMovingEnabled(false);

  // populate listview
  loadSettings();

  connect(patternListView, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(entrySelected(QListViewItem*)));
  connect(patternListView, SIGNAL(clicked(QListViewItem*)), this, SLOT(entrySelected(QListViewItem*)));
  connect(patternListView, SIGNAL(moved()), SIGNAL(modified()));

  connect(patternListView, SIGNAL(aboutToMove()), SLOT(disableSort()));
  connect(patternListView->header(), SIGNAL(clicked(int)), SLOT(sort(int)));

  connect(directionCombo, SIGNAL(activated(int)), this, SLOT(directionChanged(int)));

  connect(patternInput, SIGNAL(textChanged(const QString&)), this, SLOT(patternChanged(const QString&)));
  connect(regExpEditorButton, SIGNAL(clicked()), this, SLOT(showRegExpEditor()));
  connect(replacementInput, SIGNAL(textChanged(const QString&)), this, SLOT(replacementChanged(const QString&)));

  connect(newButton, SIGNAL(clicked()), this, SLOT(addEntry()));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeEntry()));
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
    // Regular expression?
    if(definition.section(',',0,0)=="1") newItem->setOn(true);
    // direction input/output/both
    if(definition.section(',',1,1)=="i") newItem->setText(1,directionCombo->text(DIRECTION_INPUT));
    else if(definition.section(',',1,1)=="o") newItem->setText(1,directionCombo->text(DIRECTION_OUTPUT));
    else if(definition.section(',',1,1)=="io") newItem->setText(1,directionCombo->text(DIRECTION_BOTH));
    // pattern
    newItem->setText(2,definition.section(',',2,2));
    // replacement
    newItem->setText(3,definition.section(',',3));
    // hidden column, so we are independent of the i18n()ed display string
    newItem->setText(4,definition.section(',',1,1));
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
      // write the current entry's pattern and replacement (adds a "#" to preserve blanks at the end of the line)
      config->writeEntry(QString("Autoreplace%1").arg(index),newList[index]+'#');
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

    // remember entry in internal list (col 4 is hidden for input/output)
    newList.append(checked+','+item->text(4)+','+item->text(2)+','+item->text(3));
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
    patternInput->setText(autoreplaceEntry->text(2));
    replacementInput->setText(autoreplaceEntry->text(3));

    // set combobox to selected item
    int itemIndex=0;
    QString direction=autoreplaceEntry->text(4);
    if(direction=="i") itemIndex=DIRECTION_INPUT;
    else if(direction=="o") itemIndex=DIRECTION_OUTPUT;
    else if(direction=="io") itemIndex=DIRECTION_BOTH;
    directionCombo->setCurrentItem(itemIndex);
    // re-enable modified() signal on text changes in edit widgets
    m_newItemSelected=false;
  }
  // enable or disable editing widgets
  removeButton->setEnabled(enabled);
  directionLabel->setEnabled(enabled);
  directionCombo->setEnabled(enabled);
  patternLabel->setEnabled(enabled);
  patternInput->setEnabled(enabled);
  replacementLabel->setEnabled(enabled);
  replacementInput->setEnabled(enabled);

  if(!KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty())
  {
    regExpEditorButton->setEnabled(enabled);
  }

  // make checkboxes work
  emit modified();
}

// what to do when the user changes the direction of an entry
void Autoreplace_Config::directionChanged(int newDirection)
{
  // get possible selected item
  QListViewItem* item=patternListView->selectedItem();

  // sanity check
  if(item)
  {
    // prepare hidden identifier string
    QString id;
    // find the direction strings to set up in the item
    if(newDirection==DIRECTION_INPUT)       id="i";
    else if(newDirection==DIRECTION_OUTPUT) id="o";
    else if(newDirection==DIRECTION_BOTH)   id="io";
    // rename direction
    item->setText(1,directionCombo->text(newDirection));
    item->setText(4,id);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// what to do when the user changes the pattern of an entry
void Autoreplace_Config::patternChanged(const QString& newPattern)
{
  // get possible selected item
  QListViewItem* item=patternListView->selectedItem();

  // sanity check
  if(item)
  {
    // rename pattern
    item->setText(2,newPattern);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// what to do when the user changes the replacement of an entry
void Autoreplace_Config::replacementChanged(const QString& newReplacement)
{
  // get possible selected item
  QListViewItem* item=patternListView->selectedItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(3,newReplacement);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// add button pressed
void Autoreplace_Config::addEntry()
{
  disableSort();

  // add new item at the bottom of list view
  QCheckListItem* newItem=new QCheckListItem(patternListView,QString::null,QCheckListItem::CheckBox);
  // if successful ...
  if(newItem)
  {
    // set default direction
    newItem->setText(1,directionCombo->text(DIRECTION_OUTPUT));
    // set default pattern name
    newItem->setText(2,i18n("New"));
    // set default direction
    newItem->setText(4,"o");
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

void Autoreplace_Config::sort(int column)
{
    bool ascending = true;

    if (patternListView->sortColumn() != -1)
        ascending  = (patternListView->sortOrder() == Qt::Ascending);

    patternListView->setSorting(column, ascending);

    emit modified();
}

void Autoreplace_Config::disableSort()
{
    patternListView->setSorting(-1);
}

void Autoreplace_Config::showRegExpEditor()
{
    QDialog *editorDialog =
            KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor" );

    if(editorDialog)
    {
        // kdeutils was installed, so the dialog was found.  Fetch the editor interface.
        KRegExpEditorInterface *reEditor =
                static_cast<KRegExpEditorInterface *>(editorDialog->qt_cast( "KRegExpEditorInterface" ) );
        Q_ASSERT(reEditor); // This should not fail!
        reEditor->setRegExp(patternInput->text());
        int dlgResult = editorDialog->exec();

        if(dlgResult == QDialog::Accepted)
        {
            QString re = reEditor->regExp();
            patternInput->setText(re);
        }

        delete editorDialog;
    }
}

#include "autoreplace_preferences.moc"
