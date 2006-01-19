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
#include <qwhatsthis.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>

#include "identity.h"
#include "konversationapplication.h"
#include "preferences.h"
#include "serversettings.h"
#include "serverdialog.h"
#include "channeldialog.h"
#include "identitydialog.h"

namespace Konversation
{

    ServerGroupDialog::ServerGroupDialog(const QString& title, QWidget *parent, const char *name)
        : KDialogBase(Plain, title, Ok|Cancel, Ok, parent, name)
    {
        m_id = -1;
        m_identitiesNeedsUpdate = false;
        m_editedServer = false;

        QFrame* mainWidget = plainPage();
        QGridLayout* mainLayout = new QGridLayout(mainWidget, 1, 2, 0, spacingHint());
        mainLayout->setColStretch(1, 10);

        QLabel* nameLbl = new QLabel(i18n("&Network:"), mainWidget);
        m_nameEdit = new QLineEdit(mainWidget);
        QWhatsThis::add(m_nameEdit, i18n("Enter the name of the Network here. You may create as many entries in the Server List screen with the same Network as you like."));
        nameLbl->setBuddy(m_nameEdit);

        QLabel* identityLbl = new QLabel(i18n("&Identity:"), mainWidget);
        m_identityCBox = new QComboBox(mainWidget);
        QWhatsThis::add(m_identityCBox,i18n("Choose an existing Identity or click the Edit button to add a new Identity or edit an existing one. The Identity will identify you and determine your nickname when you connect to the network."));
        identityLbl->setBuddy(m_identityCBox);
        QPushButton* editIdentityBtn = new QPushButton(i18n("Edit..."), mainWidget);
        connect(editIdentityBtn, SIGNAL(clicked()), this, SLOT(editIdentity()));

        QValueList<IdentityPtr> identities = Preferences::identityList();

        for(QValueList<IdentityPtr>::iterator it = identities.begin(); it != identities.end(); ++it)
        {
            m_identityCBox->insertItem((*it)->getName());
        }

        QLabel* commandLbl = new QLabel(i18n("Co&mmands:"), mainWidget);
        m_commandEdit = new QLineEdit(mainWidget);
        QWhatsThis::add(m_commandEdit, i18n("Optional. This command will be sent to the server after connecting. Example: <b>/msg NickServ IDENTIFY <i>konvirocks</i></b>. This example is for the freenode network, which requires users to register their nickname with a password and login when connecting. <i>konvirocks<i> is the password for the nickname given in Identity. You may enter more than one command by separating them with semicolons."));
        commandLbl->setBuddy(m_commandEdit);

        m_autoConnectCBox = new QCheckBox(i18n("Connect on &application start up"), mainWidget);
        QWhatsThis::add(m_autoConnectCBox, i18n("Check here if you want Konversation to automatically connect to this network whenever you open Konversation."));

        QWidget* groupWidget = new QWidget(mainWidget);
        QGridLayout* groupLayout = new QGridLayout(groupWidget, 1, 2, 0, spacingHint());

        QGroupBox* serverGBox = new QGroupBox(0, Qt::Horizontal, i18n("Servers"), groupWidget);
        serverGBox->setMargin(marginHint());
        QGridLayout* serverLayout = new QGridLayout(serverGBox->layout(), 1, 2, spacingHint());

        m_serverLBox = new QListBox(serverGBox);
        QWhatsThis::add(m_serverLBox, i18n("This is a list of IRC Servers in the network. When connecting to the network, Konversation will attempt to connect to the top server first. If this fails, it will attempt the second server. If this fails, it will attempt the third, and so on. At least one server must be specified. Click a server to highlight it."));
        QPushButton* addServerBtn = new QPushButton(i18n("Add..."), serverGBox);
        QPushButton* changeServerBtn = new QPushButton(i18n("Edit..."), serverGBox);
        QPushButton* removeServerBtn = new QPushButton(i18n("Delete"), serverGBox);
        m_upServerBtn = new QToolButton(serverGBox);
        m_upServerBtn->setIconSet(SmallIconSet("up"));
        m_upServerBtn->setAutoRepeat(true);
        m_upServerBtn->setEnabled(false);
        m_downServerBtn = new QToolButton(serverGBox);
        m_downServerBtn->setIconSet(SmallIconSet("down"));
        m_downServerBtn->setAutoRepeat(true);
        m_downServerBtn->setEnabled(false);

        connect(addServerBtn, SIGNAL(clicked()), this, SLOT(addServer()));
        connect(changeServerBtn, SIGNAL(clicked()), this, SLOT(editServer()));
        connect(removeServerBtn, SIGNAL(clicked()), this, SLOT(deleteServer()));
        connect(m_serverLBox, SIGNAL(selectionChanged()), this, SLOT(updateServerArrows()));
        connect(m_upServerBtn, SIGNAL(clicked()), this, SLOT(moveServerUp()));
        connect(m_downServerBtn, SIGNAL(clicked()), this, SLOT(moveServerDown()));

        serverLayout->setColStretch(0, 10);
        serverLayout->setRowStretch(4, 10);
        serverLayout->addMultiCellWidget(m_serverLBox, 0, 4, 0, 0);
        serverLayout->addMultiCellWidget(addServerBtn, 0, 0, 1, 4);
        serverLayout->addMultiCellWidget(changeServerBtn, 1, 1, 1, 4);
        serverLayout->addMultiCellWidget(removeServerBtn, 2, 2, 1, 4);
        serverLayout->addWidget(m_upServerBtn, 3, 2);
        serverLayout->addWidget(m_downServerBtn, 3, 3);

        QGroupBox* channelGBox = new QGroupBox(0, Qt::Horizontal, i18n("Auto Join Channels"), groupWidget);
        channelGBox->setMargin(marginHint());
        QGridLayout* channelLayout = new QGridLayout(channelGBox->layout(), 1, 2, spacingHint());

        m_channelLBox = new QListBox(channelGBox);
        QWhatsThis::add(m_channelLBox, i18n("Optional. This is a list of the channels that will be automatically joined once Konversation has connected to a server. You may leave this blank if you wish to not automatically join any channels."));
        QPushButton* addChannelBtn = new QPushButton(i18n("Add..."), channelGBox);
        QPushButton* changeChannelBtn = new QPushButton(i18n("Edit..."), channelGBox);
        QPushButton* removeChannelBtn = new QPushButton(i18n("Delete"), channelGBox);
        m_upChannelBtn = new QToolButton(channelGBox);
        m_upChannelBtn->setIconSet(SmallIconSet("up"));
        m_upChannelBtn->setAutoRepeat(true);
        m_upChannelBtn->setEnabled(false);
        m_downChannelBtn = new QToolButton(channelGBox);
        m_downChannelBtn->setIconSet(SmallIconSet("down"));
        m_downChannelBtn->setAutoRepeat(true);
        m_downChannelBtn->setEnabled(false);

        connect(addChannelBtn, SIGNAL(clicked()), this, SLOT(addChannel()));
        connect(changeChannelBtn, SIGNAL(clicked()), this, SLOT(editChannel()));
        connect(removeChannelBtn, SIGNAL(clicked()), this, SLOT(deleteChannel()));
        connect(m_channelLBox, SIGNAL(selectionChanged()), this, SLOT(updateChannelArrows()));
        connect(m_upChannelBtn, SIGNAL(clicked()), this, SLOT(moveChannelUp()));
        connect(m_downChannelBtn, SIGNAL(clicked()), this, SLOT(moveChannelDown()));

        channelLayout->setColStretch(0, 10);
        channelLayout->setRowStretch(4, 10);
        channelLayout->addMultiCellWidget(m_channelLBox, 0, 4, 0, 0);
        channelLayout->addMultiCellWidget(addChannelBtn, 0, 0, 1, 4);
        channelLayout->addMultiCellWidget(changeChannelBtn, 1, 1, 1, 4);
        channelLayout->addMultiCellWidget(removeChannelBtn, 2, 2, 1, 4);
        channelLayout->addWidget(m_upChannelBtn, 3, 2);
        channelLayout->addWidget(m_downChannelBtn, 3, 3);

        mainLayout->addWidget(nameLbl, 0, 0);
        mainLayout->addMultiCellWidget(m_nameEdit, 0, 0, 1, 2);
        mainLayout->addWidget(identityLbl, 2, 0);
        mainLayout->addWidget(m_identityCBox, 2, 1);
        mainLayout->addWidget(editIdentityBtn, 2, 2);
        mainLayout->addWidget(commandLbl, 3, 0);
        mainLayout->addMultiCellWidget(m_commandEdit, 3, 3, 1, 2);
        mainLayout->addMultiCellWidget(m_autoConnectCBox, 4, 4, 0, 2);
        mainLayout->addMultiCellWidget(groupWidget, 5, 5, 0, 2);
        mainLayout->setColStretch(1, 10);
        groupLayout->addWidget(serverGBox, 0, 0);
        groupLayout->addWidget(channelGBox, 0, 1);

        setButtonOK(KGuiItem(i18n("&OK"), "button_ok", i18n("Change network information")));
        setButtonCancel(KGuiItem(i18n("&Cancel"), "button_cancel", i18n("Discards all changes made")));

        m_nameEdit->setFocus();
    }

