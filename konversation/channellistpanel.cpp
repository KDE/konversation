/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  channellistpanel.cpp  -  Shows the list of channels
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qhbox.h>
#include <qgrid.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qhgroupbox.h>

#include <klineedit.h>
#include <klocale.h>

#include "channellistpanel.h"
#include "server.h"

ChannelListPanel::ChannelListPanel(QWidget* parent) :
                  ChatWindow(parent)
{
  setType(ChatWindow::ChannelList);

  setNumChannels(0);
  setNumUsers(0);

  QHGroupBox* filterGroup=new QHGroupBox(i18n("Filter settings"),this);
  QGrid* mainGrid=new QGrid(2,Qt::Vertical,filterGroup);
  mainGrid->setSpacing(spacing());

  new QLabel(i18n("Minimum users:"),mainGrid);
  new QLabel(i18n("Maximum users:"),mainGrid);
  QSpinBox* minUsersSpin=new QSpinBox(mainGrid,"min_users_spin");
  QSpinBox* maxUsersSpin=new QSpinBox(mainGrid,"max_users_spin");
  
  new QLabel(i18n("Filter pattern:"),mainGrid);
  new QLabel(i18n("Filter target:"),mainGrid);
  
  KLineEdit* filterInput=new KLineEdit(mainGrid,"channel_list_filter_input");

  QHBox* targetBox=new QHBox(mainGrid);
  targetBox->setSpacing(spacing());
  
  QCheckBox* channelFilter=new QCheckBox(i18n("Channel"),targetBox,"filter_target_channel_check");  
  QCheckBox* topicFilter=new QCheckBox(i18n("Topic"),targetBox,"filter_target_topic_check");
  QCheckBox* regexCheck=new QCheckBox(i18n("Regular expression"),targetBox,"regex_check");
  QPushButton* applyFilter=new QPushButton(i18n("Apply filter"),targetBox,"apply_filter_button");     
  
  targetBox->setStretchFactor(topicFilter,10);
  
  channelListView=new KListView(this,"channel_list_view");
  channelListView->addColumn(i18n("Channel name"));
  channelListView->addColumn(i18n("Users"));
  channelListView->addColumn(i18n("Channel topic"));
  channelListView->setFullWidth(true);

  QHBox* actionBox=new QHBox(this);
  actionBox->setSpacing(spacing());
  
  QPushButton* refreshListButton=new QPushButton(i18n("Refresh list"),actionBox,"refresh_list_button");
  QPushButton* saveListButton=new QPushButton(i18n("Save list"),actionBox,"save_list_button");
  QPushButton* joinChannelButton=new QPushButton(i18n("Join channel"),actionBox,"join_channel_button");

  connect(refreshListButton,SIGNAL (clicked()),this,SLOT (refreshList()) );
  connect(saveListButton,SIGNAL (clicked()),this,SLOT (saveList()) );
  connect(joinChannelButton,SIGNAL (clicked()),this,SLOT (joinChannelClicked()) );
}

ChannelListPanel::~ChannelListPanel()
{
}

void ChannelListPanel::refreshList()
{
  channelListView->clear();
  setNumChannels(0);
  setNumUsers(0);
  emit refreshChannelList();
}

void ChannelListPanel::saveList()
{
}

void ChannelListPanel::joinChannelClicked()
{
  QListViewItem* item=channelListView->selectedItem();
  if(item)
  {
    emit joinChannel(item->text(0));
  }
}

void ChannelListPanel::addToChannelList(const QString& channel,int users,const QString& topic)
{
  new KListViewItem(channelListView,channel,QString::number(users),topic);
}

void ChannelListPanel::setNumChannels(int num)
{
  numChannels=num;
  // update widgets here
}

void ChannelListPanel::setNumUsers(int num)
{
  numUsers=num;
  // update widgets here
}

int ChannelListPanel::getNumChannels() { return numChannels; }
int ChannelListPanel::getNumUsers()    { return numUsers; }

void ChannelListPanel::adjustFocus()
{
}

#include "channellistpanel.moc"
