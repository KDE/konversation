/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefsdialog.cpp  -  description
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <iostream>

#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>

#include <klocale.h>

#include "prefsdialog.h"
#include "serverlistitem.h"
#include "editserverdialog.h"

PrefsDialog::PrefsDialog(Preferences* preferences,bool noServer) :
             KDialogBase(0,"editprefs",false,i18n("Edit Preferences"),
                         KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                         KDialogBase::Ok,true)
{
  cerr << "PrefsDialog::PrefsDialog()" << endl;
  setPreferences(preferences);

  /* Create the top level widget */
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  /* Create the TabWidget */
  prefsTabs=new QTabWidget(page);
  prefsTabs->setMargin(marginHint());
  /* Add the layout to the widget */
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->addWidget(prefsTabs);

  /* Server list pane*/

  /* Here goes the server list and the "connect / new / remove  ..." button box */
  QVBox* serverListPane=new QVBox(prefsTabs);
  serverListPane->setSpacing(spacingHint());
  /* Set up the server list */
  serverListView=new KListView(serverListPane);
  serverListView->setItemsRenameable(true);
  serverListView->addColumn(i18n("Group"));
  serverListView->addColumn(i18n("Server"));
  serverListView->addColumn(i18n("Port"));
  serverListView->addColumn(i18n("Keyword"));
  serverListView->addColumn(i18n("Channel"));
  serverListView->addColumn(i18n("Keyword"));
  serverListView->setRenameable(0,true);
  serverListView->setRenameable(1,true);
  serverListView->setRenameable(2,true);
  serverListView->setRenameable(4,true);

  serverListView->setAllColumnsShowFocus(true);

  /* Fill in the servers from the preferences */
  int index=0;

  QString serverString=preferences->getServerByIndex(index);
  while(serverString)
  {
    int id=preferences->getServerIdByIndex(index);
    QStringList serverEntry=QStringList::split(',',serverString,true);
    new ServerListItem(serverListView,id,serverEntry[0],
                                         serverEntry[1],
                                         serverEntry[2],
                                         (serverEntry[3]) ? "********" : "",
                                         serverEntry[4],
                                         (serverEntry[5]) ? "********" : "");

    serverString=preferences->getServerByIndex(++index);
  }
  /* Set up the button box */
  QHBox* buttonBox=new QHBox(serverListPane);
  buttonBox->setSpacing(spacingHint());
  /* Add the buttons */
  connectButton=new QPushButton(i18n("Connect"),buttonBox);
  connectButton->setDisabled(true);
/*  QPushButton* newServerButton= */ new QPushButton(i18n("New Server"),buttonBox);
  editServerButton=new QPushButton(i18n("Edit"),buttonBox);
  editServerButton->setDisabled(true);
  removeServerButton=new QPushButton(i18n("Remove"),buttonBox);
  removeServerButton->setDisabled(true);

  /* Identity pane */

  QWidget* identityPane=new QWidget(prefsTabs);

  QLabel* realNameLabel=new QLabel(i18n("Real name:"),identityPane);
  KLineEdit* realNameInput=new KLineEdit(preferences->realname,identityPane);

  QLabel* loginLabel=new QLabel(i18n("Ident:"),identityPane);
  KLineEdit* loginInput=new KLineEdit(preferences->ident,identityPane);

  KLineEdit* nick0=new KLineEdit(preferences->nicknameList[0],identityPane);
  KLineEdit* nick1=new KLineEdit(preferences->nicknameList[1],identityPane);
  KLineEdit* nick2=new KLineEdit(preferences->nicknameList[2],identityPane);
  KLineEdit* nick3=new KLineEdit(preferences->nicknameList[3],identityPane);
  /* Add a Layout to the identity pane */
  QGridLayout* identityLayout=new QGridLayout(identityPane,4,4);
  identityLayout->setSpacing(spacingHint());
  identityLayout->addWidget(realNameLabel,0,0);
  identityLayout->addMultiCellWidget(realNameInput,0,0,1,3);
  identityLayout->addWidget(loginLabel,1,0);
  identityLayout->addMultiCellWidget(loginInput,1,1,1,3);
  identityLayout->addWidget(new QLabel("Nickname 1:",identityPane),2,0);
  identityLayout->addWidget(nick0,2,1);
  identityLayout->addWidget(new QLabel("Nickname 2:",identityPane),3,0);
  identityLayout->addWidget(nick1,3,1);
  identityLayout->addWidget(new QLabel("Nickname 3:",identityPane),2,2);
  identityLayout->addWidget(nick2,2,3);
  identityLayout->addWidget(new QLabel("Nickname 4:",identityPane),3,2);
  identityLayout->addWidget(nick3,3,3);
  identityLayout->setRowStretch(4,10);

  /* Add the panes to the Tab Widget */
  prefsTabs->addTab(serverListPane,i18n("Server List"));
  prefsTabs->addTab(identityPane,i18n("Identity"));

  /* Set up signals / slots for server list */
  connect(connectButton,SIGNAL(clicked()),
                   this,SLOT  (connectClicked()) );
  connect(editServerButton,SIGNAL(clicked()),
                      this,SLOT  (editServer()) );
  connect(serverListView,SIGNAL(selectionChanged(QListViewItem*)),
                    this,SLOT  (serverSelected(QListViewItem*)) );
  connect(serverListView,SIGNAL(itemRenamed(QListViewItem*,const QString&,int)),
                    this,SLOT  (updateServerProperty(QListViewItem*,const QString&,int)) );

  // FIXME: Double click Server Entry in PrefsDialog!
  // This would delete the ListView while inside the doubleClicked() signal and
  // this is not allowed. We must find another way to du this!
  connect(serverListView,SIGNAL(doubleClicked(QListViewItem*)),
                    this,SLOT  (serverDoubleClicked(QListViewItem*)) );


  /* Set up signals / slots for identity page */
  connect(realNameInput,SIGNAL (textChanged(const QString&)),this,SLOT (realNameChanged(const QString&)) );
  connect(loginInput,SIGNAL (textChanged(const QString&)),this,SLOT (loginChanged(const QString&)) );
  connect(nick0,SIGNAL (textChanged(const QString&)),this,SLOT (nick0Changed(const QString&)) );
  connect(nick1,SIGNAL (textChanged(const QString&)),this,SLOT (nick1Changed(const QString&)) );
  connect(nick2,SIGNAL (textChanged(const QString&)),this,SLOT (nick2Changed(const QString&)) );
  connect(nick3,SIGNAL (textChanged(const QString&)),this,SLOT (nick3Changed(const QString&)) );

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));

  if(noServer)
  {
    enableButtonOK(false);
    setButtonCancelText(i18n("Quit"),i18n("Quits Application"));
  }
  else
  {
    setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));
  }
}

