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

  $Id$
*/

#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <klineedit.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>

#include "editserverdialog.h"
#include "konversationapplication.h"
#include "identity.h"

EditServerDialog::EditServerDialog(QWidget* parent,
                                   QString group,
                                   QString name,
                                   QString port,
                                   QString serverKey,
                                   QString channelName,
                                   QString channelKey,
                                   QString currentIdentity) :

                  KDialogBase(parent,"editserver",true,i18n("Edit server"),
                              KDialogBase::Ok | KDialogBase::Cancel,
                              KDialogBase::Ok,true)

{
  kdDebug() << "EditServerDialog::EditServerDialog("<< currentIdentity <<")" << endl;

  QWidget* page=new QWidget(this);
  setMainWidget(page);

  QGridLayout* layout=new QGridLayout(page,3,4);
  layout->setSpacing(spacingHint());
  layout->setColStretch(1,10);

  QLabel* groupNameLabel=new QLabel(i18n("Group name:"),page);
  groupNameInput=new KLineEdit(group,page);

  QLabel* identityLabel=new QLabel(i18n("Identity:"),page);
  identityCombo=new KComboBox(page);

  QPtrList<Identity> identities=KonversationApplication::preferences.getIdentityList();

  for(unsigned int index=0;index<identities.count();index++)
  {
    QString name=identities.at(index)->getName();
    identityCombo->insertItem(name);
    if(name==currentIdentity) identityCombo->setCurrentItem(identityCombo->count()-1);
  }

  QLabel* serverNameLabel=new QLabel(i18n("Server name:"),page);

  QHBox* serverBox=new QHBox(page);
  serverBox->setSpacing(spacingHint());
  serverNameInput=new KLineEdit(name,serverBox);
  new QLabel(i18n("Port:"),serverBox);
  serverPortInput=new KLineEdit(port,serverBox);

  QLabel* serverKeyLabel=new QLabel(i18n("Keyword:"),page);
  serverKeyInput=new KLineEdit(serverKey,page);
  serverKeyInput->setEchoMode(QLineEdit::Password);

  QLabel* channelNameLabel=new QLabel(i18n("Channel name:"),page);
  channelNameInput=new KLineEdit(channelName,page);
  QLabel* channelKeyLabel=new QLabel(i18n("Keyword:"),page);
  channelKeyInput=new KLineEdit(channelKey,page);
  channelKeyInput->setEchoMode(QLineEdit::Password);

  QHBox* spacer=new QHBox(page);

  int row=0;

  layout->addWidget(groupNameLabel,row,0);
  layout->addWidget(groupNameInput,row,1);
  layout->addWidget(identityLabel,row,2);
  layout->addWidget(identityCombo,row,3);

  row++;
  layout->addWidget(serverNameLabel,row,0);
  layout->addWidget(serverBox,row,1);
  layout->addWidget(serverKeyLabel,row,2);
  layout->addWidget(serverKeyInput,row,3);

  row++;
  layout->addWidget(channelNameLabel,row,0);
  layout->addWidget(channelNameInput,row,1);
  layout->addWidget(channelKeyLabel,row,2);
  layout->addWidget(channelKeyInput,row,3);

  row++;
  layout->addMultiCellWidget(spacer,row,row,0,3);
  layout->setRowStretch(row,10);

  setButtonOKText(i18n("OK"),i18n("Change server information"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));
}

EditServerDialog::~EditServerDialog()
{
  kdDebug() << "EditServerDialog::~EditServerDialog()" << endl;
}

void EditServerDialog::slotOk()
{
  emit serverChanged(groupNameInput->text(),
                     serverNameInput->text(),
                     serverPortInput->text(),
                     serverKeyInput->text(),
                     channelNameInput->text(),
                     channelKeyInput->text(),
                     identityCombo->currentText());
  delayedDestruct();
}

#include "editserverdialog.moc"
