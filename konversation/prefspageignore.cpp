/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageignore.cpp  -  Provides an interface to the ignore list
  begin:     Fre Jun 13 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <klistview.h>

#include "prefspageignore.h"
#include "preferences.h"
#include "ignore.h"
#include "ignorecheckbox.h"
#include "ignorelistviewitem.h"

PrefsPageIgnore::PrefsPageIgnore(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add the layout to the widget
  QVBoxLayout* dialogLayout=new QVBoxLayout(parentFrame);
  dialogLayout->setSpacing(spacingHint());
  // Set up the ignore list
  QHBox* listBox=new QHBox(parentFrame);
  listBox->setSpacing(spacingHint());
  ignoreListView=new KListView(listBox);

  ignoreListView->addColumn(i18n("Ignore pattern"));
  ignoreListView->addColumn(i18n("Channels"));
  ignoreListView->addColumn(i18n("Queries"));
  ignoreListView->addColumn(i18n("Notices"));
  ignoreListView->addColumn(i18n("CTCPs"));
  ignoreListView->addColumn(i18n("DCCs"));
  ignoreListView->addColumn(i18n("Exception"));

  ignoreListView->setAllColumnsShowFocus(true);
  ignoreListView->setItemsRenameable(true);
  ignoreListView->setRenameable(0,true);
  ignoreListView->setSorting(-1,false);
  ignoreListView->setDragEnabled(true);
  ignoreListView->setAcceptDrops(true);

  // Set up the buttons to the right of the list
  QGrid* buttonBox=new QGrid(3,QGrid::Vertical,listBox);
  buttonBox->setSpacing(spacingHint());
  newButton=new QPushButton(i18n("New"),buttonBox);
  removeButton=new QPushButton(i18n("Remove"),buttonBox);

  // Set up the checkboxes
  QHBox* flagBox=new QHBox(parentFrame);
  flagBox->setSpacing(spacingHint());
  IgnoreCheckBox* c;
  // Store the checkbox pointers in a list
  c=new IgnoreCheckBox(i18n("Channels"),flagBox,Ignore::Channel);
  checkList.append(c);
  c=new IgnoreCheckBox(i18n("Queries"),flagBox,Ignore::Query);
  checkList.append(c);
  c=new IgnoreCheckBox(i18n("Notices"),flagBox,Ignore::Notice);
  checkList.append(c);
  c=new IgnoreCheckBox(i18n("CTCPs"),flagBox,Ignore::CTCP);
  checkList.append(c);
  c=new IgnoreCheckBox(i18n("DCCs"),flagBox,Ignore::DCC);
  checkList.append(c);
  c=new IgnoreCheckBox(i18n("Exception"),flagBox,Ignore::Exception);
  checkList.append(c);

  dialogLayout->addWidget(listBox);
  dialogLayout->addWidget(flagBox);

  connect(newButton,SIGNAL(clicked()),
                 this,SLOT(newIgnore()));
  connect(removeButton,SIGNAL(clicked()),
                    this,SLOT(removeIgnore()));
  connect(ignoreListView,SIGNAL(selectionChanged(QListViewItem*)),
                      this,SLOT(select(QListViewItem*)));

  // Connect the checkboxes's signals to the appropriate slot
  for(int index=0;index<6;index++)
    connect(checkList.at(index),SIGNAL(flagChanged(int,bool)),
                             this,SLOT(checked(int,bool)));

  QPtrList<Ignore> ignoreList=preferences->getIgnoreList();
  // Insert Ignore items backwards to get them sorted properly
  Ignore* item=ignoreList.last();
  while(item)
  {
    new IgnoreListViewItem(ignoreListView,item->getName(),item->getFlags());
    item=ignoreList.prev();
  }
}

PrefsPageIgnore::~PrefsPageIgnore()
{
}

void PrefsPageIgnore::newIgnore()
{
  new IgnoreListViewItem(ignoreListView,
                         "new!new@new.new",
                         Ignore::Channel |
                         Ignore::Query |
                         Ignore::Notice |
                         Ignore::CTCP |
                         Ignore::DCC);
}

void PrefsPageIgnore::removeIgnore()
{
  delete ignoreListView->selectedItem();
}

QPtrList<Ignore> PrefsPageIgnore::getIgnoreList()
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

void PrefsPageIgnore::checked(int flag,bool active)
{
  IgnoreListViewItem* i=static_cast<IgnoreListViewItem*>(ignoreListView->selectedItem());
  if(i) i->setFlag(flag,active);
}

void PrefsPageIgnore::select(QListViewItem* item)
{
  // FIXME: Cast to IgnoreListViewItem, maybe derive from KListView some day
  IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(item);

  if(item)
  {
    // Update Checkboxes
    int flagValue=1;
    for(int index=0;index<6;index++)
    {
      IgnoreCheckBox* check=checkList.at(index);
      check->setChecked(selectedItem->getFlag(flagValue));
      flagValue+=flagValue;
    }
  }
}

void PrefsPageIgnore::applyPreferences()
{
  preferences->setIgnoreList(getIgnoreList());
}

#include "prefspageignore.moc"
