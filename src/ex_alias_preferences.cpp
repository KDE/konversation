/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include <klocale.h>
#include <klistview.h>
#include <kinputdialog.h>
#include <kdebug.h>

#include "preferences.h"
#include "ex_alias_preferences.h"

Alias_Config_Ext::Alias_Config_Ext(QWidget* parent, const char* name) 
  : Alias_Config(parent, name)
{
  aliasesListView->setRenameable(0,true);
  aliasesListView->setRenameable(1,true);
  aliasesListView->setSorting(-1,false);
  
  QStringList aliasList(Preferences::aliasList());
  
  // Insert alias items backwards to get them sorted properly
  for(int index=aliasList.count();index!=0;index--)
    {
      QString item=aliasList[index-1];
      new KListViewItem(aliasesListView,item.section(' ',0,0),item.section(' ',1));
    }

  connect(newButton,SIGNAL (clicked()),this,SLOT (newAlias()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeAlias()) );
  connect(parent, SIGNAL(okClicked()), this, SLOT(saveAliases()));
    
}

Alias_Config_Ext::~Alias_Config_Ext()
{
}

void Alias_Config_Ext::newAlias()
{
  bool ok=false;
  QString newPattern=KInputDialog::getText(i18n("New Alias"),i18n("Add alias:"),i18n("New"),&ok,parentFrame);
  if(ok)
    {
      KListViewItem* newItem=new KListViewItem(aliasesListView,newPattern);
      aliasesListView->setSelected(newItem,true);
    }
}

void Alias_Config_Ext::removeAlias()
{
  QListViewItem* selected=aliasesListView->selectedItem();
  if(selected)
    {
      if(selected->itemBelow()) aliasesListView->setSelected(selected->itemBelow(),true);
      else aliasesListView->setSelected(selected->itemAbove(),true);
      
      delete selected;
    }
}

void Alias_Config_Ext::saveAliases()
{
  kdDebug() << "Alias_Config_Ext::saveAliases" << endl;
  QStringList newList;
  
  QListViewItem* item=aliasesListView->itemAtIndex(0);
  while(item)
    {
      newList.append(item->text(0)+" "+item->text(1));
      item=item->itemBelow();
    }
  
  Preferences::setAliasList(newList);
}

#include "ex_alias_preferences.moc"
