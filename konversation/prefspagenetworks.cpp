/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircnetworks.cpp  -  Provides classes for loading IRC Network settings
  begin:     Mon Jun 21 2004
  copyright: (C) by Piotr Szymanski
  email:     djurban@pld-linux.org
*/

#include "ircnetworks.h"
#include "preferences.h"
#include <qlayout.h>
#include <qhbox.h>

// Begin layouting, first separate the network's listbox from the editing parts

PrefsPageNetworks::PrefsPageNetworks(QFrame* newParent,Preferences* newPreferences) :
                     PrefsPage(newParent,newPreferences)
{
QHBoxLayout * masterLayout=new QHBoxLayout(parentFrame,marginHint(),spacingHint(),"main_layout");
QVBoxLayout * leftLayout=new QVBoxLayout(masterLayout, spacingHint(),"left_layout");
QHBoxLayout * leftDownLayout=new QHBoxLayout(parentFrame,marginHint(),spacingHint(),"left_down_layout");



QListbox * networks=new QListBox(this,"network_listbox");
QPushButton netAdd = new QPushButton (QString(i18n("Add")),this,"addNetwork_button");
QPushButton netDel = new QPushButton (QString(i18n("Remove")),this,"delNetwork_button");

leftDownLayout->addWidget(netAdd);
leftDownLayout->addStrech(1);
leftDownLayout->addWidget(netDel);
leftLayout->addWidget(networks);
leftLayout->addLayout(leftDownLayout);


QVBoxLayout * rightLayout=new QVBoxLayout(masterLayout, spacingHint(),"right_layout");
QGridLayout * netPrefs = new QGridLayout (3,2,spacingHint());

QLineEdit * netName = new QLineEdit(this,"network name");
QLabel * netNameL = new QLabel (netName,i18n("Name:"));

QLineEdit * netDesc = new QLineEdit(this,"network name");
QLabel * netDescL = new QLabel (netDesc,i18n("Description:"));

QComboBox * netId = new QComboBox(false, this,"network name");
QLabel * netIdL = new QLabel (netId,i18n("Identity:"));

QHBoxLayout * rightMiddleLayout = new QHBoxLayout(parentFrame,marginHint(),spacingHint(),"righ_middle_layout");
QListbox * hosts=new QListBox(this,"host_listbox");

QVBoxLayout * rightMiddleRightLayout=new QVBoxLayout(parentFrame, spacingHint(),"right_middle_right_layout");
QPushButton * hostUp = new QPushButton(QString(i18n("Up")),this,"upHost_button");
QPushButton * hostDown = new QPushButton(QString(i18n("Down")),this,"downHost_button");
QPushButton * hostAdd = new QPushButton(QString(i18n("Add")),this,"addHost_button");
QPushButton * hostDel = new QPushButton(QString(i18n("Remove")),this,"delHost_button");

QGroupBox * hostPrefs =  new QGroupBox (this,"host_prefs_groupbox");
hostPrefs->setTitle(QString("Host settings");



QLabel * hostAddrL = new QLabel (hostAddr,i18n("Identity:"),hostPrefs);
QLabel * hostPassL = new QLabel (hostPass,i18n("Identity:"),hostPrefs);
QSpinBox * hostPort = new QSpinBox (hostPrefs,"host_port");
hostPort->prefix(i18n("Port:"));
hostPort->setMinValue(0);
hostPort->setLineStep(1);
hostPort->setValue(6667);
QCheckBox * hostSSL = new QCheckBox(i18n("Use SSL"),hostPrefs);
QCheckBox * hostIdc = new QCheckBox(i18n("Use own identity: "),hostPrefs);
QComboBox * hostId = new QComboBox(false, hostPrefs,"network name");



//QHBoxLayout * rightMiddleLayout = new QHBoxLayout(hostPrefs,marginHint(),spacingHint(),"right_lower_layout");

netPrefs->addItem(netNameL,1,1);
netPrefs->addItem(netName,1,2);
netPrefs->addItem(netDescL,2,1);
netPrefs->addItem(netDesc,2,2);
netPrefs->addItem(netIdL,3,1);
netPrefs->addItem(netId,3,2);

rightMiddleRightLayout->(hostUp);
rightMiddleRightLayout->(hostDown);
rightMiddleRightLayout->(hostAdd);
rightMiddleRightLayout->(hostDel);

rightMiddleLayout->addWidget(hosts);
rightMiddleLayout->addLayout(rightMiddleRightLayout);


rightLayout->addLayout(netPrefs);
rightLayout->addLayout(rightMiddleLayout);
rightLayout->addWidget(hostPrefs);
}
