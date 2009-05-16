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
#include "identity.h"
#include "application.h" ////// header renamed
#include "viewcontainer.h"
#include "preferences.h"
#include "serversettings.h"
#include "serverdialog.h"
#include "channeldialog.h"
#include "identitydialog.h"
#include "ui_servergroupdialogui.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <klineedit.h>


namespace Konversation
{

    ServerGroupDialog::ServerGroupDialog(const QString& title, QWidget *parent)
        : KDialog(parent)
    {
        setCaption(title);
        setButtons(Ok|Cancel);

        m_id = -1;
        m_identitiesNeedsUpdate = false;
        m_editedServer = false;

        m_mainWidget = new Ui::ServerGroupDialogUI();
        m_mainWidget->setupUi(mainWidget());

        m_mainWidget->m_networkLabel->setBuddy(m_mainWidget->m_nameEdit);

        m_mainWidget->m_identityLabel->setBuddy(m_mainWidget->m_identityCBox);
        connect(m_mainWidget->m_editIdentityButton, SIGNAL(clicked()), this, SLOT(editIdentity()));

        IdentityList identities = Preferences::identityList();

        for (IdentityList::ConstIterator it = identities.constBegin(); it != identities.constEnd(); ++it)
            m_mainWidget->m_identityCBox->addItem((*it)->getName());

        m_mainWidget->m_commandsLabel->setBuddy(m_mainWidget->m_commandEdit);

        m_mainWidget->m_removeServerButton->setIcon(KIcon("list-remove"));
        m_mainWidget->m_upServerBtn->setIcon(KIcon("arrow-up"));
        m_mainWidget->m_downServerBtn->setIcon(KIcon("arrow-down"));

        connect(m_mainWidget->m_addServerButton, SIGNAL(clicked()), this, SLOT(addServer()));
        connect(m_mainWidget->m_changeServerButton, SIGNAL(clicked()), this, SLOT(editServer()));
        connect(m_mainWidget->m_removeServerButton, SIGNAL(clicked()), this, SLOT(deleteServer()));
        connect(m_mainWidget->m_serverLBox, SIGNAL(currentRowChanged(int)), this, SLOT(updateServerArrows()));
        connect(m_mainWidget->m_upServerBtn, SIGNAL(clicked()), this, SLOT(moveServerUp()));
        connect(m_mainWidget->m_downServerBtn, SIGNAL(clicked()), this, SLOT(moveServerDown()));

        m_mainWidget->m_removeChannelButton->setIcon(KIcon("list-remove"));
        m_mainWidget->m_upChannelBtn->setIcon(KIcon("arrow-up"));
        m_mainWidget->m_downChannelBtn->setIcon(KIcon("arrow-down"));

        connect(m_mainWidget->m_addChannelButton, SIGNAL(clicked()), this, SLOT(addChannel()));
        connect(m_mainWidget->m_changeChannelButton, SIGNAL(clicked()), this, SLOT(editChannel()));
        connect(m_mainWidget->m_removeChannelButton, SIGNAL(clicked()), this, SLOT(deleteChannel()));
        connect(m_mainWidget->m_channelLBox, SIGNAL(currentRowChanged(int)), this, SLOT(updateChannelArrows()));
        connect(m_mainWidget->m_upChannelBtn, SIGNAL(clicked()), this, SLOT(moveChannelUp()));
        connect(m_mainWidget->m_downChannelBtn, SIGNAL(clicked()), this, SLOT(moveChannelDown()));

        setButtonGuiItem(Ok, KGuiItem(i18n("&OK"), "dialog-ok", i18n("Change network information")));
        setButtonGuiItem(Cancel, KGuiItem(i18n("&Cancel"), "dialog-cancel", i18n("Discards all changes made")));

        m_mainWidget->m_nameEdit->setFocus();
    }

    ServerGroupDialog::~ServerGroupDialog()
    {
    }

