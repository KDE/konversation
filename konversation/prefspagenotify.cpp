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
#include <kinputdialog.h>
#include <klineedit.h>

#include "prefspagenotify.h"
#include "preferences.h"

PrefsPageNotify::PrefsPageNotify(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add the layout to the page
  QVBoxLayout* notifyLayout = new QVBoxLayout(parentFrame,marginHint(),spacingHint());

  // Set up notify delay widgets
  QHBox* delayBox = new QHBox(parentFrame);
  delayBox->setSpacing(spacingHint());

  useNotifyCheck = new QCheckBox(i18n("&Use nickname watcher"),delayBox,"use_nick_watcher_checkbox");
  useNotifyCheck->setChecked(preferences->getUseNotify());
  notifyDelayLabel = new QLabel(i18n("Check &interval:"),delayBox,"interval_label");
  notifyDelayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  notifyDelaySpin = new QSpinBox(5,1000,1,delayBox,"delay_spin");
  notifyDelaySpin->setValue(preferences->getNotifyDelay());
  notifyDelaySpin->setSuffix(i18n(" seconds"));
  notifyDelayLabel->setBuddy(notifyDelaySpin);

  // Set up the notify list
  QHBox* listBox=new QHBox(parentFrame);
  listBox->setSpacing(spacingHint());
  notifyListView=new KListView(listBox);

  notifyListView->addColumn(i18n("Watched Nicknames"));

  notifyListView->setAllColumnsShowFocus(true);
  notifyListView->setItemsRenameable(true);
  notifyListView->setRenameable(0,true);
  notifyListView->setFullWidth(true);
  notifyListView->setSorting(-1,false);
  notifyListView->setDragEnabled(true);
  notifyListView->setAcceptDrops(true);

  // Set up the buttons to the right of the list
  QGrid* buttonBox = new QGrid(3, QGrid::Vertical, listBox);
  buttonBox->setSpacing(spacingHint());
  newButton = new QPushButton(i18n("&New..."),buttonBox);
  removeButton = new QPushButton(i18n("&Remove"),buttonBox);

  QHBox* actionEditBox=new QHBox(parentFrame);
  actionEditBox->setSpacing(spacingHint());
  actionEditBox->setEnabled(useNotifyCheck->isChecked());

  QLabel* notifyActionLabel = new QLabel(i18n("Command &executed when nickname is double clicked:"), actionEditBox);
  notifyActionInput = new KLineEdit(preferences->getNotifyDoubleClickAction(), actionEditBox);
  notifyActionLabel->setBuddy(notifyActionInput);

  notifyLayout->addWidget(delayBox);
  notifyLayout->addWidget(listBox);
  notifyLayout->addWidget(actionEditBox);

  connect(useNotifyCheck, SIGNAL(toggled(bool)), this, SLOT(notifyCheckChanged(bool)));
  connect(useNotifyCheck, SIGNAL(toggled(bool)), actionEditBox, SLOT(setEnabled(bool)));
  connect(newButton, SIGNAL(clicked()), this, SLOT(newNotify()) );
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeNotify()) );

  QStringList notifyList(preferences->getNotifyList());

  // Insert Notify items backwards to get them sorted properly
  for(int index = notifyList.count(); index != 0; index--)
  {
    QString item = notifyList[index - 1];
    new KListViewItem(notifyListView, item);
  }

  notifyCheckChanged(useNotifyCheck->isChecked());
}

PrefsPageNotify::~PrefsPageNotify()
{
}

void PrefsPageNotify::newNotify()
{
  bool ok = false;
  QString newPattern = KInputDialog::getText(i18n("Nick Watch Dialog"),
    i18n("Add nick to watch for:"), i18n("New"), &ok, parentFrame);

  if(ok)
  {
    KListViewItem* newItem = new KListViewItem(notifyListView, newPattern);
    notifyListView->setSelected(newItem, true);
  }
}

void PrefsPageNotify::removeNotify()
{
  QListViewItem* selected = notifyListView->selectedItem();

  if(selected)
  {
    if(selected->itemBelow()) {
      notifyListView->setSelected(selected->itemBelow(),true);
    } else {
      notifyListView->setSelected(selected->itemAbove(),true);
    }

    delete selected;
  }
}

QStringList PrefsPageNotify::getNotifyList()
{
  QStringList newList;

  QListViewItem* item = notifyListView->firstChild();
  while(item)
  {
    QString newItem = item->text(0);
    newList.append(newItem);
    item=item->itemBelow();
  }

  return newList;
}

void PrefsPageNotify::notifyCheckChanged(bool enable)
{
  notifyDelayLabel->setEnabled(enable);
  notifyDelaySpin->setEnabled(enable);
  notifyListView->setEnabled(enable);
  newButton->setEnabled(enable);
  removeButton->setEnabled(enable);
}

void PrefsPageNotify::applyPreferences()
{
  preferences->setUseNotify(useNotifyCheck->isChecked());
  preferences->setNotifyList(getNotifyList());
  preferences->setNotifyDelay(notifyDelaySpin->value());
  preferences->setNotifyDoubleClickAction(notifyActionInput->text());
}

#include "prefspagenotify.moc"