    ServerGroupDialog::~ServerGroupDialog()
    {
    }

    void ServerGroupDialog::setServerGroupSettings(ServerGroupSettingsPtr settings)
    {
        m_id = settings->id();
        m_sortIndex = settings->sortIndex();
        m_expanded = settings->expanded();
        m_nameEdit->setText(settings->name());
        m_identityCBox->setCurrentText(settings->identity()->getName());
        m_commandEdit->setText(settings->connectCommands());
        m_autoConnectCBox->setChecked(settings->autoConnectEnabled());
        m_serverList = settings->serverList(true);
        m_channelHistory = settings->channelHistory();
        ServerList::iterator it;
        m_serverLBox->clear();

        for(it = m_serverList.begin(); it != m_serverList.end(); ++it)
        {
            m_serverLBox->insertItem((*it).server());
        }

        m_channelList = settings->channelList();
        ChannelList::iterator it2;

        for(it2 = m_channelList.begin(); it2 != m_channelList.end(); ++it2)
        {
            m_channelLBox->insertItem((*it2).name());
        }
    }

    ServerGroupSettingsPtr ServerGroupDialog::serverGroupSettings()
    {
        ServerGroupSettingsPtr settings = new ServerGroupSettings(m_id);
        settings->setSortIndex(m_sortIndex);
        settings->setName(m_nameEdit->text());
        QValueList<IdentityPtr> identities = Preferences::identityList();
        settings->setIdentityId(identities[m_identityCBox->currentItem()]->id());
        settings->setConnectCommands(m_commandEdit->text());
        settings->setAutoConnectEnabled(m_autoConnectCBox->isChecked());
        settings->setServerList(m_serverList);
        settings->setChannelList(m_channelList);
        settings->setChannelHistory(m_channelHistory);
        settings->setExpanded(m_expanded);

        return settings;
    }

