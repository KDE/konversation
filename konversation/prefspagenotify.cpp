/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagenotify.cpp  -  Proivides an interface to the notify list
  begin:     Fre Jun 13 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <klistview.h>
#include <klineeditdlg.h>

#include "prefspagenotify.h"
#include "preferences.h"

PrefsPageNotify::PrefsPageNotify(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add the layout to the page
  QVBoxLayout* notifyLayout=new QVBoxLayout(parentFrame,marginHint(),spacingHint());

  // Set up notify delay widgets
  QHBox* delayBox=new QHBox(parentFrame);
  delayBox->setSpacing(spacingHint());

  useNotifyCheck=new QCheckBox(i18n("Use notify"),delayBox,"use_notify_checkbox");
  notifyDelayLabel=new QLabel(i18n("Notify interval:"),delayBox,"interval_label");
  notifyDelayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  notifyDelaySpin=new QSpinBox(5,1000,1,delayBox,"delay_spin");
  notifyDelaySpin->setValue(preferences->getNotifyDelay());
  notifyDelaySpin->setSuffix(i18n(" seconds"));

  // Set up the notify list
  QHBox* listBox=new QHBox(parentFrame);
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

  // Set up the buttons to the right of the list
  QGrid* buttonBox=new QGrid(3,QGrid::Vertical,listBox);
  buttonBox->setSpacing(spacingHint());
  newButton=new QPushButton(i18n("New"),buttonBox);
  removeButton=new QPushButton(i18n("Remove"),buttonBox);

  notifyLayout->addWidget(delayBox);
  notifyLayout->addWidget(listBox);

  connect(useNotifyCheck,SIGNAL (stateChanged(int)),this,SLOT (notifyCheckChanged(int)));
  connect(newButton,SIGNAL (clicked()),this,SLOT (newNotify()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeNotify()) );

  QStringList notifyList(preferences->getNotifyList());
  // Insert Notify items backwards to get them sorted properly
  for(int index=notifyList.count();index!=0;index--)
  {
    QString item=notifyList[index-1];
    new KListViewItem(notifyListView,item);
  }

  bool use=preferences->getUseNotify();
  notifyCheckChanged(use ? 2 : 0);
  useNotifyCheck->setChecked(use);
}

PrefsPageNotify::~PrefsPageNotify()
{
}

void PrefsPageNotify::newNotify()
{
  bool ok=false;
  QString newPattern=KLineEditDlg::getText(i18n("Add notify"),i18n("New"),&ok,parentFrame);
  if(ok)
  {
    KListViewItem* newItem=new KListViewItem(notifyListView,newPattern);
    notifyListView->setSelected(newItem,true);
  }
}

void PrefsPageNotify::removeNotify()
{
  QListViewItem* selected=notifyListView->selectedItem();
  if(selected)
  {
    if(selected->itemBelow()) notifyListView->setSelected(selected->itemBelow(),true);
    else notifyListView->setSelected(selected->itemAbove(),true);

    delete selected;
  }
}

QStringList PrefsPageNotify::getNotifyList()
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

void PrefsPageNotify::notifyCheckChanged(int state)
{
  bool enable=(state==2);
  notifyDelayLabel->setEnabled(enable);
  notifyDelaySpin->setEnabled(enable);
  notifyListView->setEnabled(enable);
  newButton->setEnabled(enable);
  removeButton->setEnabled(enable);
}

// TODO: This should be done in all preferences pages I think
void PrefsPageNotify::applyPreferences()
{
  preferences->setUseNotify(useNotifyCheck->state()==2);
  preferences->setNotifyList(getNotifyList());
  preferences->setNotifyDelay(notifyDelaySpin->value());
}

#include "prefspagenotify.moc"
