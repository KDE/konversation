/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#include "alias_preferences.h"
#include "config/preferences.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <q3header.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <klineedit.h>
#include <k3listview.h>


Alias_Config::Alias_Config(QWidget* parent, const char* name)
 : Alias_ConfigUI(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  m_newItemSelected = false;

  // populate listview
  loadSettings();

  // make items react to drag & drop
  aliasListView->setSorting(-1,false);
  aliasListView->header()->setMovingEnabled(false);

  connect(aliasListView, SIGNAL(selectionChanged(Q3ListViewItem*)), this, SLOT(entrySelected(Q3ListViewItem*)));
  connect(aliasListView, SIGNAL(clicked(Q3ListViewItem*)), this, SLOT(entrySelected(Q3ListViewItem*)) );
  connect(aliasListView, SIGNAL(moved()), this, SIGNAL(modified()));

  connect(aliasInput, SIGNAL(textChanged(const QString&)), this, SLOT(nameChanged(const QString&)));
  connect(replacementInput, SIGNAL(textChanged(const QString&)), this, SLOT(actionChanged(const QString&)));

  connect(newButton, SIGNAL(clicked()), this, SLOT(addEntry()));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeEntry()));
}

Alias_Config::~Alias_Config()
{
}

void Alias_Config::loadSettings()
{
    setAliasListView(Preferences::aliasList());
}

void Alias_Config::saveSettings()
{
    QStringList newList=currentAliasList();
    Preferences::setAliasList(newList);

    // saved list is now old list, to check for changes
    m_oldAliasList=newList;
}

void Alias_Config::restorePageToDefaults()
{
    aliasListView->clear();
    setAliasListView(Preferences::defaultAliasList());
}

bool Alias_Config::hasChanged()
{
    return (currentAliasList() != m_oldAliasList);
}

void Alias_Config::setAliasListView(const QStringList& aliasList)
{
    aliasListView->clear();

    // Insert alias items backwards to get them sorted properly
    for(int index=aliasList.count(); index!=0; index--)
    {
        QString item=aliasList[index-1];
        new K3ListViewItem(aliasListView,item.section(' ',0,0),item.section(' ',1));
    }

    aliasListView->setSelected(aliasListView->firstChild(), true);
    // remember alias list
    m_oldAliasList=aliasList;
}

QStringList Alias_Config::currentAliasList()
{
    QStringList newList;

    Q3ListViewItem* item=aliasListView->itemAtIndex(0);
    while(item)
        {
        newList.append(item->text(0)+' '+item->text(1));
        item=item->itemBelow();
        }
    return newList;
}

// what to do when the user selects an item
void Alias_Config::entrySelected(Q3ListViewItem* aliasEntry)
{
    // play it safe, assume disabling all widgets first
    bool enabled = false;

    // check if there really was an item selected
    if (aliasEntry)
    {
        // remember to enable the editing widgets
        enabled = true;
        // tell the editing widgets not to emit modified() on signals now
        m_newItemSelected = true;
        // update editing widget contents
        aliasInput->setText(aliasEntry->text(0));
        replacementInput->setText(aliasEntry->text(1));
        // re-enable modified() signal on text changes in edit widgets
        m_newItemSelected = false;
    }
    // enable or disable editing widgets
    removeButton->setEnabled(enabled);
    aliasLabel->setEnabled(enabled);
    aliasInput->setEnabled(enabled);
    replacementLabel->setEnabled(enabled);
    replacementInput->setEnabled(enabled);
}

// what to do when the user change the name of a quick button
void Alias_Config::nameChanged(const QString& newName)
{
    // get possible first selected item
    Q3ListViewItem* item = aliasListView->selectedItem();

    // sanity check
    if (item)
    {
        // rename item
        item->setText(0,newName);
        // tell the config system that something has changed
        if (!m_newItemSelected) emit modified();
    }
}

// what to do when the user change the action definition of a quick button
void Alias_Config::actionChanged(const QString& newAction)
{
    // get possible first selected item
    Q3ListViewItem* item = aliasListView->selectedItem();

    // sanity check
    if (item)
    {
        // rename item
        item->setText(1,newAction);
        // tell the config system that something has changed
        if(!m_newItemSelected) emit modified();
    }
}

// add button pressed
void Alias_Config::addEntry()
{
    // add new item at the bottom of list view
    K3ListViewItem* newItem = new K3ListViewItem(aliasListView,aliasListView->lastChild(),i18n("New"),QString());
    // if successful ...
    if (newItem)
    {
        // select new item and make it the current one
        aliasListView->setSelected(newItem,true);
        aliasListView->setCurrentItem(newItem);
        // set input focus on item name edit
        aliasInput->setFocus();
        // select all text to make overwriting easier
        aliasInput->selectAll();
        // tell the config system that something has changed
        emit modified();
    }
}

// remove button pressed
void Alias_Config::removeEntry()
{
    // get possible first selected item
    Q3ListViewItem* item = aliasListView->selectedItem();

    // sanity check
    if (item)
    {
        // get item below the current one
        Q3ListViewItem* nextItem = item->itemBelow();
        // if there was none, get the one above
        if(!nextItem) nextItem = item->itemAbove();

        // remove the item from the list
        delete item;

        // check if we found the next item
        if (nextItem)
        {
            // select the item and make it the current ite,
            aliasListView->setSelected(nextItem,true);
            aliasListView->setCurrentItem(nextItem);
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

#include "alias_preferences.moc"
