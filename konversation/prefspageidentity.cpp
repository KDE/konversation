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

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>

#include "prefspageidentity.h"

PrefsPageIdentity::PrefsPageIdentity(QFrame* newParent,Preferences* newPreferences) :
                   PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the identity pane
  QGridLayout* identityLayout=new QGridLayout(parentFrame,4,4,marginHint(),spacingHint());

  QLabel* realNameLabel=new QLabel(i18n("Real name:"),parentFrame);
  KLineEdit* realNameInput=new KLineEdit(preferences->getRealName(),parentFrame);

  QLabel* loginLabel=new QLabel(i18n("Ident:"),parentFrame);
  KLineEdit* loginInput=new KLineEdit(preferences->getIdent(),parentFrame);

  QStringList nicknameList=preferences->getNicknameList();

  nick0=new KLineEdit(nicknameList[0],parentFrame);
  nick1=new KLineEdit(nicknameList[1],parentFrame);
  nick2=new KLineEdit(nicknameList[2],parentFrame);
  nick3=new KLineEdit(nicknameList[3],parentFrame);

  QLabel* partLabel=new QLabel(i18n("Part Reason:"),parentFrame);
  KLineEdit* partInput=new KLineEdit(preferences->getPartReason(),parentFrame);

  QLabel* kickLabel=new QLabel(i18n("Kick Reason:"),parentFrame);
  KLineEdit* kickInput=new KLineEdit(preferences->getKickReason(),parentFrame);

  QCheckBox* showAwayMessageCheck=new QCheckBox(i18n("Show away messages"),parentFrame,"away_message_check");
  showAwayMessageCheck->setChecked(preferences->getShowAwayMessage());

  QLabel* awayLabel=new QLabel(i18n("Away message:"),parentFrame);
  KLineEdit* awayInput=new KLineEdit(preferences->getAwayMessage(),parentFrame);

  QLabel* unAwayLabel=new QLabel(i18n("Return message:"),parentFrame);
  KLineEdit* unAwayInput=new KLineEdit(preferences->getUnAwayMessage(),parentFrame);

  int row=0;
  identityLayout->addWidget(realNameLabel,row,0);
  identityLayout->addMultiCellWidget(realNameInput,row,row,1,3);
  row++;
  identityLayout->addWidget(loginLabel,row,0);
  identityLayout->addMultiCellWidget(loginInput,row,row,1,3);
  row++;
  identityLayout->addWidget(new QLabel("Nickname 1:",parentFrame),row,0);
  identityLayout->addWidget(nick0,row,1);
  identityLayout->addWidget(new QLabel("Nickname 3:",parentFrame),row,2);
  identityLayout->addWidget(nick2,row,3);
  row++;
  identityLayout->addWidget(new QLabel("Nickname 2:",parentFrame),row,0);
  identityLayout->addWidget(nick1,row,1);
  identityLayout->addWidget(new QLabel("Nickname 4:",parentFrame),row,2);
  identityLayout->addWidget(nick3,row,3);
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
  identityLayout->addMultiCellWidget(new QLabel(i18n(
                                     "<qt>This is the default identity used for all servers "
                                     "where no separate identity was selected.</qt>"),
                                     parentFrame),row,row,0,3);
  row++;
  identityLayout->setRowStretch(row,10);

  // Set up signals / slots for identity page
  connect(realNameInput,SIGNAL (textChanged(const QString&)),this,SLOT (realNameChanged(const QString&)) );
  connect(loginInput,SIGNAL (textChanged(const QString&)),this,SLOT (loginChanged(const QString&)) );
  connect(nick0,SIGNAL (textChanged(const QString&)),this,SLOT (nick0Changed(const QString&)) );
  connect(nick1,SIGNAL (textChanged(const QString&)),this,SLOT (nick1Changed(const QString&)) );
  connect(nick2,SIGNAL (textChanged(const QString&)),this,SLOT (nick2Changed(const QString&)) );
  connect(nick3,SIGNAL (textChanged(const QString&)),this,SLOT (nick3Changed(const QString&)) );
  connect(partInput,SIGNAL (textChanged(const QString&)),this,SLOT (partReasonChanged(const QString&)) );
  connect(kickInput,SIGNAL (textChanged(const QString&)),this,SLOT (kickReasonChanged(const QString&)) );
  connect(showAwayMessageCheck,SIGNAL (stateChanged(int)),this,SLOT (showAwayMessageChanged(int)) );
  connect(awayInput,SIGNAL (textChanged(const QString&)),this,SLOT (awayMessageChanged(const QString&)) );
  connect(unAwayInput,SIGNAL (textChanged(const QString&)),this,SLOT (unAwayMessageChanged(const QString&)) );
}

PrefsPageIdentity::~PrefsPageIdentity()
{
}

void PrefsPageIdentity::realNameChanged(const QString& newRealName)
{
  preferences->setRealName(newRealName);
}

void PrefsPageIdentity::loginChanged(const QString& newLogin)
{
  preferences->setIdent(newLogin);
}

// TODO: derive from QLineEdit and submit an index in the signal to
//       avoid duplicate code like this
void PrefsPageIdentity::nick0Changed(const QString& newNick)
{
  preferences->setNickname(0,newNick);
}

void PrefsPageIdentity::nick1Changed(const QString& newNick)
{
  preferences->setNickname(1,newNick);
}

void PrefsPageIdentity::nick2Changed(const QString& newNick)
{
  preferences->setNickname(2,newNick);
}

void PrefsPageIdentity::nick3Changed(const QString& newNick)
{
  preferences->setNickname(3,newNick);
}

void PrefsPageIdentity::partReasonChanged(const QString& newReason)
{
  preferences->setPartReason(newReason);
}

void PrefsPageIdentity::kickReasonChanged(const QString& newReason)
{
  preferences->setKickReason(newReason);
}

void PrefsPageIdentity::showAwayMessageChanged(int state)
{
  preferences->setShowAwayMessage(state==2);
}

void PrefsPageIdentity::awayMessageChanged(const QString& newMessage)
{
  preferences->setAwayMessage(newMessage);
}

void PrefsPageIdentity::unAwayMessageChanged(const QString& newMessage)
{
  preferences->setUnAwayMessage(newMessage);
}

#include "prefspageidentity.moc"
