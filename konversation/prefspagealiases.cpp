/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagealiases.cpp  -  The preferences GUI managing aliases
  begin:     Mon Jul 14 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qgrid.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <klistview.h>
#include <klineeditdlg.h>

#include "preferences.h"
#include "prefspagealiases.h"

PrefsPageAliases::PrefsPageAliases(QFrame* newParent,Preferences* newPreferences) :
                  PrefsPage(newParent,newPreferences)
{
  // Add the layout to the page
  QHBoxLayout* aliasesLayout=new QHBoxLayout(parentFrame,marginHint(),spacingHint());

  aliasesListView=new KListView(parentFrame);

  aliasesListView->addColumn(i18n("Alias"));
  aliasesListView->addColumn(i18n("Replacement"));

  aliasesListView->setAllColumnsShowFocus(true);
  aliasesListView->setItemsRenameable(true);
  aliasesListView->setRenameable(0,true);
  aliasesListView->setRenameable(1,true);
  aliasesListView->setFullWidth(true);
  aliasesListView->setSorting(-1,false);
  aliasesListView->setDragEnabled(true);
  aliasesListView->setAcceptDrops(true);

  QStringList aliasList(preferences->getAliasList());
  // Insert alias items backwards to get them sorted properly
  for(int index=aliasList.count();index!=0;index--)
  {
    QString item=aliasList[index-1];
    new KListViewItem(aliasesListView,item.section(' ',0,0),item.section(' ',1));
  }

  // Set up the buttons to the right of the list
  QGrid* buttonBox=new QGrid(3,QGrid::Vertical,parentFrame);
  buttonBox->setSpacing(spacingHint());
  QPushButton* newButton=new QPushButton(i18n("New..."),buttonBox);
  QPushButton* removeButton=new QPushButton(i18n("Remove"),buttonBox);

  aliasesLayout->addWidget(aliasesListView);
  aliasesLayout->addWidget(buttonBox);

  connect(newButton,SIGNAL (clicked()),this,SLOT (newAlias()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeAlias()) );
}


PrefsPageAliases::~PrefsPageAliases()
{
}

void PrefsPageAliases::newAlias()
{
  bool ok=false;
  QString newPattern=KLineEditDlg::getText(i18n("Add alias:"),i18n("New"),&ok,parentFrame);
  if(ok)
  {
    KListViewItem* newItem=new KListViewItem(aliasesListView,newPattern);
    aliasesListView->setSelected(newItem,true);
  }
}

void PrefsPageAliases::removeAlias()
{
  QListViewItem* selected=aliasesListView->selectedItem();
  if(selected)
  {
    if(selected->itemBelow()) aliasesListView->setSelected(selected->itemBelow(),true);
    else aliasesListView->setSelected(selected->itemAbove(),true);

    delete selected;
  }
}

void PrefsPageAliases::applyPreferences()
{
  QStringList newList;

  QListViewItem* item=aliasesListView->itemAtIndex(0);
  while(item)
  {
    newList.append(item->text(0)+" "+item->text(1));
    item=item->itemBelow();
  }

  preferences->setAliasList(newList);
}

#include "prefspagealiases.moc"