    ServerSettings ServerGroupDialog::editedServer()
    {
        if (m_editedServer && m_editedServerIndex < m_serverList.count())
        {
            return m_serverList[m_editedServerIndex];
        }

        return ServerSettings("");
    }

    int ServerGroupDialog::execAndEditServer(ServerSettings server)
    {
        show();
        editServer(server);
        return exec();
    }

    void ServerGroupDialog::addServer()
    {
        ServerDialog dlg(i18n("Add Server"), this);

        if(dlg.exec() == KDialog::Accepted)
        {
            ServerSettings server = dlg.serverSettings();
            m_serverLBox->insertItem(server.server());
            m_serverList.append(server);
            updateServerArrows();
        }
    }

    void ServerGroupDialog::editServer()
    {
        uint current = m_serverLBox->currentItem();

        if(current < m_serverList.count())
        {
            ServerDialog dlg(i18n("Edit Server"), this);
            dlg.setServerSettings(m_serverList[current]);

            if(dlg.exec() == KDialog::Accepted)
            {
                ServerSettings server = dlg.serverSettings();
                m_serverLBox->changeItem(server.server(), current);
                m_serverList[current] = server;
            }
        }
    }

    void ServerGroupDialog::editServer(ServerSettings server)
    {
        // Track the server the Server List dialog told us to edit
        // and find out which server to select in the listbox
        m_editedServer = true;
        m_editedServerIndex = m_serverList.findIndex(server);
        m_serverLBox->setCurrentItem(m_editedServerIndex);

        editServer();
    }

