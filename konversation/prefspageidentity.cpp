/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageidentity.cpp  -  Provides a user interface to customize identity settings
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include <klineeditdlg.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>

#include "prefspageidentity.h"
#include "preferences.h"
#include "identity.h"

PrefsPageIdentity::PrefsPageIdentity(QFrame* newParent,Preferences* newPreferences) :
                   PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the identity pane
  QGridLayout* identityLayout=new QGridLayout(parentFrame,4,4,marginHint(),spacingHint());

  QLabel* identityLabel=new QLabel(i18n("Identity:"),parentFrame);
  identityCombo=new KComboBox(parentFrame);
  identityCombo->setEditable(false);
//  identityCombo->setInsertionPolicy(QComboBox::NoInsertion);

  QPushButton* renameButton=new QPushButton(i18n("Rename"),parentFrame,"rename_identity_button");
  
  identities=preferences->getIdentityList();

  for(unsigned int index=0;index<identities.count();index++)
    identityCombo->insertItem(identities.at(index)->getName());

  QLabel* realNameLabel=new QLabel(i18n("Real name:"),parentFrame);
  realNameInput=new KLineEdit(parentFrame);

  QLabel* loginLabel=new QLabel(i18n("Ident:"),parentFrame);
  loginInput=new KLineEdit(parentFrame);

  nick0=new KLineEdit(parentFrame);
  nick1=new KLineEdit(parentFrame);
  nick2=new KLineEdit(parentFrame);
  nick3=new KLineEdit(parentFrame);

  bot=new KLineEdit(parentFrame);
  password=new KLineEdit(parentFrame);
  password->setEchoMode(QLineEdit::Password);
  
  QLabel* partLabel=new QLabel(i18n("Part reason:"),parentFrame);
  partInput=new KLineEdit(parentFrame);

  QLabel* kickLabel=new QLabel(i18n("Kick reason:"),parentFrame);
  kickInput=new KLineEdit(parentFrame);

  showAwayMessageCheck=new QCheckBox(i18n("Show away messages"),parentFrame,"away_message_check");

  awayLabel=new QLabel(i18n("Away message:"),parentFrame);
  awayInput=new KLineEdit(parentFrame);

  unAwayLabel=new QLabel(i18n("Return message:"),parentFrame);
  unAwayInput=new KLineEdit(parentFrame);

  defaultText=new QLabel(i18n("<qt>This is the default identity used for all servers "
                              "where no separate identity was selected.</qt>"),
                              parentFrame);

  QHBox* buttonBox=new QHBox(parentFrame);
  buttonBox->setSpacing(spacingHint());

  QPushButton* addIdentityButton=new QPushButton(i18n("Add new identity"),buttonBox,"add_identity_button");
  removeIdentityButton=new QPushButton(i18n("Remove identity"),buttonBox,"remove_identity_button");
  
  QHBox* spacer=new QHBox(parentFrame);

  // set values for the widgets
  updateIdentity(0);

  int row=0;
  identityLayout->addWidget(identityLabel,row,0);
  identityLayout->addMultiCellWidget(identityCombo,row,row,1,2);
  identityLayout->addWidget(renameButton,row,3);
  row++;
  identityLayout->addWidget(realNameLabel,row,0);
  identityLayout->addMultiCellWidget(realNameInput,row,row,1,3);
  row++;
  identityLayout->addWidget(loginLabel,row,0);
  identityLayout->addMultiCellWidget(loginInput,row,row,1,3);
  row++;
  identityLayout->addWidget(new QLabel(i18n("Nickname %1:").arg(1),parentFrame),row,0);
  identityLayout->addWidget(nick0,row,1);
  identityLayout->addWidget(new QLabel(i18n("Nickname %1:").arg(2),parentFrame),row,2);
  identityLayout->addWidget(nick1,row,3);
  row++;
  identityLayout->addWidget(new QLabel(i18n("Nickname %1:").arg(3),parentFrame),row,0);
  identityLayout->addWidget(nick2,row,1);
  identityLayout->addWidget(new QLabel(i18n("Nickname %1:").arg(4),parentFrame),row,2);
  identityLayout->addWidget(nick3,row,3);
  row++;
  identityLayout->addWidget(new QLabel(i18n("Service:"), parentFrame),row,0);
  identityLayout->addWidget(bot,row,1);
  identityLayout->addWidget(new QLabel(i18n("Password:"), parentFrame),row,2);
  identityLayout->addWidget(password,row,3);
  row++;
  identityLayout->addWidget(partLabel,row,0);
  identityLayout->addMultiCellWidget(partInput,row,row,1,3);
  row++;
  identityLayout->addWidget(kickLabel,row,0);
  identityLayout->addMultiCellWidget(kickInput,row,row,1,3);
  row++;
  identityLayout->addMultiCellWidget(showAwayMessageCheck,row,row,0,3);
  row++;
  identityLayout->addWidget(awayLabel,row,0);
  identityLayout->addMultiCellWidget(awayInput,row,row,1,3);
  row++;
  identityLayout->addWidget(unAwayLabel,row,0);
  identityLayout->addMultiCellWidget(unAwayInput,row,row,1,3);
  row++;
  identityLayout->addMultiCellWidget(defaultText,row,row,0,3);
  row++;
  identityLayout->addMultiCellWidget(buttonBox,row,row,0,3);
  row++;
  identityLayout->addMultiCellWidget(spacer,row,row,0,3);
  identityLayout->setRowStretch(row,10);

  // Set up signals / slots for identity page
  connect(identityCombo,SIGNAL (activated(int)),this,SLOT (updateIdentity(int)) );