    void ServerGroupDialog::setServerGroupSettings(ServerGroupSettingsPtr settings)
    {
        m_id = settings->id();
        m_sortIndex = settings->sortIndex();
        m_expanded = settings->expanded();
        m_enableNotifications = settings->enableNotifications();
        m_mainWidget->m_nameEdit->setText(settings->name());

        const int i = m_mainWidget->m_identityCBox->findText(settings->identity()->getName());
        if (i != -1)
            m_mainWidget->m_identityCBox->setCurrentIndex(i);
        else
            m_mainWidget->m_identityCBox->setItemText(m_mainWidget->m_identityCBox->currentIndex(), settings->identity()->getName());

        m_mainWidget->m_commandEdit->setText(settings->connectCommands());
        m_mainWidget->m_autoConnectCBox->setChecked(settings->autoConnectEnabled());
        m_serverList = settings->serverList();
        m_channelHistory = settings->channelHistory();
        ServerList::iterator it;
        m_mainWidget->m_serverLBox->clear();

        for(it = m_serverList.begin(); it != m_serverList.end(); ++it)
        {
            m_mainWidget->m_serverLBox->addItem((*it).host());
        }

        m_channelList = settings->channelList();
        ChannelList::iterator it2;

        for(it2 = m_channelList.begin(); it2 != m_channelList.end(); ++it2)
        {
            m_mainWidget->m_channelLBox->addItem((*it2).name());
        }
    }

    ServerGroupSettingsPtr ServerGroupDialog::serverGroupSettings()
    {
        ServerGroupSettingsPtr settings(new ServerGroupSettings(m_id));
        settings->setSortIndex(m_sortIndex);
        settings->setName(m_mainWidget->m_nameEdit->text());
        IdentityList identities = Preferences::identityList();
        settings->setIdentityId(identities[m_mainWidget->m_identityCBox->currentIndex()]->id());
        settings->setConnectCommands(m_mainWidget->m_commandEdit->text());
        settings->setAutoConnectEnabled(m_mainWidget->m_autoConnectCBox->isChecked());
        settings->setServerList(m_serverList);
        settings->setChannelList(m_channelList);
        settings->setChannelHistory(m_channelHistory);
        settings->setNotificationsEnabled(m_enableNotifications);
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
        QPointer<ServerDialog> dlg = new ServerDialog(i18n("Add Server"), this);

        if(dlg->exec() == KDialog::Accepted)
        {
            ServerSettings server = dlg->serverSettings();
            m_mainWidget->m_serverLBox->addItem(server.host());
            m_serverList.append(server);
            updateServerArrows();
        }
        delete dlg;
    }

