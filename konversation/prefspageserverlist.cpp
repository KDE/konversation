/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageserverlist.cpp  -  description
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhbox.h>

#include "prefspageserverlist.h"
#include "serverlistitem.h"
#include "editserverdialog.h"

PrefsPageServerList::PrefsPageServerList(QFrame* newParent,Preferences* newPreferences) :
                     PrefsPage(newParent,newPreferences)                    
{
  // Add Layout to Server list pane
  QVBoxLayout* serverListLayout=new QVBoxLayout(parentFrame,marginHint(),spacingHint(),"server_list_layout");

  // Set up the server list
  serverListView=new KListView(parentFrame);

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

  // Fill in the servers from the preferences
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
  // Set up the button box
  QHBox* buttonBox=new QHBox(parentFrame);
  buttonBox->setSpacing(spacingHint());
  // Add the buttons
  connectButton=new QPushButton(i18n("Connect"),buttonBox);
  connectButton->setDisabled(true);
  newServerButton=new QPushButton(i18n("New Server"),buttonBox);
  editServerButton=new QPushButton(i18n("Edit"),buttonBox);
  editServerButton->setDisabled(true);
  removeServerButton=new QPushButton(i18n("Remove"),buttonBox);
  removeServerButton->setDisabled(true);

  serverListLayout->addWidget(serverListView);
  serverListLayout->addWidget(buttonBox);

  // Set up signals / slots for server list
  connect(connectButton,SIGNAL(clicked()),
                   this,SLOT  (connectClicked()) );
  connect(newServerButton,SIGNAL(clicked()),
                     this,SLOT  (newServer()) );
  connect(editServerButton,SIGNAL(clicked()),
                      this,SLOT  (editServer()) );
  connect(removeServerButton,SIGNAL(clicked()),
                        this,SLOT  (removeServer()) );
  connect(serverListView,SIGNAL(selectionChanged(QListViewItem*)),
                    this,SLOT  (serverSelected(QListViewItem*)) );
  connect(serverListView,SIGNAL(itemRenamed(QListViewItem*,const QString&,int)),
                    this,SLOT  (updateServerProperty(QListViewItem*,const QString&,int)) );

  // FIXME: Double click Server Entry in PrefsDialog!
  // This would delete the ListView while inside the doubleClicked() signal and
  // this is not allowed. Delayed Destruct fixes this, but seems to have a race.
  connect(serverListView,SIGNAL(doubleClicked(QListViewItem*)),
                    this,SLOT  (serverDoubleClicked(QListViewItem*)) );
}

PrefsPageServerList::~PrefsPageServerList()
{
}

void PrefsPageServerList::connectClicked()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  // FIXME: Do I really need to cast here? Isn't there a better way?
  // Maybe inherit from serverListView to return proper type
  ServerListItem* item=(ServerListItem*)lv_item;

  if (item)
  {
    // copy the getId() accessor functions return value
    // into an integer variable.
    itemId = item->getId();
    // now we can pass the integer variable as argument for the emitted
    // signal instead of a reference to the locally declared
    // ServerListItems accessor function. [MG]
    emit connectToServer(itemId);
  }
}

void PrefsPageServerList::newServer()
{
  int newId=preferences->addServer("New,new.server.com,6667,,,,");

  ServerListItem* newItem=new ServerListItem(serverListView,newId,"");

  serverListView->setSelected(newItem,true);
  editServer();
}

void PrefsPageServerList::serverSelected(QListViewItem* item)
{
  bool mode=false;

  if(item) mode=true;

  connectButton->setEnabled(mode);
  editServerButton->setEnabled(mode);
  removeServerButton->setEnabled(mode);
}

void PrefsPageServerList::removeServer()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  // FIXME: Do I really need to cast here? Isn't there a better way?
  // Yup, there is, I could override the serverListView to return the correct type
  ServerListItem* item=(ServerListItem*)lv_item;
  if(item)
  {
    preferences->removeServer(item->getId());
    delete item;
  }
}

void PrefsPageServerList::editServer()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  // FIXME: Do I really need to cast here? Isn't there a better way?
  // Yup, there is, I could override the serverListView to return the correct type
  ServerListItem* item=(ServerListItem*)lv_item;
  if(item)
  {
    QString server=preferences->getServerById(item->getId());
    if(server)
    {
      QStringList properties=QStringList::split(',',server,true);
      EditServerDialog editServerDialog(parentFrame,properties[0],properties[1],properties[2],properties[3],properties[4],properties[5]);

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

void PrefsPageServerList::updateServer(const QString& groupName,
                                       const QString& serverName,
                                       const QString& serverPort,
                                       const QString& serverKey,
                                       const QString& channelName,
                                       const QString& channelKey)
{
  QListViewItem* item=serverListView->selectedItems().first();
  // Need to find a better way without casting
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

void PrefsPageServerList::updateServerProperty(QListViewItem* item,const QString& value,int property)
{
  // Need to find a better way without casting
  ServerListItem* serverItem=(ServerListItem*) item;
  int id=serverItem->getId();

  preferences->changeServerProperty(id,property,value);
}

void PrefsPageServerList::serverDoubleClicked(QListViewItem* item)
{
  // Suppress a compiler warning
  item->height();
  connectClicked();
}