//  connect(identityCombo,SIGNAL (textChanged(const QString&)),this,SLOT (renameIdentity(const QString&)) );
  
  connect(renameButton,SIGNAL (clicked()),this,SLOT (renameIdentity()) );
  
  connect(realNameInput,SIGNAL (textChanged(const QString&)),this,SLOT (realNameChanged(const QString&)) );
  
  connect(loginInput,SIGNAL (textChanged(const QString&)),this,SLOT (loginChanged(const QString&)) );
  
  connect(nick0,SIGNAL (textChanged(const QString&)),this,SLOT (nick0Changed(const QString&)) );
  connect(nick1,SIGNAL (textChanged(const QString&)),this,SLOT (nick1Changed(const QString&)) );
  connect(nick2,SIGNAL (textChanged(const QString&)),this,SLOT (nick2Changed(const QString&)) );
  connect(nick3,SIGNAL (textChanged(const QString&)),this,SLOT (nick3Changed(const QString&)) );
  
  connect(bot,SIGNAL (textChanged(const QString&)), this,SLOT (botChanged(const QString&)) );
  connect(password,SIGNAL (textChanged(const QString&)), this,SLOT (passwordChanged(const QString&)) );
  
  connect(partInput,SIGNAL (textChanged(const QString&)),this,SLOT (partReasonChanged(const QString&)) );
  connect(kickInput,SIGNAL (textChanged(const QString&)),this,SLOT (kickReasonChanged(const QString&)) );
  
  connect(showAwayMessageCheck,SIGNAL (stateChanged(int)),this,SLOT (showAwayMessageChanged(int)) );
  connect(awayInput,SIGNAL (textChanged(const QString&)),this,SLOT (awayMessageChanged(const QString&)) );
  connect(unAwayInput,SIGNAL (textChanged(const QString&)),this,SLOT (unAwayMessageChanged(const QString&)) );
  
  connect(addIdentityButton,SIGNAL (clicked()),this,SLOT(addIdentity()) );
  connect(removeIdentityButton,SIGNAL (clicked()),this,SLOT(removeIdentity()) );
}

PrefsPageIdentity::~PrefsPageIdentity()
{
}

void PrefsPageIdentity::realNameChanged(const QString& newRealName)
{
  identity->setRealName(newRealName);
}

void PrefsPageIdentity::loginChanged(const QString& newLogin)
{
  identity->setIdent(newLogin);
}

