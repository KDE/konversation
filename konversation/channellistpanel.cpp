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
*/

#include <qhbox.h>
#include <qvbox.h>
#include <qgrid.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qhgroupbox.h>
#include <qregexp.h>
#include <qcheckbox.h>
#include <qtimer.h>

#include <klistview.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include "channellistpanel.h"
#include "channellistviewitem.h"
#include "server.h"

ChannelListPanel::ChannelListPanel(QWidget* parent) :
                  ChatWindow(parent)
{
  setType(ChatWindow::ChannelList);

  setNumChannels(0);
  setNumUsers(0);
  setVisibleChannels(0);
  setVisibleUsers(0);

  setMinUsers(0);
  setMaxUsers(0);

  setChannelTarget(true);
  setTopicTarget(false);
  setRegExp(false);

  filterTextChanged(QString::null);

  QHGroupBox* filterGroup=new QHGroupBox(i18n("Filter Settings"),this);
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
  QPushButton* applyFilter=new QPushButton(i18n("Apply Filter"),targetBox,"apply_filter_button");

  channelFilter->setChecked(getChannelTarget());
  topicFilter->setChecked(getTopicTarget());
  regexpCheck->setChecked(getRegExp());

  targetBox->setStretchFactor(topicFilter,10);

  channelListView=new KListView(this,"channel_list_view");
  channelListView->addColumn(i18n("Channel Name"));
  channelListView->addColumn(i18n("Users"));
  channelListView->addColumn(i18n("Channel Topic"));
  channelListView->setFullWidth(true);
  channelListView->setAllColumnsShowFocus(true);

  QHBox* statsBox=new QHBox(this);
  statsBox->setSpacing(spacing());

  QLabel* usersLabel=new QLabel(QString::null,statsBox);
  QLabel* channelsLabel=new QLabel(QString::null,statsBox);

  statsBox->setStretchFactor(channelsLabel,10);

  QHBox* actionBox=new QHBox(this);
  actionBox->setSpacing(spacing());

  QPushButton* refreshListButton=new QPushButton(i18n("Refresh List"),actionBox,"refresh_list_button");
  QPushButton* saveListButton=new QPushButton(i18n("Save List..."),actionBox,"save_list_button");
  QPushButton* joinChannelButton=new QPushButton(i18n("Join Channel"),actionBox,"join_channel_button");

  // update list view every 0.5 seconds
  updateTimer.start(500);
  connect(&updateTimer,SIGNAL (timeout()),this,SLOT (updateDisplay()));

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

  connect(this,SIGNAL (updateNumUsers(const QString&)),usersLabel,SLOT (setText(const QString&)) );
  connect(this,SIGNAL (updateNumChannels(const QString&)),channelsLabel,SLOT (setText(const QString&)) );

  updateUsersChannels();
  KMessageBox::information(this,i18n("Warning! Using this function may result in a lot "
                                     "of network traffic. If your connection is not fast "
                                     "enough, it's possible that your client gets "
                                     "disconnected by the server!"),
                                i18n("Channel List Warning"),"ChannelListWarning");
}

ChannelListPanel::~ChannelListPanel()
{
}

void ChannelListPanel::refreshList()
{
  channelListView->clear();

  setNumChannels(0);
  setNumUsers(0);
  setVisibleChannels(0);
  setVisibleUsers(0);

  updateUsersChannels();
  emit refreshChannelList();
}

