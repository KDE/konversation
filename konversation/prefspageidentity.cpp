/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageidentity.cpp  -  description
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qlabel.h>

#include <klineedit.h>

#include "prefspageidentity.h"

PrefsPageIdentity::PrefsPageIdentity(QFrame* newParent,Preferences* newPreferences) :
                   PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the identity pane
  QGridLayout* identityLayout=new QGridLayout(parentFrame,4,4,marginHint(),spacingHint());

  QLabel* realNameLabel=new QLabel(i18n("Real name:"),parentFrame);
  KLineEdit* realNameInput=new KLineEdit(preferences->realname,parentFrame);

  QLabel* loginLabel=new QLabel(i18n("Ident:"),parentFrame);
  KLineEdit* loginInput=new KLineEdit(preferences->ident,parentFrame);

  QStringList nicknameList=preferences->getNicknameList();

  KLineEdit* nick0=new KLineEdit(nicknameList[0],parentFrame);
  KLineEdit* nick1=new KLineEdit(nicknameList[1],parentFrame);
  KLineEdit* nick2=new KLineEdit(nicknameList[2],parentFrame);
  KLineEdit* nick3=new KLineEdit(nicknameList[3],parentFrame);

  int row=0;
  identityLayout->addMultiCellWidget(new QLabel(i18n(
                                     "<qt>This is the default identity used for all servers "
                                     "where no separate identity was selected.</qt>"),
                                     parentFrame),row,row,0,3);
  row++;
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
  identityLayout->setRowStretch(row,10);

  // Set up signals / slots for identity page
  connect(realNameInput,SIGNAL (textChanged(const QString&)),this,SLOT (realNameChanged(const QString&)) );
  connect(loginInput,SIGNAL (textChanged(const QString&)),this,SLOT (loginChanged(const QString&)) );
  connect(nick0,SIGNAL (textChanged(const QString&)),this,SLOT (nick0Changed(const QString&)) );
  connect(nick1,SIGNAL (textChanged(const QString&)),this,SLOT (nick1Changed(const QString&)) );
  connect(nick2,SIGNAL (textChanged(const QString&)),this,SLOT (nick2Changed(const QString&)) );
  connect(nick3,SIGNAL (textChanged(const QString&)),this,SLOT (nick3Changed(const QString&)) );
}

PrefsPageIdentity::~PrefsPageIdentity()
{
}

void PrefsPageIdentity::realNameChanged(const QString& newRealName)
{
  preferences->realname=newRealName;
}

void PrefsPageIdentity::loginChanged(const QString& newLogin)
{
  preferences->ident=newLogin;
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
