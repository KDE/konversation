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
#include <qpushbutton.h>
#include <qhgroupbox.h>
#include <qregexp.h>

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>

#include "channellistpanel.h"
#include "server.h"

ChannelListPanel::ChannelListPanel(QWidget* parent) :
                  ChatWindow(parent)
{
  setType(ChatWindow::ChannelList);

  setNumChannels(0);
  setNumUsers(0);

  setMinUsers(0);
  setMaxUsers(0);

  setChannelTarget(true);
  setTopicTarget(false);
  setRegExp(false);

  filterTextChanged(QString::null);

  QHGroupBox* filterGroup=new QHGroupBox(i18n("Filter settings"),this);
  QGrid* mainGrid=new QGrid(2,Qt::Vertical,filterGroup);
  mainGrid->setSpacing(spacing());

  new QLabel(i18n("Minimum users:"),mainGrid);
  new QLabel(i18n("Maximum users:"),mainGrid);
  QSpinBox* minUsersSpin=new QSpinBox(mainGrid,"min_users_spin");
  QSpinBox* maxUsersSpin=new QSpinBox(mainGrid,"max_users_spin");
  minUsersSpin->setValue(getMinUsers());
  maxUsersSpin->setValue(getMaxUsers());

  new QLabel(i18n("Filter pattern:"),mainGrid);
  new QLabel(i18n("Filter target:"),mainGrid);

  KLineEdit* filterInput=new KLineEdit(mainGrid,"channel_list_filter_input");
  filterInput->setText(getFilterText());

  QHBox* targetBox=new QHBox(mainGrid);
  targetBox->setSpacing(spacing());

  channelFilter=new QCheckBox(i18n("Channel"),targetBox,"filter_target_channel_check");
  topicFilter=new QCheckBox(i18n("Topic"),targetBox,"filter_target_topic_check");
  regexpCheck=new QCheckBox(i18n("Regular expression"),targetBox,"regexp_check");
  QPushButton* applyFilter=new QPushButton(i18n("Apply filter"),targetBox,"apply_filter_button");

  channelFilter->setChecked(getChannelTarget());
  topicFilter->setChecked(getTopicTarget());
  regexpCheck->setChecked(getRegExp());

  targetBox->setStretchFactor(topicFilter,10);

  channelListView=new KListView(this,"channel_list_view");
  channelListView->addColumn(i18n("Channel name"));
  channelListView->addColumn(i18n("Users"));
  channelListView->addColumn(i18n("Channel topic"));
  channelListView->setFullWidth(true);
  channelListView->setAllColumnsShowFocus(true);

  QHBox* actionBox=new QHBox(this);
  actionBox->setSpacing(spacing());

  QPushButton* refreshListButton=new QPushButton(i18n("Refresh list"),actionBox,"refresh_list_button");
  QPushButton* saveListButton=new QPushButton(i18n("Save list"),actionBox,"save_list_button");
  QPushButton* joinChannelButton=new QPushButton(i18n("Join channel"),actionBox,"join_channel_button");

  connect(minUsersSpin,SIGNAL (valueChanged(int)),this,SLOT(setMinUsers(int)) );
  connect(maxUsersSpin,SIGNAL (valueChanged(int)),this,SLOT(setMaxUsers(int)) );
  connect(this,SIGNAL (adjustMinValue(int)),minUsersSpin,SLOT (setValue(int)) );
  connect(this,SIGNAL (adjustMaxValue(int)),maxUsersSpin,SLOT (setValue(int)) );

  connect(filterInput,SIGNAL (textChanged(const QString&)),this,SLOT (filterTextChanged(const QString&)) );
  connect(filterInput,SIGNAL (returnPressed()),this,SLOT (applyFilterClicked()) );

  connect(channelFilter,SIGNAL (clicked()),this,SLOT (channelTargetClicked()) );
  connect(topicFilter,SIGNAL (clicked()),this,SLOT (topicTargetClicked()) );
  connect(regexpCheck,SIGNAL (clicked()),this,SLOT (regExpClicked()) );

  connect(applyFilter,SIGNAL (clicked()),this,SLOT (applyFilterClicked()) );

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
  channelList.clear();

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
  channelList.append(channel+" "+QString::number(users)+" "+topic);

  setNumChannels(getNumChannels()+1);
  setNumUsers(getNumUsers()+users);
}

void ChannelListPanel::filterTextChanged(const QString& newText)
{
  filterText=newText;
}

int ChannelListPanel::getNumChannels() { return numChannels; }
int ChannelListPanel::getNumUsers()    { return numUsers; }

int ChannelListPanel::getMinUsers()    { return minUsers; }
int ChannelListPanel::getMaxUsers()    { return maxUsers; }

bool ChannelListPanel::getChannelTarget() { return channelTarget; }
bool ChannelListPanel::getTopicTarget()   { return topicTarget; }
bool ChannelListPanel::getRegExp()        { return regExp; }

const QString& ChannelListPanel::getFilterText() { return filterText; }

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

void ChannelListPanel::setMinUsers(int num)
{
  minUsers=num;
  if(minUsers>maxUsers) adjustMaxValue(num);
}

void ChannelListPanel::setMaxUsers(int num)
{
  maxUsers=num;
  if(maxUsers<minUsers) adjustMinValue(num);
}

void ChannelListPanel::setChannelTarget(bool state)  { channelTarget=state; }
void ChannelListPanel::setTopicTarget(bool state)    { topicTarget=state; }

void ChannelListPanel::setRegExp(bool state)         { regExp=state; }

void ChannelListPanel::channelTargetClicked()        { setChannelTarget(channelFilter->state()==2); }
void ChannelListPanel::topicTargetClicked()          { setTopicTarget(topicFilter->state()==2); }
void ChannelListPanel::regExpClicked()               { setRegExp(regexpCheck->state()==2); }

void ChannelListPanel::applyFilterClicked()
{
  unsigned int index=0;

  QListViewItem* item=channelListView->itemAtIndex(0);

  while(item)
  {
    bool visible=true;

    if(getMaxUsers())
    {
      if(item->text(1).toInt()<getMinUsers() ||
         item->text(1).toInt()>getMaxUsers() ) visible=false;
    }

    if(!getFilterText().isEmpty())
    {
      if(getChannelTarget())
      {
        if(item->text(0).find(QRegExp(getFilterText(),false,!getRegExp()))==-1) visible=false;
      }

      if(getTopicTarget())
      {
        if(item->text(2).find(QRegExp(getFilterText(),false,!getRegExp()))==-1) visible=false;
      }
    }

    item->setVisible(visible);

    item=channelListView->itemAtIndex(++index);
  }
}

void ChannelListPanel::closeYourself()
{
  // make the server delete us so server can reset the pointer to us
  kdDebug() << "ChannelListPanel::closeYourself()" << endl;
  server->closeChannelListPanel();
}

void ChannelListPanel::adjustFocus()
{
}

#include "channellistpanel.moc"
