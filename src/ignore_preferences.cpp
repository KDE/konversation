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

#include <klocale.h>
#include <klistview.h>
#include <qlistview.h>
#include <qlineedit.h>
#include "ignore_preferences.h"
#include "ignorelistviewitem.h"
#include "ignore.h"
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <qcheckbox.h>
#include "preferences.h"

Ignore_Config::Ignore_Config( QWidget* parent, const char* name, WFlags fl )
    : Ignore_ConfigUI( parent, name, fl )
{
    connect(newButton,SIGNAL(clicked()),
        this,SLOT(newIgnore()));
    connect(removeButton,SIGNAL(clicked()),
        this,SLOT(removeIgnore()));
    connect(removeAllButton,SIGNAL(clicked()),
	this,SLOT(removeAllIgnore()));
    connect(ignoreListView,SIGNAL(selectionChanged(QListViewItem*)),
        this,SLOT(select(QListViewItem*)));
    connect(chkChannel, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkQuery, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkNotice, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkCTCP, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkDCC, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(txtPattern, SIGNAL(textChanged(const QString &)), this, SLOT(flagCheckboxChanged()));
//    connect(chkException, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    loadSettings();
}

Ignore_Config::~Ignore_Config()
{

}

void Ignore_Config::newIgnore()
{
     ignoreListView->setSelected(new IgnoreListViewItem(ignoreListView,
        "new!new@new.new",
        Ignore::Channel |
        Ignore::Query |
        Ignore::Notice |
        Ignore::CTCP |
        Ignore::DCC), true);
    txtPattern->setFocus();
    txtPattern->selectAll();
     
    updateEnabledness();
    emit modified();
}
void Ignore_Config::removeAllIgnore()
{
    ignoreListView->clear();
    updateEnabledness();
    emit modified();
}
void Ignore_Config::removeIgnore()
{
    delete ignoreListView->selectedItem();
    updateEnabledness();
    emit modified();
}

QPtrList<Ignore> Ignore_Config::getIgnoreList()
{
    QPtrList<Ignore> newList;

    IgnoreListViewItem* item=static_cast<IgnoreListViewItem*>(ignoreListView->firstChild());
    while(item)
    {
        Ignore* newItem=new Ignore(item->text(0),item->getFlags());
        newList.append(newItem);
        item=item->itemBelow();
    }

    return newList;
}

// returns the currently visible ignore list as QStringList to make comparing easy
QStringList Ignore_Config::currentIgnoreList()
{
    QStringList newList;

    IgnoreListViewItem* item=static_cast<IgnoreListViewItem*>(ignoreListView->firstChild());
    while(item)
    {
        newList.append(item->text(0)+' '+item->getFlags());
        item=item->itemBelow();
    }

    return newList;
}

// checks if the currently visible ignore list differs from the currently saved one
bool Ignore_Config::hasChanged()
{
  return(m_oldIgnoreList!=currentIgnoreList());
}

void Ignore_Config::restorePageToDefaults()
{
    if(ignoreListView->childCount() != 0) {
      ignoreListView->clear();
      updateEnabledness();
      emit modified();
    }
}
void Ignore_Config::saveSettings()
{
    Preferences::setIgnoreList(getIgnoreList());
    // remember the list for hasChanged()
    m_oldIgnoreList=currentIgnoreList();
}

void Ignore_Config::loadSettings()
{
    QPtrList<Ignore> ignoreList=Preferences::ignoreList();
    // Insert Ignore items backwards to get them sorted properly
    Ignore* item=ignoreList.last();
    ignoreListView->clear();
    while(item)
    {
        new IgnoreListViewItem(ignoreListView,item->getName(),item->getFlags());
        item=ignoreList.prev();
    }
    // remember the list for hasChanged()
    m_oldIgnoreList=currentIgnoreList();
    updateEnabledness();
}

void Ignore_Config::updateEnabledness()
{
    IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(ignoreListView->selectedItem());

    chkChannel->setEnabled(selectedItem != NULL);
    chkQuery->setEnabled(selectedItem != NULL);
    chkNotice->setEnabled(selectedItem != NULL);
    chkCTCP->setEnabled(selectedItem != NULL);
    chkDCC->setEnabled(selectedItem != NULL);
//	chkExceptions->setEnabled(selectedItem != NULL);
    txtPattern->setEnabled(selectedItem != NULL);
    removeButton->setEnabled(selectedItem != NULL);
    removeAllButton->setEnabled(ignoreListView->childCount() > 0);

}

void Ignore_Config::select(QListViewItem* item)
{
    updateEnabledness();
    // FIXME: Cast to IgnoreListViewItem, maybe derive from KListView some day
    IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(item);

    if(selectedItem)
    {
	int flags = selectedItem->getFlags();
        chkChannel->setChecked(flags & Ignore::Channel);
        chkQuery->setChecked(flags & Ignore::Query);
        chkNotice->setChecked(flags & Ignore::Notice);
        chkCTCP->setChecked(flags & Ignore::CTCP);
        chkDCC->setChecked(flags & Ignore::DCC);
	txtPattern->blockSignals(true);
	txtPattern->setText(selectedItem->getName());
	txtPattern->blockSignals(false);

//        chkExceptions->setChecked(flags & Ignore::Exception) ;
    }
}

void Ignore_Config::flagCheckboxChanged()
{
    int flags = 0;
    if(chkChannel->isChecked()) flags |= Ignore::Channel;
    if(chkQuery->isChecked()) flags |= Ignore::Query;
    if(chkNotice->isChecked()) flags |= Ignore::Notice;
    if(chkCTCP->isChecked()) flags |= Ignore::CTCP;
    if(chkDCC->isChecked()) flags |= Ignore::DCC;
    
//    if(chkExceptions->isChecked()) flags |= Ignore::Exceptions;
    IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(ignoreListView->selectedItem());
    if(selectedItem) {
        selectedItem->setFlags(flags);
	selectedItem->setName(txtPattern->text());
    }
    emit modified();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Ignore_Config::languageChange()
{
  loadSettings();
}

#include "ignore_preferences.moc"