void ChannelListPanel::saveList()
{
  // Ask user for file name
  QString fileName=KFileDialog::getSaveFileName(
                                                 QString::null,
                                                 QString::null,
                                                 this,
                                                 i18n("Save Channel List"));

  if(!fileName.isEmpty())
  {
    // first find the longest channel name and nick number for clean table layouting
    unsigned int index=0;
    unsigned int maxChannelWidth=0;
    unsigned int maxNicksWidth=0;

    QListViewItem* item=channelListView->itemAtIndex(0);
    while(item)
    {
      if(item->isVisible())
      {
        if(item->text(0).length()>maxChannelWidth) maxChannelWidth=item->text(0).length();
        if(item->text(1).length()>maxNicksWidth) maxNicksWidth=item->text(1).length();
      }
      item=channelListView->itemAtIndex(++index);
    }

    // now save the list to disk
    QFile listFile(fileName);
    listFile.open(IO_WriteOnly);
    // wrap the file into a stream
    QTextStream stream(&listFile);

    QString header(i18n("Konversation Channel List: %1 - %2\n\n")
                         .arg(server->getServerName())
                         .arg(QDateTime::currentDateTime().toString()));

    // send header to stream
    stream << header;

    index=0;
    item=channelListView->itemAtIndex(0);
    while(item)
    {
      if(item->isVisible())
      {
        QString channelName;
        channelName.fill(' ',maxChannelWidth);
        channelName.replace(0,item->text(0).length(),item->text(0));

        QString nicksPad;
        nicksPad.fill(' ',maxNicksWidth);
        QString nicksNum(nicksPad+item->text(1));
        nicksNum=nicksNum.right(maxNicksWidth);

        QString line(channelName+" "+nicksNum+" "+item->text(2)+"\n");

        // send final line to stream
        stream << line;
      }
      item=channelListView->itemAtIndex(++index);
    }

    listFile.close();
  }
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
  pendingChannels.append(channel+" "+QString::number(users)+" "+topic);

  // set internal numbers of channels and users, display will be updated by a timer
  setNumChannels(getNumChannels()+1);
  setNumUsers(getNumUsers()+users);

  // no filter yet, so set visible value to the same value
  setVisibleChannels(getNumChannels());
  setVisibleUsers(getNumUsers());
}

void ChannelListPanel::updateDisplay()
{
  if(pendingChannels.count())
  {
    // stop list view from updating
    channelListView->setUpdatesEnabled(false);

    // list view changes are done inside the loop, since QT can't update the
    // widget afterwards ...
    for(unsigned int index=0;index<pendingChannels.count();index++)
    {
      // fetch next channel line
      QString channelLine=pendingChannels[index];
      // split it up into the single parts we need
      QString channel=channelLine.section(' ',0,0);
      QString users=channelLine.section(' ',1,1);
      QString topic=channelLine.section(' ',2);
      // if it's the last one of this batch, update the widget
      if(index==pendingChannels.count()-1) channelListView->setUpdatesEnabled(true);
      // add channel line to list view
      new ChannelListViewItem(channelListView,channel,users,topic);
    }
    // clear list of pending inserts
    pendingChannels.clear();
    // update display
    updateUsersChannels();
  }
}

void ChannelListPanel::filterTextChanged(const QString& newText)
{
  filterText=newText;
}

int ChannelListPanel::getNumChannels()     { return numChannels; }
int ChannelListPanel::getNumUsers()        { return numUsers; }

void ChannelListPanel::setNumChannels(int num) { numChannels=num; }
void ChannelListPanel::setNumUsers(int num)    { numUsers=num; }

int ChannelListPanel::getVisibleChannels() { return visibleChannels; }
int ChannelListPanel::getVisibleUsers()    { return visibleUsers; }

void ChannelListPanel::setVisibleChannels(int num) { visibleChannels=num; }
void ChannelListPanel::setVisibleUsers(int num)    { visibleUsers=num; }

int ChannelListPanel::getMinUsers()    { return minUsers; }
int ChannelListPanel::getMaxUsers()    { return maxUsers; }

bool ChannelListPanel::getChannelTarget() { return channelTarget; }
bool ChannelListPanel::getTopicTarget()   { return topicTarget; }
bool ChannelListPanel::getRegExp()        { return regExp; }

const QString& ChannelListPanel::getFilterText() { return filterText; }

void ChannelListPanel::setMinUsers(int num)
{
  minUsers=num;
}

void ChannelListPanel::setMaxUsers(int num)
{
  maxUsers=num;
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

  setVisibleChannels(0);
  setVisibleUsers(0);

  while(item)
  {
    bool visible=true;

    if(getMinUsers() || getMaxUsers())
    {
      if(item->text(1).toInt()<getMinUsers() ||
          (getMaxUsers()>=getMinUsers() &&
           item->text(1).toInt()>getMaxUsers())) visible=false;
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
    if(visible)
    {
      setVisibleUsers(getVisibleUsers()+item->text(1).toInt());
      setVisibleChannels(getVisibleChannels()+1);
    }

    item=channelListView->itemAtIndex(++index);
  }

  updateUsersChannels();
}

void ChannelListPanel::updateUsersChannels()
{
  emit updateNumChannels(i18n("Channels: %1 (%2 shown)").arg(getNumChannels()).arg(getVisibleChannels()));
  emit updateNumUsers(i18n("Users: %1 (%2 shown)").arg(getNumUsers()).arg(getVisibleUsers()));
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
