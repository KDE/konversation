/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#include "servergroupdialog.h"

#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>

#include <klocale.h>
#include <kiconloader.h>

#include "identity.h"
#include "konversationapplication.h"
#include "preferences.h"
#include "serversettings.h"
#include "serverdialog.h"
#include "channeldialog.h"

namespace Konversation {

ServerGroupDialog::ServerGroupDialog(const QString& title, QWidget *parent, const char *name)
 : KDialogBase(Plain, title, Ok|Cancel, Ok, parent, name)
{
  m_id = -1;
  
  QFrame* mainWidget = plainPage();
  QGridLayout* mainLayout = new QGridLayout(mainWidget, 1, 2, 0, spacingHint());
  mainLayout->setColStretch(1, 10);
  
  QLabel* nameLbl = new QLabel(i18n("&Network:"), mainWidget);
  m_nameEdit = new QLineEdit(mainWidget);
  nameLbl->setBuddy(m_nameEdit);
  
  QLabel* groupLbl = new QLabel(i18n("&Group:"), mainWidget);
  m_groupCBox = new QComboBox(true, mainWidget);
  groupLbl->setBuddy(m_groupCBox);
  
  QLabel* identityLbl = new QLabel(i18n("&Identity:"), mainWidget);
  m_identityCBox = new QComboBox(mainWidget);
  identityLbl->setBuddy(m_identityCBox);
  
  QValueList<IdentityPtr> identities = KonversationApplication::preferences.getIdentityList();

  for(QValueList<IdentityPtr>::iterator it = identities.begin(); it != identities.end(); ++it)
  {
    m_identityCBox->insertItem((*it)->getName());
  }
  
  QLabel* commandLbl = new QLabel(i18n("Co&mmands:"), mainWidget);
  m_commandEdit = new QLineEdit(mainWidget);
  commandLbl->setBuddy(m_commandEdit);
  
  m_autoConnectCBox = new QCheckBox(i18n("Connect on &application start up."), mainWidget);
  
  QWidget* groupWidget = new QWidget(mainWidget);
  QGridLayout* groupLayout = new QGridLayout(groupWidget, 1, 2, 0, spacingHint());
  
  QGroupBox* serverGBox = new QGroupBox(0, Qt::Horizontal, i18n("Servers"), groupWidget);
  serverGBox->setMargin(marginHint());
  QGridLayout* serverLayout = new QGridLayout(serverGBox->layout(), 1, 2, spacingHint());
  
  m_serverLBox = new QListBox(serverGBox);
  QPushButton* addServerBtn = new QPushButton(i18n("Add..."), serverGBox);
  QPushButton* changeServerBtn = new QPushButton(i18n("Edit..."), serverGBox);
  QPushButton* removeServerBtn = new QPushButton(i18n("Delete"), serverGBox);
  QToolButton* upServerBtn = new QToolButton(serverGBox);
  upServerBtn->setIconSet(SmallIconSet("up"));
  upServerBtn->setAutoRepeat(true);
  QToolButton* downServerBtn = new QToolButton(serverGBox);
  downServerBtn->setIconSet(SmallIconSet("down"));
  downServerBtn->setAutoRepeat(true);
  
  connect(addServerBtn, SIGNAL(clicked()), this, SLOT(addServer()));
  connect(changeServerBtn, SIGNAL(clicked()), this, SLOT(editServer()));
  connect(removeServerBtn, SIGNAL(clicked()), this, SLOT(deleteServer()));
  connect(upServerBtn, SIGNAL(clicked()), this, SLOT(moveServerUp()));
  connect(downServerBtn, SIGNAL(clicked()), this, SLOT(moveServerDown()));
  
  serverLayout->setColStretch(0, 10);
  serverLayout->setRowStretch(4, 10);
  serverLayout->addMultiCellWidget(m_serverLBox, 0, 4, 0, 0);
  serverLayout->addMultiCellWidget(addServerBtn, 0, 0, 1, 4);
  serverLayout->addMultiCellWidget(changeServerBtn, 1, 1, 1, 4);
  serverLayout->addMultiCellWidget(removeServerBtn, 2, 2, 1, 4);
  serverLayout->addWidget(upServerBtn, 3, 2);
  serverLayout->addWidget(downServerBtn, 3, 3);

  QGroupBox* channelGBox = new QGroupBox(0, Qt::Horizontal, i18n("Auto Join Channels"), groupWidget);
  channelGBox->setMargin(marginHint());
  QGridLayout* channelLayout = new QGridLayout(channelGBox->layout(), 1, 2, spacingHint());
  
  m_channelLBox = new QListBox(channelGBox);
  QPushButton* addChannelBtn = new QPushButton(i18n("Add..."), channelGBox);
  QPushButton* changeChannelBtn = new QPushButton(i18n("Edit..."), channelGBox);
  QPushButton* removeChannelBtn = new QPushButton(i18n("Delete"), channelGBox);
  QToolButton* upChannelBtn = new QToolButton(channelGBox);
  upChannelBtn->setIconSet(SmallIconSet("up"));
  upChannelBtn->setAutoRepeat(true);
  QToolButton* downChannelBtn = new QToolButton(channelGBox);
  downChannelBtn->setIconSet(SmallIconSet("down"));
  downChannelBtn->setAutoRepeat(true);
  
  connect(addChannelBtn, SIGNAL(clicked()), this, SLOT(addChannel()));
  connect(changeChannelBtn, SIGNAL(clicked()), this, SLOT(editChannel()));
  connect(removeChannelBtn, SIGNAL(clicked()), this, SLOT(deleteChannel()));
  connect(upChannelBtn, SIGNAL(clicked()), this, SLOT(moveChannelUp()));
  connect(downChannelBtn, SIGNAL(clicked()), this, SLOT(moveChannelDown()));
  
  channelLayout->setColStretch(0, 10);
  channelLayout->setRowStretch(4, 10);
  channelLayout->addMultiCellWidget(m_channelLBox, 0, 4, 0, 0);
  channelLayout->addMultiCellWidget(addChannelBtn, 0, 0, 1, 4);
  channelLayout->addMultiCellWidget(changeChannelBtn, 1, 1, 1, 4);
  channelLayout->addMultiCellWidget(removeChannelBtn, 2, 2, 1, 4);
  channelLayout->addWidget(upChannelBtn, 3, 2);
  channelLayout->addWidget(downChannelBtn, 3, 3);
  
  mainLayout->addWidget(nameLbl, 0, 0);
  mainLayout->addWidget(m_nameEdit, 0, 1);
  mainLayout->addWidget(groupLbl, 1, 0);
  mainLayout->addWidget(m_groupCBox, 1, 1);
  mainLayout->addWidget(identityLbl, 2, 0);
  mainLayout->addWidget(m_identityCBox, 2, 1);
  mainLayout->addWidget(commandLbl, 3, 0);
  mainLayout->addWidget(m_commandEdit, 3, 1);
  mainLayout->addMultiCellWidget(m_autoConnectCBox, 4, 4, 0, 1);
  mainLayout->addMultiCellWidget(groupWidget, 5, 5, 0, 1);
  groupLayout->addWidget(serverGBox, 0, 0);
  groupLayout->addWidget(channelGBox, 0, 1);

  setButtonOK(KGuiItem(i18n("&OK"),"button_ok",i18n("Change server information")));
  setButtonCancel(KGuiItem(i18n("&Cancel"),"button_cancel",i18n("Discards all changes made")));
  
  m_nameEdit->setFocus();
}

ServerGroupDialog::~ServerGroupDialog()
{
}

void ServerGroupDialog::setServerGroupSettings(const ServerGroupSettings& settings)
{
  m_id = settings.id();
  m_nameEdit->setText(settings.name());
  m_groupCBox->setCurrentText(settings.group());
  m_identityCBox->setCurrentText(settings.identity()->getName());
  m_commandEdit->setText(settings.connectCommands());
  m_autoConnectCBox->setChecked(settings.autoConnectEnabled());
  m_serverList = settings.serverList();
  ServerList::iterator it;
  m_serverLBox->clear();

  for(it = m_serverList.begin(); it != m_serverList.end(); ++it) {
    m_serverLBox->insertItem((*it).server());
  }
  
  m_channelList = settings.channelList();
  ChannelList::iterator it2;
  
  for(it2 = m_channelList.begin(); it2 != m_channelList.end(); ++it2) {
    m_channelLBox->insertItem((*it2).name());
  }
}

ServerGroupSettings ServerGroupDialog::serverGroupSettings()
{
  ServerGroupSettings settings(m_id);
  settings.setName(m_nameEdit->text());
  settings.setGroup(m_groupCBox->currentText());
  QValueList<IdentityPtr> identities = KonversationApplication::preferences.getIdentityList();
  settings.setIdentityId(identities[m_identityCBox->currentItem()]->id());
  settings.setConnectCommands(m_commandEdit->text());
  settings.setAutoConnectEnabled(m_autoConnectCBox->isChecked());
  settings.setServerList(m_serverList);
  settings.setChannelList(m_channelList);

  return settings;
}

void ServerGroupDialog::setAvailableGroups(const QStringList& groups)
{
  m_groupCBox->clear();
  m_groupCBox->insertStringList(groups, 1);
}

void ServerGroupDialog::addServer()
{
  ServerDialog dlg(i18n("Add Server"), this);
  
  if(dlg.exec() == KDialog::Accepted) {
    ServerSettings server = dlg.serverSettings();
    m_serverLBox->insertItem(server.server());
    m_serverList.append(server);
  }
}

void ServerGroupDialog::editServer()
{
  uint current = m_serverLBox->currentItem();
  
  if(current < m_serverList.count()) {
    ServerDialog dlg(i18n("Edit Server"), this);
    dlg.setServerSettings(m_serverList[current]);

    if(dlg.exec() == KDialog::Accepted) {
      ServerSettings server = dlg.serverSettings();
      m_serverLBox->changeItem(server.server(), current);
      m_serverList[current] = server;
    }
  }
}

void ServerGroupDialog::deleteServer()
{
  uint current = m_serverLBox->currentItem();
  
  if(current < m_serverList.count()) {
    m_serverList.remove(m_serverList.at(current));
    m_serverLBox->removeItem(current);
  }
}

void ServerGroupDialog::moveServerUp()
{
  uint current = m_serverLBox->currentItem();

  if(current > 0) {
    ServerSettings server = m_serverList[current];
    m_serverLBox->removeItem(current);
    m_serverLBox->insertItem(server.server(), current - 1);
    m_serverLBox->setCurrentItem(current - 1);
    ServerList::iterator it = m_serverList.remove(m_serverList.at(current));
    --it;
    m_serverList.insert(it, server);
  }
}

void ServerGroupDialog::moveServerDown()
{
  uint current = m_serverLBox->currentItem();

  if(current < (m_serverList.count() - 1)) {
    ServerSettings server = m_serverList[current];
    m_serverLBox->removeItem(current);
    m_serverLBox->insertItem(server.server(), current + 1);
    m_serverLBox->setCurrentItem(current + 1);
    ServerList::iterator it = m_serverList.remove(m_serverList.at(current));
    ++it;
    m_serverList.insert(it, server);
  }
}

void ServerGroupDialog::addChannel()
{
  ChannelDialog dlg(i18n("Add Channel"), this);

  if(dlg.exec() == KDialog::Accepted) {
    ChannelSettings channel = dlg.channelSettings();
    m_channelLBox->insertItem(channel.name());
    m_channelList.append(channel);
  }
}

void ServerGroupDialog::editChannel()
{
  uint current = m_channelLBox->currentItem();

  if(current < m_channelList.count()) {
    ChannelDialog dlg(i18n("Edit Channel"), this);
    dlg.setChannelSettings(m_channelList[current]);

    if(dlg.exec() == KDialog::Accepted) {
      ChannelSettings channel = dlg.channelSettings();
      m_channelLBox->changeItem(channel.name(), current);
      m_channelList[current] = channel;
    }
  }
}

void ServerGroupDialog::deleteChannel()
{
  uint current = m_channelLBox->currentItem();

  if(current < m_channelList.count()) {
    m_channelList.remove(m_channelList.at(current));
    m_channelLBox->removeItem(current);
  }
}

void ServerGroupDialog::moveChannelUp()
{
  uint current = m_channelLBox->currentItem();

  if(current > 0) {
    ChannelSettings channel = m_channelList[current];
    m_channelLBox->removeItem(current);
    m_channelLBox->insertItem(channel.name(), current - 1);
    m_channelLBox->setCurrentItem(current - 1);
    ChannelList::iterator it = m_channelList.remove(m_channelList.at(current));
    --it;
    m_channelList.insert(it, channel);
  }
}

void ServerGroupDialog::moveChannelDown()
{
  uint current = m_channelLBox->currentItem();

  if(current < (m_channelList.count() - 1)) {
    ChannelSettings channel = m_channelList[current];
    m_channelLBox->removeItem(current);
    m_channelLBox->insertItem(channel.name(), current + 1);
    m_channelLBox->setCurrentItem(current + 1);
    ChannelList::iterator it = m_channelList.remove(m_channelList.at(current));
    ++it;
    m_channelList.insert(it, channel);
  }
}

};

#include "servergroupdialog.moc"
