/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dccrecipientdialog.cpp  -  lets the user choose a nick from the list
  begin:     Sam Dez 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>

#include <klocale.h>
#include <kdebug.h>

#include "dccrecipientdialog.h"

DccRecipientDialog::DccRecipientDialog(QWidget* parent,QStringList list,QSize size) :
                    KDialogBase(parent,"dcc_recipient_dialog",true,i18n("Select Recipient"),
                    KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,true)
{
  kdDebug() << "DccRecipientDialog::DccRecipientDialog()" << endl;

  /* Create the top level widget */
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  /* Add the layout to the widget */
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());
  /* Add the nickname list widget */
  KListBox* nicknameList=new KListBox(page,"recipient_list");

  nicknameList->insertStringList(list);
  nicknameList->sort(true);

  dialogLayout->addWidget(nicknameList);

  connect(nicknameList,SIGNAL (highlighted(QListBoxItem*)),this,SLOT (newNicknameSelected(QListBoxItem*)) );
  connect(nicknameList,SIGNAL (executed(QListBoxItem*)),this,SLOT (newNicknameSelectedQuit(QListBoxItem*)) );

  setButtonOKText(i18n("OK"),i18n("Select nickname and close the window"));
  setButtonCancelText(i18n("Cancel"),i18n("Close the window without changes"));

  setInitialSize(size);
  show();
}

DccRecipientDialog::~DccRecipientDialog()
{
  kdDebug() << "DccRecipientDialog::~DccRecipientDialog()" << endl;
}

QString DccRecipientDialog::getSelectedNickname()
{
  return selectedNickname;
}

void DccRecipientDialog::newNicknameSelected(QListBoxItem* item)
{
  selectedNickname=item->text();
}

void DccRecipientDialog::newNicknameSelectedQuit(QListBoxItem* item)
{
  selectedNickname=item->text();
  delayedDestruct();
}

void DccRecipientDialog::slotCancel()
{
  selectedNickname="";
  KDialogBase::slotCancel();
}

QString DccRecipientDialog::getNickname(QWidget* parent,QStringList list)
{
  kdDebug() << "DccRecipientDialog::getNickname()" << endl;

  QSize size; // TODO: get it from KonversationApplication::preferences
  DccRecipientDialog dlg(parent,list,size);
  dlg.exec();

  return dlg.getSelectedNickname();
}

#include "dccrecipientdialog.moc"
