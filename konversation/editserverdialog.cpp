/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  editserverdialog.cpp  -  description
  begin:     Tue Feb 12 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <iostream>

#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>

#include "editserverdialog.h"

EditServerDialog::EditServerDialog(QWidget* parent,QString group,QString name,QString port,QString serverKey,QString channelName,QString channelKey) :
//                  KDialog(parent,"editserver",true)
                  KDialogBase(parent,"editserver",true,i18n("Edit Server"),
                              KDialogBase::Ok | KDialogBase::Cancel,
                              KDialogBase::Ok,true)

{
  cerr << "EditServerDialog::EditServerDialog()" << endl;

  QWidget* page=new QWidget(this);
  setMainWidget(page);

  QGridLayout* layout=new QGridLayout(page,3,4);
  layout->setMargin(marginHint());
  layout->setSpacing(spacingHint());
  layout->setColStretch(1,10);

  QLabel* groupNameLabel=new QLabel(i18n("Group name:"),page);
  groupNameInput=new KLineEdit(group,page);

  QLabel* serverNameLabel=new QLabel(i18n("Server name:"),page);

  QHBox* serverBox=new QHBox(page);
  serverBox->setSpacing(spacingHint());
  serverNameInput=new KLineEdit(name,serverBox);
  new QLabel(i18n("Port:"),serverBox);
  serverPortInput=new KLineEdit(port,serverBox);

  QLabel* serverKeyLabel=new QLabel(i18n("Keyword:"),page);
  serverKeyInput=new KLineEdit(serverKey,page);

  QLabel* channelNameLabel=new QLabel(i18n("Channel name:"),page);
  channelNameInput=new KLineEdit(channelName,page);
  QLabel* channelKeyLabel=new QLabel(i18n("Keyword:"),page);
  channelKeyInput=new KLineEdit(channelKey,page);

  layout->addWidget(groupNameLabel,0,0);
  layout->addMultiCellWidget(groupNameInput,0,0,1,3);

  layout->addWidget(serverNameLabel,1,0);
  layout->addWidget(serverBox,1,1);
  layout->addWidget(serverKeyLabel,1,2);
  layout->addWidget(serverKeyInput,1,3);

  layout->addWidget(channelNameLabel,2,0);
  layout->addWidget(channelNameInput,2,1);
  layout->addWidget(channelKeyLabel,2,2);
  layout->addWidget(channelKeyInput,2,3);

  layout->setRowStretch(3,10);

  setButtonOKText(i18n("OK"),i18n("Change server information"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));
}

EditServerDialog::~EditServerDialog()
{
  cerr << "EditServerDialog::~EditServerDialog()" << endl;
}

void EditServerDialog::slotOk()
{
  emit serverChanged(groupNameInput->text(),
                     serverNameInput->text(),
                     serverPortInput->text(),
                     serverKeyInput->text(),
                     channelNameInput->text(),
                     channelKeyInput->text());
  delayedDestruct();
}
