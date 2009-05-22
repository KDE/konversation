/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004, 2007 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "serverlistdialog.h"
#include "preferences.h"
#include "application.h" ////// header renamed
#include "servergroupdialog.h"
#include "connectionsettings.h"
#include "ui_serverlistdialogui.h"

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <QHeaderView>

#include <klocale.h>
#include <kdebug.h>
#include <kguiitem.h>
#include <kmessagebox.h>
#include <kglobal.h>


namespace Konversation
{
    //
    // ServerListItem
    //

    ServerListItem::ServerListItem(QTreeWidget* parent, int serverGroupId, int sortIndex,
        const QString& serverGroup, const QString& identity, const QString& channels)
        : QTreeWidgetItem(parent, QStringList() << serverGroup << identity << channels)
    {
        m_serverGroupId = serverGroupId;
        m_sortIndex = sortIndex;
        m_name = serverGroup;
        m_isServer = false;
    }

    ServerListItem::ServerListItem(QTreeWidgetItem* parent, int serverGroupId, int sortIndex,
        const QString& name, const ServerSettings& server)
        : QTreeWidgetItem(parent, QStringList() << name)
    {
        m_serverGroupId = serverGroupId;
        m_sortIndex = sortIndex;
        m_name = name;
        m_server = server;
        m_isServer = true;
    }
    
    int ServerListItem::selectedChildrenCount()
    {
        int count = 0;
        for(int i=0; i<childCount(); i++)
        {
            if (child(i)->isSelected())
                ++count;
        }

        return count;
    }
    
