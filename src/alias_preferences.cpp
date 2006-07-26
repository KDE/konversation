/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#include <qpushbutton.h>

#include <klocale.h>
#include <klistview.h>
#include <kinputdialog.h>
#include <kdebug.h>

#include "preferences.h"
#include "alias_preferences.h"

Alias_Config::Alias_Config(QWidget* parent, const char* name)
  : Alias_ConfigUI(parent, name)
{
  aliasesListView->setRenameable(0,true);
  aliasesListView->setRenameable(1,true);
  aliasesListView->setSorting(-1,false);
  m_defaultAliasList = Preferences::defaultAliasList();
  loadSettings();
  connect(newButton,SIGNAL (clicked()),this,SLOT (newAlias()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeAlias()) );
  connect(aliasesListView,SIGNAL (moved()),this,SIGNAL (modified()) );
  connect(aliasesListView,SIGNAL (itemRenamed(QListViewItem*)),this,SLOT (itemRenamed(QListViewItem*)));
}

Alias_Config::~Alias_Config()
{
}

void Alias_Config::newAlias()
{
  bool ok=false;

  QString newPattern=KInputDialog::getText(i18n("New Alias"),i18n("Add alias:"),i18n("New"),&ok,this);
  if(ok)
    {
      KListViewItem* newItem=new KListViewItem(aliasesListView,newPattern);
      aliasesListView->setSelected(newItem,true);
    }
  emit modified();
}

void Alias_Config::removeAlias()
{
  QListViewItem* selected=aliasesListView->selectedItem();
  if(selected)
    {
      if(selected->itemBelow()) aliasesListView->setSelected(selected->itemBelow(),true);
      else aliasesListView->setSelected(selected->itemAbove(),true);

      delete selected;
    }
  emit modified();
}

void Alias_Config::restorePageToDefaults()
{
  setAliases(m_defaultAliasList);
}

void Alias_Config::saveSettings()
{
  QStringList newList=currentList();
  Preferences::setAliasList(newList);

  // saved list is now old list, to check for changes
  m_oldAliasList=newList;
}

void Alias_Config::loadSettings()
{
  setAliases(Preferences::aliasList());
}

void Alias_Config::setAliases(const QStringList &aliasList)
{
  aliasesListView->clear();
  // Insert alias items backwards to get them sorted properly
  for(int index=aliasList.count();index!=0;index--)
    {
      QString item=aliasList[index-1];
      new KListViewItem(aliasesListView,item.section(' ',0,0),item.section(' ',1));
    }
  // remember alias list
  m_oldAliasList=aliasList;
}

QStringList Alias_Config::currentList()
{
  QStringList newList;

  QListViewItem* item=aliasesListView->itemAtIndex(0);
  while(item)
    {
      newList.append(item->text(0)+' '+item->text(1));
      item=item->itemBelow();
    }
  return newList;
}

void Alias_Config::itemRenamed(QListViewItem* /* item */)
{
  // this is all we care about here
  emit modified();
}

bool Alias_Config::hasChanged()
{
  return(currentList()!=m_oldAliasList);
}

#include "alias_preferences.moc"