    void ServerGroupDialog::deleteServer()
    {
        uint current = m_serverLBox->currentItem();

        if (current < m_serverList.count())
        {
            m_serverList.remove(m_serverList.at(current));
            m_serverLBox->removeItem(current);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServer = false;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::updateServerArrows()
    {
        m_upServerBtn->setEnabled( m_serverLBox->count()>1 && m_serverLBox->currentItem()>0 );

        // FIXME: find another way than casting to overcome signedness warning
        m_downServerBtn->setEnabled( m_serverLBox->count()>1 &&
            m_serverLBox->currentItem()< static_cast<int>(m_serverLBox->count()-1) );
    }

    void ServerGroupDialog::moveServerUp()
    {
        uint current = m_serverLBox->currentItem();

        if (current > 0)
        {
            ServerSettings server = m_serverList[current];
            m_serverLBox->removeItem(current);
            m_serverLBox->insertItem(server.server(), current - 1);
            m_serverLBox->setCurrentItem(current - 1);
            ServerList::iterator it = m_serverList.remove(m_serverList.at(current));
            --it;
            m_serverList.insert(it, server);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServerIndex = current - 1;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::moveServerDown()
    {
        uint current = m_serverLBox->currentItem();

        if (current < (m_serverList.count() - 1))
        {
            ServerSettings server = m_serverList[current];
            m_serverLBox->removeItem(current);
            m_serverLBox->insertItem(server.server(), current + 1);
            m_serverLBox->setCurrentItem(current + 1);
            ServerList::iterator it = m_serverList.remove(m_serverList.at(current));
            ++it;
            m_serverList.insert(it, server);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServerIndex = current + 1;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::addChannel()
    {
        ChannelDialog dlg(i18n("Add Channel"), this);

        if(dlg.exec() == KDialog::Accepted)
        {
            ChannelSettings channel = dlg.channelSettings();
            m_channelLBox->insertItem(channel.name());
            m_channelList.append(channel);
            updateChannelArrows();
        }
    }

    void ServerGroupDialog::editChannel()
    {
        uint current = m_channelLBox->currentItem();

        if(current < m_channelList.count())
        {
            ChannelDialog dlg(i18n("Edit Channel"), this);
            dlg.setChannelSettings(m_channelList[current]);

            if(dlg.exec() == KDialog::Accepted)
            {
                ChannelSettings channel = dlg.channelSettings();
                m_channelLBox->changeItem(channel.name(), current);
                m_channelList[current] = channel;
            }
        }
    }

    void ServerGroupDialog::deleteChannel()
    {
        uint current = m_channelLBox->currentItem();

        if(current < m_channelList.count())
        {
            m_channelList.remove(m_channelList.at(current));
            m_channelLBox->removeItem(current);
            updateChannelArrows();
        }
    }

    void ServerGroupDialog::updateChannelArrows()
    {
        m_upChannelBtn->setEnabled( m_channelLBox->count()>1 && m_channelLBox->currentItem()>0 );

        // FIXME: find another way than casting to overcome signedness warning
        m_downChannelBtn->setEnabled( m_channelLBox->count()>1 &&
            m_channelLBox->currentItem()< static_cast<int>(m_channelLBox->count()-1) );
    }

    void ServerGroupDialog::moveChannelUp()
    {
        uint current = m_channelLBox->currentItem();

        if(current > 0)
        {
            ChannelSettings channel = m_channelList[current];
            m_channelLBox->removeItem(current);
            m_channelLBox->insertItem(channel.name(), current - 1);
            m_channelLBox->setCurrentItem(current - 1);
            ChannelList::iterator it = m_channelList.remove(m_channelList.at(current));
            --it;
            m_channelList.insert(it, channel);
        }

        updateChannelArrows();
    }

    void ServerGroupDialog::moveChannelDown()
    {
        uint current = m_channelLBox->currentItem();

        if(current < (m_channelList.count() - 1))
        {
            ChannelSettings channel = m_channelList[current];
            m_channelLBox->removeItem(current);
            m_channelLBox->insertItem(channel.name(), current + 1);
            m_channelLBox->setCurrentItem(current + 1);
            ChannelList::iterator it = m_channelList.remove(m_channelList.at(current));
            ++it;
            m_channelList.insert(it, channel);
        }

        updateChannelArrows();
    }

    void ServerGroupDialog::editIdentity()
    {
        IdentityDialog dlg(this);
        dlg.setCurrentIdentity(m_identityCBox->currentItem());
        QValueList<IdentityPtr> identities = Preferences::identityList();
        int identityId = identities[m_identityCBox->currentItem()]->id();

        if(dlg.exec() == KDialog::Accepted)
        {
            identities = Preferences::identityList();
            m_identityCBox->clear();

            for(QValueList<IdentityPtr>::iterator it = identities.begin(); it != identities.end(); ++it)
            {
                m_identityCBox->insertItem((*it)->getName());
            }

            m_identityCBox->setCurrentText(Preferences::identityById(identityId)->getName());
            m_identitiesNeedsUpdate = true;
        }
    }

    void ServerGroupDialog::slotOk()
    {
        if (m_nameEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The network name is required."));
        }
        else if (m_serverList.count() == 0)
        {
            KMessageBox::error(this, i18n("You need to add at least one server to the network."));
        }
        else
        {
            accept();
        }
    }

}

#include "servergroupdialog.moc"