PrefsDialog::~PrefsDialog()
{
  cerr << "PrefsDialog::~PrefsDialog()" << endl;
}

void PrefsDialog::connectClicked()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  /* Do I really need to cast here? Isn't there a better way? */
  ServerListItem* item=(ServerListItem*)lv_item;
  if(item) emit connectToServer(item->getId());
}

void PrefsDialog::editServer()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  /* Do I really need to cast here? Isn't there a better way? */
  ServerListItem* item=(ServerListItem*)lv_item;
  if(item)
  {
    QString server=preferences->getServerById(item->getId());
    if(server)
    {
      QStringList properties=QStringList::split(',',server,true);
      EditServerDialog editServerDialog(this,properties[0],properties[1],properties[2],properties[3],properties[4],properties[5]);

      connect(&editServerDialog,SIGNAL (serverChanged(const QString&,
                                                      const QString&,
                                                      const QString&,
                                                      const QString&,
                                                      const QString&,
                                                      const QString&)),
                              this,SLOT (updateServer(const QString&,
                                                      const QString&,
                                                      const QString&,
                                                      const QString&,
                                                      const QString&,
                                                      const QString&)) );
      editServerDialog.exec();
    }
  }
}

void PrefsDialog::serverSelected(QListViewItem* item)
{
  bool mode=false;

  if(item) mode=true;

  connectButton->setEnabled(mode);
  editServerButton->setEnabled(mode);
  removeServerButton->setEnabled(mode);
}

void PrefsDialog::serverDoubleClicked(QListViewItem* item)
{
  connectClicked();
}

void PrefsDialog::updateServer(const QString& groupName,
                               const QString& serverName,
                               const QString& serverPort,
                               const QString& serverKey,
                               const QString& channelName,
                               const QString& channelKey)
{
  QListViewItem* item=serverListView->selectedItems().first();
  /* Need to find a better way without casting */
  ServerListItem* serverItem=(ServerListItem*) item;
  int id=serverItem->getId();

  serverItem->setText(0,groupName);
  serverItem->setText(1,serverName);
  serverItem->setText(2,serverPort);
  serverItem->setText(3,(!serverKey || serverKey=="") ? "" : "********");
  serverItem->setText(4,channelName);
  serverItem->setText(5,(!channelKey || channelKey=="") ? "" : "********");

  preferences->updateServer(id,groupName+","+
                               serverName+","+
                               serverPort+","+
                               serverKey+","+
                               channelName+","+
                               channelKey);
}

void PrefsDialog::updateServerProperty(QListViewItem* item,const QString& value,int property)
{
  /* Need to find a better way without casting */
  ServerListItem* serverItem=(ServerListItem*) item;
  int id=serverItem->getId();

  preferences->changeServerProperty(id,property,value);
}

void PrefsDialog::realNameChanged(const QString& newRealName)
{
  preferences->realname=newRealName;
}

void PrefsDialog::loginChanged(const QString& newLogin)
{
  preferences->ident=newLogin;
}

void PrefsDialog::nick0Changed(const QString& newNick)
{
  preferences->nicknameList[0]=newNick;
}

void PrefsDialog::nick1Changed(const QString& newNick)
{
  preferences->nicknameList[1]=newNick;
}

void PrefsDialog::nick2Changed(const QString& newNick)
{
  preferences->nicknameList[2]=newNick;
}

void PrefsDialog::nick3Changed(const QString& newNick)
{
  preferences->nicknameList[3]=newNick;
}

void PrefsDialog::slotOk()
{
  slotApply();
  slotCancel();
}

void PrefsDialog::slotApply()
{
  emit prefsChanged();
}

void PrefsDialog::slotCancel()
{
  cerr << "PrefsDialog::slotCancel()" << endl;
  emit cancelClicked();
}
