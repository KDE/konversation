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
#include <qmap.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <klistview.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <kdebug.h>

#include "prefspagenotify.h"
#include "preferences.h"
#include "editnotifydialog.h"

PrefsPageNotify::PrefsPageNotify(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add the layout to the page
  QVBoxLayout* notifyLayout = new QVBoxLayout(parentFrame,marginHint(),spacingHint());

  // Set up notify delay widgets
  QHBox* delayBox = new QHBox(parentFrame);
  delayBox->setSpacing(spacingHint());

  m_useNotifyCheck = new QCheckBox(i18n("&Use nickname watcher"),delayBox,"use_nick_watcher_checkbox");
  QString useNotifyCheckWT = i18n(
      "<p>When the nickname watcher is turned on, you will be notified when the "
      "nicknames appearing in the <b>Watched Networkss/Nicknames</b> list come "
      "online or go offline.</p>"
      "<p>You can also open the <b>Nicks Online</b> window to see the status of all the "
      "watched nicknames.</p>");
  QWhatsThis::add(m_useNotifyCheck, useNotifyCheckWT);
  m_useNotifyCheck->setChecked(preferences->getUseNotify());
  m_notifyDelayLabel = new QLabel(i18n("Check &interval:"),delayBox,"interval_label");
  QString notifyDelayWT = i18n(
      "Konversation will check the status of the nicknames listed below at this interval.");
  QWhatsThis::add(m_notifyDelayLabel, notifyDelayWT);
  m_notifyDelayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_notifyDelaySpin = new QSpinBox(5,1000,1,delayBox,"delay_spin");
  QWhatsThis::add(m_notifyDelaySpin, notifyDelayWT);
  m_notifyDelaySpin->setValue(preferences->getNotifyDelay());
  m_notifyDelaySpin->setSuffix(i18n(" seconds"));
  m_notifyDelayLabel->setBuddy(m_notifyDelaySpin);
  
  m_showWatchedNicksAtStartup = new QCheckBox(i18n("&Show watched nicks online on startup"), parentFrame);
  QString showWatchedNicksAtStartupWT = i18n(
      "When checked, the <b>Nicks Online</b> window will be automatically opened when "
      "starting Konversation.");
  QWhatsThis::add(m_showWatchedNicksAtStartup, showWatchedNicksAtStartupWT);
  m_showWatchedNicksAtStartup->setChecked(preferences->getOpenWatchedNicksAtStartup());

  // Set up the notify list
  QHBox* listBox=new QHBox(parentFrame);
  listBox->setSpacing(spacingHint());
  m_notifyListView=new KListView(listBox);
  QWhatsThis::add(m_notifyListView, useNotifyCheckWT);

  m_notifyListView->addColumn(i18n("Watched Networks/Nicknames"));

  m_notifyListView->setAllColumnsShowFocus(true);
  m_notifyListView->setItemsRenameable(true);
  m_notifyListView->setRenameable(0,true);
  m_notifyListView->setFullWidth(true);
  m_notifyListView->setSorting(-1,false);
  m_notifyListView->setRootIsDecorated(true);
//  m_notifyListView->setDragEnabled(true);
//  m_notifyListView->setAcceptDrops(true);

  // Set up the buttons to the right of the list
  QGrid* buttonBox = new QGrid(3, QGrid::Vertical, listBox);
  buttonBox->setSpacing(spacingHint());
  m_newButton = new QPushButton(i18n("&New..."),buttonBox);
  QString newButtonWT = i18n(
      "Click to add a nickname to the list.");
  QWhatsThis::add(m_newButton, newButtonWT);
  m_removeButton = new QPushButton(i18n("&Remove"),buttonBox);
  QString removeButtonTW = i18n(
      "Click to remove the selected nickname from the list.");
  QWhatsThis::add(m_removeButton, removeButtonTW);

  QHBox* actionEditBox=new QHBox(parentFrame);
  actionEditBox->setSpacing(spacingHint());
  actionEditBox->setEnabled(m_useNotifyCheck->isChecked());

  QLabel* notifyActionLabel = new QLabel(i18n("Command &executed when nickname is double clicked:"), actionEditBox);
  m_notifyActionInput = new KLineEdit(preferences->getNotifyDoubleClickAction(), actionEditBox);
  notifyActionLabel->setBuddy(m_notifyActionInput);
  QString notifyActionWT = i18n(
      "<p>When you double click a nickname in the <b>Nicks Online</b> window, this "
      "command is placed in the <b>Input Line</b> on the server window.</p>"
      "<p>The following symbols can be used in the command:</p><ul>"
      "<li>%u: The nickname double clicked.</li>"
      "<li>%K: Server password.</li>"
      "<li>%n: Send command directly to the server instead of your input line.</li>"
      "</ul>");
  QWhatsThis::add(notifyActionLabel, notifyActionWT);
  QWhatsThis::add(m_notifyActionInput, notifyActionWT);
  QWhatsThis::add(notifyActionLabel, notifyActionWT);

  notifyLayout->addWidget(delayBox);
  notifyLayout->addWidget(m_showWatchedNicksAtStartup);
  notifyLayout->addWidget(listBox);
  notifyLayout->addWidget(actionEditBox);

  connect(m_useNotifyCheck, SIGNAL(toggled(bool)), this, SLOT(notifyCheckChanged(bool)));
  connect(m_useNotifyCheck, SIGNAL(toggled(bool)), actionEditBox, SLOT(setEnabled(bool)));
  connect(m_newButton, SIGNAL(clicked()), this, SLOT(newNotify()) );
  connect(m_removeButton, SIGNAL(clicked()), this, SLOT(removeNotify()) );

  QMap<QString, QStringList> notifyList(preferences->getNotifyList());

  // Insert backwards to get them sorted properly.
  QStringList networkNameList = notifyList.keys();
  for(int networkIndex = networkNameList.count(); networkIndex != 0; networkIndex--)
  {
    QString networkName = networkNameList[networkIndex - 1];
    QStringList nicknameList = notifyList[networkName];
    QListViewItem* networkNameItem = new KListViewItem(m_notifyListView, networkName);
    for(int index = nicknameList.count(); index != 0; index--)
    {
      QString nickname = nicknameList[index - 1];
      KListViewItem* nicknameItem = new KListViewItem(networkNameItem, nickname);
      // Nicknames are selectable, editable, and removable.
      nicknameItem->setSelectable(true);
    }
    // Network names are not selectable, not editable, and not removable.
    networkNameItem->setSelectable(false);
    networkNameItem->setOpen(true);
  }
  connect(m_notifyListView, SIGNAL(selectionChanged()),
    this, SLOT(slotNotifyListView_SelectionChanged()));

  notifyCheckChanged(m_useNotifyCheck->isChecked());
}

