/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nickchangedialog.cpp  -  description
  begin:     Mon Jul 8 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <iostream>

#include <qlayout.h>

#include <klocale.h>

#include "nickchangedialog.h"

NickChangeDialog::NickChangeDialog(QWidget* parent,QString currentNick,QStringList nickList,QSize size):
                  KDialogBase(0,"nickchangedialog",false,i18n("Change Nickname"),
                              KDialogBase::Ok | KDialogBase::Cancel,
                              KDialogBase::Ok,true)
{
  cerr << "NickChangeDialog::NickChangeDialog()" << endl;

  /* Create the top level widget */
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  /* Add the layout to the widget */
  QHBoxLayout* dialogLayout=new QHBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());
  /* Add the nickname input widget */
  nicknameInput=new QComboBox(true,page,"nicknameinput");
  
  nicknameInput->insertStringList(nickList);
  nicknameInput->insertItem(currentNick,0);

  dialogLayout->addWidget(nicknameInput);

  connect(nicknameInput,SIGNAL (activated(QString)),this,SLOT (newNicknameEntered(QString)) );
  
  setButtonOKText(i18n("OK"),i18n("Set new nickname and close the window"));
  setButtonCancelText(i18n("Cancel"),i18n("Close the window without changes"));

  setInitialSize(size);
}

NickChangeDialog::~NickChangeDialog()
{
  cerr << "NickChangeDialog::~NickChangeDialog()" << endl;
}

void NickChangeDialog::slotOk()
{
  emit newNickname(nicknameInput->currentText());
  emit closeDialog(size());
}

void NickChangeDialog::slotCancel()
{
  emit closeDialog(size());
}

void NickChangeDialog::newNicknameEntered(const QString& newNick)
{
  emit newNickname(newNick);
  emit closeDialog(size());
}
