/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ignoredialog.cpp  -  description
  begin:     Mon Jun 24 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include <klocale.h>
#include <klistview.h>
#include <kdebug.h>

#include "ignoredialog.h"
#include "ignore.h"
#include "ignorecheckbox.h"
#include "ignorelistviewitem.h"

IgnoreDialog::IgnoreDialog(QPtrList<Ignore> newIgnoreList,QSize newSize):
               KDialogBase(static_cast<QWidget*>(0),"ignoredialog",false,i18n("Edit ignore list"),
                           KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                           KDialogBase::Ok,true)
{
  kdDebug() << "IgnoreDialog::IgnoreDialog()" << endl;
  /* Create the top level widget */
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  /* Add the layout to the widget */
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());
  /* Set up the ignore list */
  QHBox* listBox=new QHBox(page);
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

  /* Set up the buttons to the right of the list */
  QGrid* buttonBox=new QGrid(3,QGrid::Vertical,listBox);
  buttonBox->setSpacing(spacingHint());
  newButton=new QPushButton(i18n("New"),buttonBox);
  removeButton=new QPushButton(i18n("Remove"),buttonBox);

  /* Set up the checkboxes */
  QHBox* flagBox=new QHBox(page);
  flagBox->setSpacing(spacingHint());
  IgnoreCheckBox* c;
  /* Store the checkbox pointers in a list */
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

//  QLabel* note=new QLabel(i18n("<qt>Please note that the ignore function is not yet operational!</qt>"),page);

  dialogLayout->addWidget(listBox);
  dialogLayout->addWidget(flagBox);
//  dialogLayout->addWidget(note);

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));

  connect(newButton,SIGNAL(clicked()),
                 this,SLOT(newIgnore()));
  connect(removeButton,SIGNAL(clicked()),
                    this,SLOT(removeIgnore()));
  connect(ignoreListView,SIGNAL(selectionChanged(QListViewItem*)),
                      this,SLOT(select(QListViewItem*)));

  /* Connect the checkboxes's signals to the appropriate slot */
  for(int index=0;index<6;index++)
    connect(checkList.at(index),SIGNAL(flagChanged(int,bool)),
                             this,SLOT(checked(int,bool)));

  /* Insert Ignore items backwards to get them sorted properly */
  Ignore* item=newIgnoreList.last();
  while(item)
  {
/*    IgnoreListViewItem* newItem= */ new IgnoreListViewItem(ignoreListView,item->getName(),item->getFlags());
    item=newIgnoreList.prev();
  }

  setInitialSize(newSize);
}

IgnoreDialog::~IgnoreDialog()
{
}

void IgnoreDialog::newIgnore()
{
/*  IgnoreListViewItem* newItem= */ new IgnoreListViewItem(ignoreListView,"new!new@new.new",
                                                     Ignore::Channel |
                                                     Ignore::Query |
                                                     Ignore::Notice |
                                                     Ignore::CTCP |
                                                     Ignore::DCC);
}

void IgnoreDialog::removeIgnore()
{
  delete ignoreListView->selectedItem();
}

void IgnoreDialog::slotOk()
{
  slotApply();
  slotCancel();
}

void IgnoreDialog::slotApply()
{
  emit applyClicked(getIgnoreList());
}

void IgnoreDialog::slotCancel()
{
  emit cancelClicked(size());
}

QPtrList<Ignore> IgnoreDialog::getIgnoreList()
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

void IgnoreDialog::checked(int flag,bool active)
{
  IgnoreListViewItem* i=static_cast<IgnoreListViewItem*>(ignoreListView->selectedItem());
  if(i) i->setFlag(flag,active);
}

void IgnoreDialog::select(QListViewItem* item)
{
  /* FIXME: Cast to IgnoreListViewItem, maybe derive from KListView some day */
  IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(item);

  if(item)
  {
    /* Update Checkboxes */
    int flagValue=1;
    for(int index=0;index<6;index++)
    {
      IgnoreCheckBox* check=checkList.at(index);
      check->setChecked(selectedItem->getFlag(flagValue));
      flagValue+=flagValue;
    }
  }
}

#include "ignoredialog.moc"
