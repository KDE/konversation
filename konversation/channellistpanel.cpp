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

ChannelListPanel::ChannelListPanel(QWidget* parent) :
                  ChatWindow(parent)
{
  setType(ChatWindow::ChannelList);

  QHGroupBox* filterGroup=new QHGroupBox(i18n("Filter settings"),this);
  QGrid* mainGrid=new QGrid(2,Qt::Vertical,filterGroup);
  mainGrid->setSpacing(spacing());

  new QLabel(i18n("Minimum users:"),mainGrid);
  new QLabel(i18n("Maximum users:"),mainGrid);
  QSpinBox* minUsersSpin=new QSpinBox(mainGrid);
  QSpinBox* maxUsersSpin=new QSpinBox(mainGrid);
  
  new QLabel(i18n("Filter pattern:"),mainGrid);
  new QLabel(i18n("Filter target:"),mainGrid);
  
  KLineEdit* filterInput=new KLineEdit(mainGrid);

  QHBox* targetBox=new QHBox(mainGrid);
  targetBox->setSpacing(spacing());
  
  QCheckBox* channelFilter=new QCheckBox(i18n("Channel"),targetBox);  
  QCheckBox* topicFilter=new QCheckBox(i18n("Topic"),targetBox);
  QPushButton* applyFilter=new QPushButton(i18n("Apply filter"),targetBox);     
  
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
}

ChannelListPanel::~ChannelListPanel()
{
}

void ChannelListPanel::adjustFocus()
{
}