// TODO: derive from QLineEdit and submit an index in the signal to
//       avoid duplicate code like this
void PrefsPageIdentity::nick0Changed(const QString& newNick)
{
  identity->setNickname(0,newNick);
}

void PrefsPageIdentity::nick1Changed(const QString& newNick)
{
  identity->setNickname(1,newNick);
}

void PrefsPageIdentity::nick2Changed(const QString& newNick)
{
  identity->setNickname(2,newNick);
}

void PrefsPageIdentity::nick3Changed(const QString& newNick)
{
  identity->setNickname(3,newNick);
}

void PrefsPageIdentity::botChanged(const QString& newBot)
{
  identity->setBot(newBot);
}

void PrefsPageIdentity::passwordChanged(const QString& newPassword)
{
  identity->setPassword(newPassword);
}

void PrefsPageIdentity::partReasonChanged(const QString& newReason)
{
  identity->setPartReason(newReason);
}

void PrefsPageIdentity::kickReasonChanged(const QString& newReason)
{
  identity->setKickReason(newReason);
}

void PrefsPageIdentity::showAwayMessageChanged(int state)
{
  identity->setShowAwayMessage(state==2);
  updateAwayWidgets(state==2);
}

void PrefsPageIdentity::awayMessageChanged(const QString& newMessage)
{
  identity->setAwayMessage(newMessage);
}

void PrefsPageIdentity::unAwayMessageChanged(const QString& newMessage)
{
  identity->setReturnMessage(newMessage);
}

void PrefsPageIdentity::updateAwayWidgets(bool enabled)
{
  awayLabel->setEnabled(enabled);
  awayInput->setEnabled(enabled);
  unAwayLabel->setEnabled(enabled);
  unAwayInput->setEnabled(enabled);
}

void PrefsPageIdentity::updateIdentity(int number)
{
  identity=identities.at(number);

  if(number==0) defaultText->show();
  else defaultText->hide();

  // TODO: Enable the button when all's fine
  removeIdentityButton->setEnabled((number!=0));
//  removeIdentityButton->setEnabled(false);

  loginInput->setText(identity->getIdent());
  realNameInput->setText(identity->getRealName());

  nick0->setText(identity->getNickname(0));
  nick1->setText(identity->getNickname(1));
  nick2->setText(identity->getNickname(2));
  nick3->setText(identity->getNickname(3));

  bot->setText(identity->getBot());
  password->setText(identity->getPassword());

  partInput->setText(identity->getPartReason());
  kickInput->setText(identity->getKickReason());

  showAwayMessageCheck->setChecked(identity->getShowAwayMessage());
  awayInput->setText(identity->getAwayMessage());
  unAwayInput->setText(identity->getReturnMessage());

  updateAwayWidgets(identity->getShowAwayMessage());
}

// void PrefsPageIdentity::renameIdentity(const QString& newName)
void PrefsPageIdentity::renameIdentity()
{
  bool ok;
  QString newName=KLineEditDlg::getText(i18n("Rename identity"),
                                        i18n("Please enter a new name for this identity:"),
                                        identity->getName(),
                                        &ok,
                                        parentFrame);
  if(ok)
  {
    identity->setName(newName);
    identityCombo->changeItem(newName,identityCombo->currentItem());
  }
}

void PrefsPageIdentity::addIdentity()
{
  identity=new Identity();
  identity->setName(i18n("New Identity"));

  preferences->addIdentity(identity);
  identities=preferences->getIdentityList();

  identityCombo->insertItem(identity->getName());
  identityCombo->setCurrentItem(identityCombo->count()-1);

  updateIdentity(identityCombo->count()-1);
}

void PrefsPageIdentity::removeIdentity()
{
  // TODO: are you sure here
  int current=identityCombo->currentItem();
  
  if(current)
  {
    preferences->removeIdentity(identity);
    identities=preferences->getIdentityList();

    delete identity;

    identityCombo->removeItem(current);
    updateIdentity(identityCombo->currentItem());
  }
  else
    // should not happen!
    kdDebug() << "Trying to delete the default identity! This should never happen!" << endl;

}

#include "prefspageidentity.moc"
