/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageserverlist.cpp  -  Provides a user interface to edit and select servers
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <klistview.h>

#include "prefspageserverlist.h"
#include "preferences.h"
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
  serverListView->addColumn(i18n("Group"));    // 0
  serverListView->addColumn(i18n("Server"));   // 1
  serverListView->addColumn(i18n("Port"));     // 2
  serverListView->addColumn(i18n("Password")); // 3
  serverListView->addColumn(i18n("Channel"));  // 4
  serverListView->addColumn(i18n("Password")); // 5
  serverListView->addColumn(i18n("Identity")); // 6

  serverListView->setRenameable(0,false);
  serverListView->setRenameable(1,true);
  serverListView->setRenameable(2,true);
  serverListView->setRenameable(4,true);

  serverListView->setAllColumnsShowFocus(true);

  // Fill in the servers from the preferences
  int index=0;

  serverListView->setRootIsDecorated(true);

  QString serverString=preferences->getServerByIndex(index);
  while(!serverString.isEmpty())
  {
    int id=preferences->getServerIdByIndex(index);
    QStringList serverEntry=QStringList::split(',',serverString,true);

    QListViewItem* branch=findBranch(serverEntry[0]);
    ServerListItem* item=new ServerListItem(branch,id,
                                            serverEntry[0],  // won't be shown, but stored inside
                                            serverEntry[1],
                                            serverEntry[2],
                                            (!serverEntry[3].isEmpty()) ? QString("********") : QString::null,
                                            serverEntry[4],
                                            (!serverEntry[5].isEmpty()) ? QString("********") : QString::null,
                                            serverEntry[7]);

    item->setOn(serverEntry[6]=="1");

    connect(item,SIGNAL(stateChanged(ServerListItem*,bool)),
            this,SLOT  (updateAutoState(ServerListItem*,bool)) );

    serverString=preferences->getServerByIndex(++index);
  }
  // Set up the button box
  QHBox* buttonBox=new QHBox(parentFrame);
  buttonBox->setSpacing(spacingHint());
  // Add the buttons
  connectButton=new QPushButton(i18n("&Connect"),buttonBox);
  connectButton->setDisabled(true);
  newServerButton=new QPushButton(i18n("&New Server..."),buttonBox);
  editServerButton=new QPushButton(i18n("&Edit..."),buttonBox);
  editServerButton->setDisabled(true);
  removeServerButton=new QPushButton(i18n("&Remove"),buttonBox);
  removeServerButton->setDisabled(true);

  showServerList=new QCheckBox(i18n("&Show server list while autoconnecting"),parentFrame,"show_serverlist_check");
  showServerList->setChecked(preferences->getShowServerList());

  serverListLayout->addWidget(serverListView);
  serverListLayout->addWidget(buttonBox);
  serverListLayout->addWidget(showServerList);

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

  connect(showServerList,SIGNAL (stateChanged(int)),
                    this,SLOT (showServerListChanged(int)) );
}

PrefsPageServerList::~PrefsPageServerList()
{
}

void PrefsPageServerList::connectClicked()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  // FIXME: Do I really need to cast here? Isn't there a better way?
  // Maybe inherit from serverListView to return proper type
  ServerListItem* item=static_cast<ServerListItem*>(lv_item);

  if(item)
  {
    // copy the getId() accessor functions return value
    // into an integer variable.
    itemId=item->getId();
    // now we can pass the integer variable as argument for the emitted
    // signal instead of a reference to the locally declared
    // ServerListItem's accessor function. [MG]
    emit connectToServer(itemId);
  }
}

void PrefsPageServerList::newServer()
{
  int newId=preferences->addServer("New,new.server.com,6667,,,,,");

  QListViewItem* branch=findBranch("New");
  ServerListItem* newItem=new ServerListItem(branch,newId,"New");

  serverListView->setSelected(newItem,true);
  serverListView->ensureItemVisible(newItem);
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
  ServerListItem* item=static_cast<ServerListItem*>(lv_item);
  if(item)
  {
    // find branch this item belongs to
    QListViewItem* branch=findBranch(item->getGroup());
    // remove server from preferences
    preferences->removeServer(item->getId());
    // remove item from view
    delete item;
    // if the branch has no other items, remove it
    if(branch->childCount()==0) delete branch;
  }
}

void PrefsPageServerList::editServer()
{
  QListViewItem* lv_item=serverListView->selectedItems().first();
  // FIXME: Do I really need to cast here? Isn't there a better way?
  // Yup, there is, I could override the serverListView to return the correct type
  ServerListItem* item=static_cast<ServerListItem*>(lv_item);
  if(item)
  {
    QString server=preferences->getServerById(item->getId());
    if(!server.isEmpty())
    {
      QStringList properties=QStringList::split(',',server,true);
      EditServerDialog editServerDialog(parentFrame,properties[0],properties[1],properties[2],properties[3],properties[4],properties[5],properties[7]);

      connect(&editServerDialog,SIGNAL (serverChanged(const QString&,
                                                      const QString&,
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
                                       const QString& channelKey,
                                       const QString& identity)
{
  // Need to find a better way without casting
  ServerListItem* item=static_cast<ServerListItem*>(serverListView->selectedItems().first());
  // save state of autoconnect checkbox
  bool autoConnect = item->isOn();
  // find branch the old item resides in
  QListViewItem* branch=findBranch(item->getGroup());
  // save server id of the old item
  int id=item->getId();
  // remove item from the list
  delete item;
  // if the branch is empty, remove it
  if(branch->childCount()==0) delete branch;

  // find branch to insert the new item into
  branch=findBranch(groupName);

  item=new ServerListItem(branch,
                          id,
                          groupName,
                          serverName,
                          serverPort,
                          (!serverKey || serverKey.isEmpty()) ? QString::null : QString("********"),
                          channelName,
                          (!channelKey || channelKey.isEmpty()) ? QString::null : QString("********"),
                          identity);

  item->setOn(autoConnect);

  preferences->updateServer(id,groupName+","+
                               serverName+","+
                               serverPort+","+
                               serverKey+","+
                               channelName+","+
                               channelKey+","+
                               (item->isOn() ? "1" : "0")+","+
                               identity);

  serverListView->setSelected(item,true);
  serverListView->ensureItemVisible(item);
}

void PrefsPageServerList::updateServerProperty(QListViewItem* item,const QString& value,int property)
{
  // Need to find a better way without casting
  ServerListItem* serverItem=static_cast<ServerListItem*>(item);
  int id=serverItem->getId();

  if(property==0) property=6;  // to keep old preferences working with auto connect checkbox

// No longer needed, since the group property was moved out
//  else property--;             // -1 because the first is the checkbox

  preferences->changeServerProperty(id,property,value);
}

void PrefsPageServerList::updateAutoState(ServerListItem* item,bool state)
{
  updateServerProperty(item,(state ? "1" : "0"),0);
}

void PrefsPageServerList::serverDoubleClicked(QListViewItem* item)
{
  if(item->text(1).isEmpty()) item->setOpen(!item->isOpen());
  else connectClicked();
}

void PrefsPageServerList::showServerListChanged(int state)
{
  preferences->setShowServerList(state==2);
}

QListViewItem* PrefsPageServerList::findBranch(QString name,bool generate)
{
  QListViewItem* branch=serverListView->findItem(name,0);
  if(branch==0 && generate==true)
  {
    branch=new QListViewItem(serverListView,name);
    branch->setOpen(true);
    branch->setSelectable(false);
  }

  return branch;
}

#include "prefspageserverlist.moc"
