/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  notifydialog.cpp  -  description
  begin:     Sam Jul 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qgrid.h>

#include <klocale.h>
#include <klistview.h>
#include <kdebug.h>

#include "notifydialog.h"

NotifyDialog::NotifyDialog(QStringList newNotifyList,QSize newSize,bool use,int delay):
               KDialogBase(0,"notifydialog",false,i18n("Edit Notify List"),
                           KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                           KDialogBase::Ok,true)
{
  kdDebug() << "NotifyDialog::NotifyDialog()" << endl;
  /* Create the top level widget */
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  /* Add the layout to the widget */
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());

  /* Set up notify delay widgets */
  QHBox* delayBox=new QHBox(page);
  delayBox->setSpacing(spacingHint());

  useNotifyCheck=new QCheckBox(i18n("Use notify"),delayBox,"use_notify_checkbox");
  useNotifyCheck->setChecked(use);
  new QLabel(i18n("Notify Interval:"),delayBox,"interval_label");
  notifyDelaySpin=new QSpinBox(5,1000,1,delayBox,"delay_spin");
  notifyDelaySpin->setValue(delay);
  notifyDelaySpin->setSuffix(i18n(" seconds"));

  /* Set up the ignore list */
  QHBox* listBox=new QHBox(page);
  listBox->setSpacing(spacingHint());
  notifyListView=new KListView(listBox);

  notifyListView->addColumn(i18n("Notify"));

  notifyListView->setAllColumnsShowFocus(true);
  notifyListView->setItemsRenameable(true);
  notifyListView->setRenameable(0,true);
  notifyListView->setFullWidth(true);
  notifyListView->setSorting(-1,false);
  notifyListView->setDragEnabled(true);
  notifyListView->setAcceptDrops(true);

  /* Set up the buttons to the right of the list */
  QGrid* buttonBox=new QGrid(3,QGrid::Vertical,listBox);
  buttonBox->setSpacing(spacingHint());
  newButton=new QPushButton(i18n("New"),buttonBox);
  removeButton=new QPushButton(i18n("Remove"),buttonBox);

  /* Set up instructions */
  QLabel* instructions=new QLabel(i18n("<b>Note:</b> If you switch off the notify function or remove all "
                                       "entries from the list the Lag-O-Meter will stop working."),page);
 
  dialogLayout->addWidget(delayBox);
  dialogLayout->addWidget(listBox);
  dialogLayout->addWidget(instructions);

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));

  connect(newButton,SIGNAL(clicked()),
                 this,SLOT(newNotify()));
  connect(removeButton,SIGNAL(clicked()),
                    this,SLOT(removeNotify()));

  /* Insert Notify items backwards to get them sorted properly */
  int index=newNotifyList.count()-1;

  while(index!=-1)
  {
    QString item=newNotifyList[index--];
/*    KListViewItem* newItem= */ new KListViewItem(notifyListView,item);
  }

  setInitialSize(newSize);
}

NotifyDialog::~NotifyDialog()
{
}

void NotifyDialog::newNotify()
{
  KListViewItem* newItem=new KListViewItem(notifyListView,"New");
  notifyListView->setSelected(newItem,true);
}

void NotifyDialog::removeNotify()
{
  delete notifyListView->selectedItem();
}

void NotifyDialog::slotOk()
{
  slotApply();
  slotCancel();
}

void NotifyDialog::slotApply()
{
  emit applyClicked(getNotifyList(),useNotifyCheck->isChecked(),notifyDelaySpin->value());
}

void NotifyDialog::slotCancel()
{
  emit cancelClicked(size());
}

QStringList NotifyDialog::getNotifyList()
{
  QStringList newList;

  QListViewItem* item=notifyListView->firstChild();
  while(item)
  {
    QString newItem=item->text(0);
    newList.append(newItem);
    item=item->itemBelow();
  }

  return newList;
}