    bool ServerListItem::operator<(const QTreeWidgetItem &other) const
    {
        int column = treeWidget()->sortColumn();
        const ServerListItem* item = static_cast<const ServerListItem*>(&other);
        if (column==0)
        {
            if (sortIndex() >= item->sortIndex())
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        return text(column) < other.text(column);    
    }

    ServerListDialog::ServerListDialog(const QString& title, QWidget *parent)
        : KDialog(parent)
    {
        setCaption(title);
        setButtons(Ok|Close);

        m_mainWidget = new Ui::ServerListDialogUI();
        m_mainWidget->setupUi(mainWidget());
        m_serverList = static_cast<ServerListView*>(m_mainWidget->m_serverList);
        m_addButton = m_mainWidget->m_addButton;
        m_delButton = m_mainWidget->m_delButton;
        m_editButton = m_mainWidget->m_editButton;
        m_showAtStartup = m_mainWidget->m_showAtStartup;

        setButtonGuiItem(Ok, KGuiItem(i18n("C&onnect"), "network-connect", i18n("Connect to the server"), i18n("Click here to connect to the selected IRC network and channel.")));

        m_showAtStartup->setChecked(Preferences::self()->showServerList());
        connect(m_showAtStartup, SIGNAL(toggled(bool)), this, SLOT(setShowAtStartup(bool)));

        m_serverList->setFocus();

        m_selectedItem = false;
        m_selectedItemPtr = 0;
        m_selectedServer = ServerSettings("");

        // Load server list
        updateServerList();

        connect(m_serverList, SIGNAL(aboutToMove()), this, SLOT(slotAboutToMove()));
        connect(m_serverList, SIGNAL(moved()), this, SLOT(slotMoved()));
        connect(m_serverList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(slotOk()));
        connect(m_serverList, SIGNAL(itemSelectionChanged()), this, SLOT(updateButtons()));
        connect(m_serverList, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(slotSetGroupExpanded(QTreeWidgetItem*)));
        connect(m_serverList, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(slotSetGroupCollapsed(QTreeWidgetItem*)));
        connect(m_addButton, SIGNAL(clicked()), this, SLOT(slotAdd()));
        connect(m_editButton, SIGNAL(clicked()), this, SLOT(slotEdit()));
        connect(m_delButton, SIGNAL(clicked()), this, SLOT(slotDelete()));
        connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
        connect(this, SIGNAL(cancelClicked()), this, SLOT(slotClose()));

        updateButtons();

        KConfigGroup config(KGlobal::config(), "ServerListDialog");
        QSize newSize = size();
        newSize = config.readEntry("Size", newSize);
        resize(newSize);
        m_serverList->header()->setMovable(false); // don't let the user reorder the header
        m_serverList->sortItems(0, Qt::AscendingOrder);
        //because it sorts the first column in ascending order by default
        //causing problems and such.
        m_serverList->topLevelItem(0)->setSelected(true);
    }

    ServerListDialog::~ServerListDialog()
    {
        KConfigGroup config(KGlobal::config(), "ServerListDialog");
        config.writeEntry("Size", size());
    }

    void ServerListDialog::slotClose()
    {
//         slotApply();
        accept();
    }

    void ServerListDialog::slotOk()
    {
        QList<QTreeWidgetItem*> selected = m_serverList->selectedItems();
        foreach (QTreeWidgetItem* i, selected)
        {
            ServerListItem * item = static_cast<ServerListItem*>(i);
            if (item->isServer())
            {
                ConnectionSettings settings;

                settings.setServerGroup(Preferences::serverGroupById(item->serverGroupId()));

                settings.setServer(item->server());

                emit connectTo(Konversation::PromptToReuseConnection, settings);
            }
            else
                emit connectTo(Konversation::PromptToReuseConnection, item->serverGroupId());
        }
    }

    void ServerListDialog::slotAdd()
    {
        QPointer<ServerGroupDialog> dlg = new ServerGroupDialog(i18n("New Network"), this);

        if(dlg->exec() == KDialog::Accepted)
        {
            addServerGroup(dlg->serverGroupSettings());

            emit serverGroupsChanged(dlg->serverGroupSettings());
        }
        delete dlg;
    }

    void ServerListDialog::slotEdit()
    {
        ServerListItem* item = static_cast<ServerListItem*>(m_serverList->selectedItems().first());

        if (item)
        {
            Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(item->serverGroupId());

            if (serverGroup)
            {
                QPointer<ServerGroupDialog> dlg = new ServerGroupDialog(i18n("Edit Network"), this);

                dlg->setServerGroupSettings(serverGroup);

                if (item->isServer())
                {
                    if(dlg->execAndEditServer(item->server()) == KDialog::Accepted)
                    {
                        delete item;

                        m_selectedItem = true;
                        m_selectedServerGroupId = serverGroup->id();
                        m_selectedServer = dlg->editedServer();

                        *serverGroup = *dlg->serverGroupSettings();

                        emit serverGroupsChanged(serverGroup); // will call updateServerList
                    }
                }
                else
                {
                    if(dlg->exec() == KDialog::Accepted)
                    {
                        delete item;

                        m_selectedItem = true;
                        m_selectedServerGroupId = serverGroup->id();
                        m_selectedServer = ServerSettings("");

                        *serverGroup = *dlg->serverGroupSettings();

                        emit serverGroupsChanged(serverGroup); // will call updateServerList
                    }
                }
                delete dlg;
            }
        }
    }

    void ServerListDialog::slotDelete()
    {
        QList<QTreeWidgetItem*> selectedItems;   
        // Make sure we're not deleting a network's only servers
        ServerListItem* parent = 0;
        ServerListItem* item;
        QTreeWidgetItemIterator it(m_serverList, QTreeWidgetItemIterator::Selected);
        while (*it)
        {
            //if it has a parent that's also selected it'll be deleted anyway so no need to worry
            if (!(*it)->parent() || !(*it)->parent()->isSelected())
            {
                item = static_cast<ServerListItem*>(*it);
                if (item->isServer())
                {
                    parent = static_cast<ServerListItem*>(item->parent());
                    
                    if (parent && parent->childCount() == 1)
                    {
                        KMessageBox::error(this, i18n("You cannot delete %1.\n\nThe network %2 needs to have at least one server.",item->name(),parent->name()));
                        return;
                    }
                    else if (parent && parent->childCount() == parent->selectedChildrenCount())
                    {
                        KMessageBox::error(this, i18np("You cannot delete the selected server.\n\nThe network %2 needs to have at least one server.",
                                            "You cannot delete the selected servers.\n\nThe network %2 needs to have at least one server.",
                                            parent->selectedChildrenCount(),
                                            parent->name()));
                                            return;
                    }
                }
                selectedItems.append(item); // if it hasn't returned by now it's good to go
            }
            ++it;
        }
        if (selectedItems.isEmpty())
            return;
        // Ask the user if he really wants to delete what he selected
        QString question;
        item = static_cast<ServerListItem*>(selectedItems.first());
        if (selectedItems.count()>1)
            question = i18n("Do you really want to delete the selected entries?");
        else
            question = i18n("Do you really want to delete %1?",item->name());

        if (KMessageBox::warningContinueCancel(this,question) == KMessageBox::Cancel)
        {
            return;
        }

        // Have fun deleting
        QTreeWidgetItem* rootItem = m_serverList->invisibleRootItem();
        foreach (QTreeWidgetItem* itemWidget, selectedItems)
        {
            item = static_cast<ServerListItem*>(itemWidget);
            
            if (item == m_selectedItemPtr)
                m_selectedItemPtr = 0;
            
            rootItem->removeChild(item);
            if (item->isServer())
            {
                Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(item->serverGroupId());
                serverGroup->removeServer(item->server());
            }
            else
            {
                Preferences::removeServerGroup(item->serverGroupId());
            }
            delete item;
        }

        emit serverGroupsChanged();
   }

    void ServerListDialog::slotSetGroupExpanded(QTreeWidgetItem* item)
    {
        ServerListItem* listItem = static_cast<ServerListItem*>(item);
        Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(listItem->serverGroupId());
        serverGroup->setExpanded(true);
    }

    void ServerListDialog::slotSetGroupCollapsed(QTreeWidgetItem* item)
    {
        ServerListItem* listItem = static_cast<ServerListItem*>(item);
        Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(listItem->serverGroupId());
        serverGroup->setExpanded(false);
    }

    void ServerListDialog::slotAboutToMove()
    {
        m_lastSortColumn = m_serverList->sortColumn();
        m_lastSortOrder = m_serverList->header()->sortIndicatorOrder();
        m_serverList->setSortingEnabled(false);
        m_serverList->header()->setSortIndicatorShown(true);
    }

    void ServerListDialog::slotMoved()
    {
        Konversation::ServerGroupList newServerGroupList;

        ServerListItem* item;
        int sort=0;
        for (int i=0; i < m_serverList->topLevelItemCount(); i++ )
        {
            item = static_cast<ServerListItem*>(m_serverList->topLevelItem(i));
            Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(item->serverGroupId());
            serverGroup->setSortIndex(sort);

            newServerGroupList.append(serverGroup);

            item->setSortIndex(sort++);
            for(int j=0; j < item->childCount(); j++)
                static_cast<ServerListItem*>(item->child(j))->setSortIndex(++sort);

        }
        if(Preferences::serverGroupList() != newServerGroupList)
        {
            Preferences::setServerGroupList(newServerGroupList);

            emit serverGroupsChanged();
        }
        m_serverList->setSortingEnabled(true);
        m_serverList->sortItems(m_lastSortColumn, m_lastSortOrder);
    }

    void ServerListDialog::updateButtons()
    {
        int count = m_serverList->selectedItems().count();
        bool enable = (count > 0);

        enableButtonOk(enable);
        m_delButton->setEnabled(enable);

        enable = (count == 1);
        m_editButton->setEnabled(enable);
    }

    void ServerListDialog::addServerGroup(ServerGroupSettingsPtr serverGroup)
    {
        int sortTotal = m_serverList->topLevelItemCount();
        serverGroup->setSortIndex(sortTotal + 1);
        

        Preferences::addServerGroup(serverGroup);
        QTreeWidgetItem* item = insertServerGroup(serverGroup);
        m_serverList->clearSelection();
        item->setSelected(true);
        m_serverList->setCurrentItem(item);
    }

    void ServerListDialog::updateServerList()
    {
        if (!m_selectedItem && m_serverList->currentItem())
        {
            ServerListItem* item = static_cast<ServerListItem*>(m_serverList->currentItem());

            m_selectedItem = true;
            m_selectedServerGroupId = item->serverGroupId();

            if (item->isServer())
                m_selectedServer = item->server();
            else
                m_selectedServer = ServerSettings("");
        }

        m_serverList->setUpdatesEnabled(false);
        m_serverList->clear();

        Konversation::ServerGroupList serverGroups = Preferences::serverGroupList();
        Konversation::ServerGroupList::const_iterator it;

        QTreeWidgetItem* networkItem = 0;

        for(it = serverGroups.constBegin(); it != serverGroups.constEnd(); ++it)
        {
            networkItem = insertServerGroup((*it));

            // The method was called by slotEdit() ... initialize a pointer to the new
            // location of the edited server group
            if (m_selectedItem && m_selectedServer.host().isEmpty() && (*it)->id()==m_selectedServerGroupId)
            {
                m_selectedItemPtr = networkItem;
            }
        }

        // Highlight the last edited item
        if (m_selectedItem)
        {
            m_selectedItemPtr->setSelected(true);
            m_serverList->setCurrentItem(m_selectedItemPtr);
            m_selectedItem = false;
        }

        m_serverList->setUpdatesEnabled(true);
        m_serverList->repaint();
    }

    QTreeWidgetItem* ServerListDialog::insertServerGroup(ServerGroupSettingsPtr serverGroup)
    {
        // Produce a list of this server group's channels
        QString channels;

        Konversation::ChannelList channelList = serverGroup->channelList();
        Konversation::ChannelList::iterator channelIt;
        Konversation::ChannelList::iterator begin = channelList.begin();

        for(channelIt = begin; channelIt != channelList.end(); ++channelIt)
        {
            if (channelIt != begin)
                channels += ", ";

            channels += (*channelIt).name();
        }

        QTreeWidgetItem* networkItem = 0;

        // Insert the server group into the list
        networkItem = new ServerListItem(m_serverList,
                                  serverGroup->id(),
                                  serverGroup->sortIndex(),
                                  serverGroup->name(),
                                  serverGroup->identity()->getName(),
                                  channels);

        // Recreate expanded/collapsed state
        if (serverGroup->expanded())
            networkItem->setExpanded(true);

        // Produce a list of this server group's servers and iterate over it
        Konversation::ServerList serverList = serverGroup->serverList();
        Konversation::ServerList::const_iterator serverIt;

        int i = 0;
        for (serverIt = serverList.constBegin(); serverIt != serverList.constEnd(); ++serverIt)
        {
            // Produce a string representation of the server object
            QString name = (*serverIt).host();

            if ((*serverIt).port() != 6667)
                name += ':' + QString::number((*serverIt).port());

            if ((*serverIt).SSLEnabled())
                name += + " (SSL)";

            // Insert the server into the list, as child of the server group list item
            QTreeWidgetItem* serverItem = new ServerListItem(networkItem,
                                            serverGroup->id(),
                                            i,
                                            name,
                                            (*serverIt));


            // Initialize a pointer to the new location of the last edited server
            if (m_selectedItem && m_selectedServer==(*serverIt))
                m_selectedItemPtr = serverItem;

            ++i;
        }

        return networkItem;
    }

    void ServerListDialog::setShowAtStartup(bool show)
    {
        Preferences::self()->setShowServerList(show);
    }
}

#include "serverlistdialog.moc"
