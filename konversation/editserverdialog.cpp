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

#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>

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
                                   QString currentIdentity,
                                   QString connectCommands) :

                  KDialogBase(parent,"editserver",true,i18n("Edit Server"),
                              KDialogBase::Ok | KDialogBase::Cancel,
                              KDialogBase::Ok,true)

{
  QWidget* page=new QWidget(this);
  setMainWidget(page);

  QGridLayout* layout=new QGridLayout(page,4,4);
  layout->setSpacing(spacingHint());
  layout->setColStretch(1,10);

  QLabel* groupNameLabel=new QLabel(i18n("&Group name:"),page);
  QString groupNameWT = i18n("Enter anything you like here. It is used to "
                             "organize the entries in the Server List.");
  QWhatsThis::add(groupNameLabel, groupNameWT);
  groupNameInput=new KLineEdit(group,page);
  QWhatsThis::add(groupNameInput, groupNameWT);
  groupNameLabel->setBuddy(groupNameInput);

  QLabel* identityLabel=new QLabel(i18n("&Identity:"),page);
  QString identityWT = i18n("<qt>Your identity, among other things, "
                            "determines your starting nickname. (See "
                            "the <strong>Identity</strong> page in the "
                            "Preferences to add or modify identities or <a "
                            "href=\"help:/konversation/identity.html\">"
                            "Setting your identity</a> in the manual for more "
                            "information).</qt>");
  QWhatsThis::add(identityLabel, identityWT);
  identityCombo=new KComboBox(page);
  QWhatsThis::add(identityCombo, identityWT);
  identityLabel->setBuddy(identityCombo);

  QPtrList<Identity> identities=KonversationApplication::preferences.getIdentityList();

  for(unsigned int index=0;index<identities.count();index++)
  {
    QString name=identities.at(index)->getName();
    identityCombo->insertItem(name);
    if(name==currentIdentity) identityCombo->setCurrentItem(identityCombo->count()-1);
  }

  QLabel* serverNameLabel=new QLabel(i18n("&Server name:"),page);
  QString serverNameWT = i18n("<qt><p>Enter the host name of the IRC server "
                              "here.\nSee irchelp.org for a <a "
                              "href='http://www.irchelp.org/irchelp/networks/'"
                              ">list of networks and servers</a>.</p>"
                              "<p>Example:  irc.kde.org</p></qt>");
  QHBox* serverBox=new QHBox(page);
  serverBox->setSpacing(spacingHint());
  serverNameInput=new KLineEdit(name,serverBox);
  QWhatsThis::add(serverNameInput, serverNameWT);
  serverNameLabel->setBuddy(serverNameInput);
  QWhatsThis::add(serverNameLabel, serverNameWT);

  QLabel* serverPortLabel=new QLabel(i18n("P&ort:"),serverBox);
  QString serverPortWT = i18n("Enter the port number on which the IRC "
                              "server listens.  (The default IRC port is "
                              "6667 and usually need not be changed.)\n\n"
                              "Example: 6667");
  QWhatsThis::add(serverPortLabel, serverPortWT);
  serverPortInput=new KLineEdit(port,serverBox);
  QWhatsThis::add(serverPortInput, serverPortWT);
  serverPortLabel->setBuddy(serverPortInput);

  QLabel* serverKeyLabel=new QLabel(i18n("&Password:"),page);
  QString serverKeyWT = i18n("If your IRC server requires a password, enter "
                             "it here.  (Most servers do not require a "
                             "password.)");
  QWhatsThis::add(serverKeyLabel, serverKeyWT);
  serverKeyInput=new KLineEdit(serverKey,page);
  serverKeyInput->setEchoMode(QLineEdit::Password);
  QWhatsThis::add(serverKeyInput, serverKeyWT);
  serverKeyLabel->setBuddy(serverKeyInput);

  QLabel* channelNameLabel=new QLabel(i18n("C&hannel name:"),page);
  QString channelNameWT = i18n("Enter the names of zero or more channels to "
                               "join automatically when this server is "
                               "connected. (Most channel names start with #. "
                               "Separate multiple channel names with "
                               "spaces.)\n\n"
                               "Example:  #debian #kde");
  QWhatsThis::add(channelNameLabel, channelNameWT);
  channelNameInput=new KLineEdit(channelName,page);
  QWhatsThis::add(channelNameInput, channelNameWT);
  channelNameLabel->setBuddy(channelNameInput);
  
  QLabel* connectCommandsLabel = new QLabel(i18n("C&onnect Command(s):"), page);
  QString connectCommandsWT = i18n("Enter the command(s) you want to execute on "
                                                                "connection to the server(separated by semicolon)");
  QWhatsThis::add(connectCommandsLabel, connectCommandsWT);
  connectCommandsInput = new KLineEdit(connectCommands, page);
  QWhatsThis::add(connectCommandsInput, connectCommandsWT);
  connectCommandsLabel->setBuddy(connectCommandsInput);

  QLabel* channelKeyLabel=new QLabel(i18n("Pass&word:"),page);
  QString channelKeyWT = i18n("If the channel requires a password in order "
                              "to join, enter it here, otherwise leave blank. "
                              "This password will apply to all channels "
                              "listed in the Channel name box.");
  QWhatsThis::add(channelKeyLabel, channelKeyWT);
  channelKeyInput=new KLineEdit(channelKey,page);
  channelKeyInput->setEchoMode(QLineEdit::Password);
  QWhatsThis::add(channelKeyInput, channelKeyWT);
  channelKeyLabel->setBuddy(channelKeyInput);

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
  
  row++;
  layout->addWidget(connectCommandsLabel, row, 0);
  layout->addMultiCellWidget(connectCommandsInput, row, row, 1, 3);

  setButtonOKText(i18n("&OK"),i18n("Change server information"));
  setButtonCancelText(i18n("&Cancel"),i18n("Discards all changes made"));
}

EditServerDialog::~EditServerDialog()
{
}

void EditServerDialog::slotOk()
{
  emit serverChanged(groupNameInput->text(),
                     serverNameInput->text(),
                     serverPortInput->text(),
                     serverKeyInput->text(),
                     channelNameInput->text(),
                     channelKeyInput->text(),
                     identityCombo->currentText(),
                     connectCommandsInput->text());
  delayedDestruct();
}

#include "editserverdialog.moc"