PrefsPageNotify::~PrefsPageNotify()
{
}

void PrefsPageNotify::newNotify()
{
  QString networkName;
  QListViewItem* item = m_notifyListView->currentItem();
  if (item)
  {
    if (item->parent()) item = item->parent();
    if (item) networkName = item->text(0);
  } 
  EditNotifyDialog editNotifyDialog(parentFrame, networkName, QString::null);

  connect(&editNotifyDialog,SIGNAL (notifyChanged(const QString&,
                                                  const QString&)),
                          this,SLOT (createNotify(const QString&,
                                                  const QString&)));
  editNotifyDialog.exec();
}

void PrefsPageNotify::createNotify(const QString& networkName, const QString& nickname)
{
  if (networkName.isEmpty() || nickname.isEmpty()) return;
  QListViewItem* networkNameItem = findBranch(networkName, true);
  QListViewItem* nicknameItem = findItemChild(networkNameItem, nickname);
  if (!nicknameItem)
  {
    KListViewItem* newItem = new KListViewItem(networkNameItem, nickname);
    newItem->setSelectable(true);
    m_notifyListView->setSelected(newItem, true);
  }
  m_removeButton->setEnabled(true);
}

void PrefsPageNotify::removeNotify()
{
  QListViewItem* selected = m_notifyListView->selectedItem();

  if(selected)
  {
    // Cannot delete network (except by deleting all nicks in the network).
    if (!selected->parent()) return;
    QListViewItem* networkNameItem = selected->parent();
    // Select next nickname, or if no such nickname, previous nickname, but skip
    // over network names.
    QListViewItem* item = selected->itemBelow();
    if (item)
    {
      if (!item->parent()) item = item->itemBelow();
    }
    else
    {
      item = selected->itemAbove();
      if (item)
      {
        if (!item->parent()) item = item->itemAbove();
      }
    }
    if (item)
      m_notifyListView->setSelected(item, true);

    delete selected;
    // If all nicknames for a network deleted, delete the network.
    item = networkNameItem->firstChild();
    if (!item) delete networkNameItem;
  }
}

