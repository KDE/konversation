/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  editnotifydialog.cpp  -  description
  begin:     Wed Sep 1 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

// Qt includes.
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>

// KDE includes.
#include <klineedit.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>

// Konversation includes.
#include "editnotifydialog.h"
#include "konversationapplication.h"
#include "serverentry.h"

EditNotifyDialog::EditNotifyDialog(QWidget* parent,
                                   QString group,
                                   QString nickname):

                  KDialogBase(parent,"editnotify",true,i18n("Edit Watched Nickname"),
                              KDialogBase::Ok | KDialogBase::Cancel,
                              KDialogBase::Ok,true)

{
  QWidget* page=new QWidget(this);
  setMainWidget(page);

  QHBoxLayout* layout = new QHBoxLayout(page);
  layout->setSpacing(spacingHint());

  QLabel* groupNameLabel=new QLabel(i18n("&Group name:"),page);
  QString groupNameWT = i18n(
    "Pick the server group you will connect to here.");
  QWhatsThis::add(groupNameLabel, groupNameWT);
  m_groupNameCombo=new KComboBox(page,"notify_group_combo");
  QWhatsThis::add(m_groupNameCombo, groupNameWT);
  groupNameLabel->setBuddy(m_groupNameCombo);
  
  QLabel* nicknameLabel=new QLabel(i18n("&Nickname:"),page);
  QString nicknameWT = i18n(
      "<qt>The nickname to watch for when connected to a server in the group.</qt>");
  QWhatsThis::add(nicknameLabel, nicknameWT);
  m_nicknameInput = new KLineEdit(nickname, page);
  QWhatsThis::add(m_nicknameInput, nicknameWT);
  nicknameLabel->setBuddy(m_nicknameInput);

  // Build a list of unique server group names.
  QPtrList<ServerEntry> serverEntries=KonversationApplication::preferences.getServerList();
  QStringList groupNames;
  for(unsigned int index=0;index<serverEntries.count();index++)
  {
    QString name=serverEntries.at(index)->getGroupName();
    if (!groupNames.contains(name)) groupNames.append(name);
  }
  groupNames.sort();
  // Add group names to group combobox and select the one corresponding to argument.
  for (QStringList::Iterator it = groupNames.begin(); it != groupNames.end(); ++it)
  {
    m_groupNameCombo->insertItem(*it);
    if(*it == group) m_groupNameCombo->setCurrentItem(m_groupNameCombo->count()-1);
  }

  layout->addWidget(groupNameLabel);
  layout->addWidget(m_groupNameCombo);
  layout->addWidget(nicknameLabel);
  layout->addWidget(m_nicknameInput);

  setButtonOK(KGuiItem(i18n("&OK"),"button_ok",i18n("Change notify information")));
  setButtonCancel(KGuiItem(i18n("&Cancel"),"button_cancel",i18n("Discards all changes made")));
}

EditNotifyDialog::~EditNotifyDialog()
{
}

void EditNotifyDialog::slotOk()
{
  emit notifyChanged(m_groupNameCombo->currentText(),
                     m_nicknameInput->text());
  delayedDestruct();
}

#include "editnotifydialog.moc"