    void ServerGroupDialog::editServer()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if(current < m_serverList.count())
        {
            QPointer <ServerDialog> dlg = new ServerDialog(i18n("Edit Server"), this);
            dlg->setServerSettings(m_serverList[current]);

            if(dlg->exec() == KDialog::Accepted)
            {
                ServerSettings server = dlg->serverSettings();
                m_mainWidget->m_serverLBox->item(current)->setText(server.host());
                m_serverList[current] = server;
            }
            delete dlg;
        }
    }

    void ServerGroupDialog::editServer(ServerSettings server)
    {
        // Track the server the Server List dialog told us to edit
        // and find out which server to select in the listbox
        m_editedServer = true;
        m_editedServerIndex = m_serverList.indexOf(server);
        m_mainWidget->m_serverLBox->setCurrentRow(m_editedServerIndex);

        editServer();
    }

    void ServerGroupDialog::deleteServer()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if (current < m_serverList.count())
        {
            m_serverList.removeAt(current);
            delete m_mainWidget->m_serverLBox->takeItem(current);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServer = false;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::updateServerArrows()
    {
        m_mainWidget->m_upServerBtn->setEnabled( m_mainWidget->m_serverLBox->count()>1 && m_mainWidget->m_serverLBox->currentRow()>0 );

        m_mainWidget->m_downServerBtn->setEnabled( m_mainWidget->m_serverLBox->count()>1 &&
            m_mainWidget->m_serverLBox->currentRow()<m_mainWidget->m_serverLBox->count()-1 );
        bool enabled = m_mainWidget->m_serverLBox->currentRow() >= 0;
        m_mainWidget->m_removeServerButton->setEnabled(enabled);
        m_mainWidget->m_changeServerButton->setEnabled(enabled);
    }

    void ServerGroupDialog::moveServerUp()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if (current > 0)
        {
            ServerSettings server = m_serverList[current];
            delete m_mainWidget->m_serverLBox->takeItem(current);
            m_mainWidget->m_serverLBox->insertItem(current - 1, server.host());
            m_mainWidget->m_serverLBox->setCurrentRow(current - 1);
            m_serverList.move(current, current - 1);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServerIndex = current - 1;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::moveServerDown()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if (current < (m_serverList.count() - 1))
        {
            ServerSettings server = m_serverList[current];
            delete m_mainWidget->m_serverLBox->takeItem(current);
            m_mainWidget->m_serverLBox->insertItem(current + 1, server.host());
            m_mainWidget->m_serverLBox->setCurrentRow(current + 1);
            m_serverList.move(current, current + 1);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServerIndex = current + 1;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::addChannel()
    {
        QPointer<ChannelDialog> dlg = new ChannelDialog(i18n("Add Channel"), this);

        if(dlg->exec() == KDialog::Accepted)
        {
            ChannelSettings channel = dlg->channelSettings();
            m_mainWidget->m_channelLBox->addItem(channel.name());
            m_channelList.append(channel);
            updateChannelArrows();
        }
        delete dlg;
    }

    void ServerGroupDialog::editChannel()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current < m_channelList.count())
        {
            QPointer<ChannelDialog> dlg = new ChannelDialog(i18n("Edit Channel"), this);
            dlg->setChannelSettings(m_channelList[current]);

            if(dlg->exec() == KDialog::Accepted)
            {
                ChannelSettings channel = dlg->channelSettings();
                m_mainWidget->m_channelLBox->item(current)->setText(channel.name());
                m_channelList[current] = channel;
            }
            delete dlg;
        }
    }

    void ServerGroupDialog::deleteChannel()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current < m_channelList.count())
        {
            m_channelList.removeOne(m_channelList.at(current));
            delete m_mainWidget->m_channelLBox->takeItem(current);
            updateChannelArrows();
        }
    }

    void ServerGroupDialog::updateChannelArrows()
    {
        m_mainWidget->m_upChannelBtn->setEnabled( m_mainWidget->m_channelLBox->count()>1 && m_mainWidget->m_channelLBox->currentRow()>0 );

        m_mainWidget->m_downChannelBtn->setEnabled( m_mainWidget->m_channelLBox->count()>1 &&
            m_mainWidget->m_channelLBox->currentRow()<m_mainWidget->m_channelLBox->count()-1 );
        bool selected = m_mainWidget->m_channelLBox->currentRow() >= 0;
        m_mainWidget->m_removeChannelButton->setEnabled(selected);
        m_mainWidget->m_changeChannelButton->setEnabled(selected);
    }

    void ServerGroupDialog::moveChannelUp()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current > 0)
        {
            ChannelSettings channel = m_channelList[current];
            delete m_mainWidget->m_channelLBox->takeItem(current);
            m_mainWidget->m_channelLBox->insertItem(current - 1, channel.name());
            m_mainWidget->m_channelLBox->setCurrentRow(current - 1);
            m_channelList.move(current, current - 1);
        }

        updateChannelArrows();
    }

    void ServerGroupDialog::moveChannelDown()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current < (m_channelList.count() - 1))
        {
            ChannelSettings channel = m_channelList[current];
            delete m_mainWidget->m_channelLBox->takeItem(current);
            m_mainWidget->m_channelLBox->insertItem(current + 1, channel.name());
            m_mainWidget->m_channelLBox->setCurrentRow(current + 1);
            m_channelList.move(current, current + 1);
        }

        updateChannelArrows();
    }

    void ServerGroupDialog::editIdentity()
    {
        QPointer<IdentityDialog> dlg = new IdentityDialog(this);
        dlg->setCurrentIdentity(m_mainWidget->m_identityCBox->currentIndex());

        if(dlg->exec() == KDialog::Accepted)
        {
            IdentityList identities = Preferences::identityList();
            m_mainWidget->m_identityCBox->clear();

            for(IdentityList::ConstIterator it = identities.constBegin(); it != identities.constEnd(); ++it)
            {
                m_mainWidget->m_identityCBox->addItem((*it)->getName());
            }

            const int i = m_mainWidget->m_identityCBox->findText(dlg->currentIdentity()->getName());
            if (i != -1)
            {
                m_mainWidget->m_identityCBox->setCurrentIndex(i);
            }
            else
            {
                m_mainWidget->m_identityCBox->setItemText(m_mainWidget->m_identityCBox->currentIndex(), dlg->currentIdentity()->getName());
            }

            m_identitiesNeedsUpdate = true; // and what's this for?
            ViewContainer* vc = KonversationApplication::instance()->getMainWindow()->getViewContainer();
            vc->updateViewEncoding(vc->getFrontView());
        }
        delete dlg;
    }

    void ServerGroupDialog::accept()
    {
        if (m_mainWidget->m_nameEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The network name is required."));
        }
        else if (m_serverList.count() == 0)
        {
            KMessageBox::error(this, i18n("You need to add at least one server to the network."));
        }
        else
        {
            KDialog::accept();
        }
    }

}

#include "servergroupdialog.moc"