QMap<QString, QStringList> PrefsPageNotify::getNotifyList()
{
  QMap<QString, QStringList> notifyList;
  
  QListViewItem* networkItem = m_notifyListView->firstChild();
  while (networkItem)
  {
    QStringList nicknameList;
    QListViewItem* nicknameItem = networkItem->firstChild();
    while (nicknameItem)
    {
        nicknameList.append(nicknameItem->text(0));
        nicknameItem = nicknameItem->nextSibling();
    }
    notifyList[networkItem->text(0)] = nicknameList;
    networkItem = networkItem->nextSibling();
  }

  return notifyList;
}

void PrefsPageNotify::notifyCheckChanged(bool enable)
{
  m_notifyDelayLabel->setEnabled(enable);
  m_notifyDelaySpin->setEnabled(enable);
  m_notifyListView->setEnabled(enable);
  m_newButton->setEnabled(enable);
  m_showWatchedNicksAtStartup->setEnabled(enable);
  QListViewItem* item = m_notifyListView->selectedItem();
  if (enable && item)
    m_removeButton->setEnabled(item->parent());
  else
    m_removeButton->setEnabled(false);
}

void PrefsPageNotify::applyPreferences()
{
  preferences->setUseNotify(m_useNotifyCheck->isChecked());
  preferences->setNotifyList(getNotifyList());
  preferences->setNotifyDelay(m_notifyDelaySpin->value());
  preferences->setNotifyDoubleClickAction(m_notifyActionInput->text());
  preferences->setOpenWatchedNicksAtStartup(m_showWatchedNicksAtStartup->isChecked());
}

QListViewItem* PrefsPageNotify::findBranch(QString name,bool generate)
{
  QListViewItem* branch=m_notifyListView->findItem(name,0);
  if(branch==0 && generate==true)
  {
    branch=new QListViewItem(m_notifyListView,name);
    branch->setOpen(true);
    branch->setSelectable(false);
  }

  return branch;
}

/**
* Returns the named child of parent item in a KListView.
* @param parent            Pointer to a QListViewItem.
* @param name              The name of the desired child QListViewItem.  Name
*                          is assumed to be in column 0 of the item.
* @return                  Pointer to the child QListViewItem or 0 if not found.
*/
QListViewItem* PrefsPageNotify::findItemChild(const QListViewItem* parent, const QString& name)
{
  if (!parent) return 0;
  QListViewItem* child;
  for (child = parent->firstChild(); (child) ; child = child->nextSibling())
  {
    if (child->text(0) == name) return child;
  }
  return 0;
}

/**
* Received when user selects a different item in the notifylistview.
*/
void PrefsPageNotify::slotNotifyListView_SelectionChanged()
{
  // Disable remove button if nickname not currently selected.
  QListViewItem* item = m_notifyListView->selectedItem();
  if (item)
    m_removeButton->setEnabled(item->parent());
  else
    m_removeButton->setEnabled(false);
}

#include "prefspagenotify.moc"
