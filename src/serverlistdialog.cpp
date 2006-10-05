/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#include "serverlistdialog.h"

#include <qpushbutton.h>
#include <qframe.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qwhatsthis.h>
#include <qheader.h>

#include <klocale.h>
#include <kdebug.h>
#include <kguiitem.h>
#include <kmessagebox.h>

#include "preferences.h"
#include "konversationapplication.h"
#include "servergroupdialog.h"

namespace Konversation
{
    //
    // ServerListItem
    //

    ServerListItem::ServerListItem(KListView* parent, int serverGroupId, int sortIndex,
        const QString& serverGroup, const QString& identity, const QString& channels)
        : KListViewItem(parent, serverGroup, identity, channels)
    {
        m_serverGroupId = serverGroupId;
        m_sortIndex = sortIndex;
        m_name = serverGroup;
        m_isServer = false;
    }

    ServerListItem::ServerListItem(QListViewItem* parent, int serverGroupId, int sortIndex,
        const QString& name, const ServerSettings& server)
        : KListViewItem(parent, name)
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

        QListViewItem* item = firstChild();

        while (item)
        {
            if (item->isSelected())
                ++count;

            item = item->nextSibling();
        }

        return count;
    }

    int ServerListItem::compare(QListViewItem *i, int col, bool ascending) const
    {
        ServerListItem* item = static_cast<ServerListItem*>(i);

        if (col==0)
        {
            if (!item->server().server().isEmpty())
            {
                if (sortIndex() == item->sortIndex())
                    return 0;
                else if (sortIndex() < item->sortIndex())
                    return ascending ? -1 : 1;
                else
                    return ascending ? 1 : -1;
            }

            if (sortIndex() == item->sortIndex())
                return 0;
            else if (sortIndex() < item->sortIndex())
                return -1;
            else
                return 1;
        }

        return key( col, ascending ).localeAwareCompare( i->key( col, ascending ) );
    }

    ServerListDialog::ServerListDialog(QWidget *parent, const char *name)
        : KDialogBase(Plain, i18n("Server List"), Ok|Close, Ok, parent, name, false)
    {
        setButtonOK(KGuiItem(i18n("C&onnect"), "connect_creating", i18n("Connect to the server"), i18n("Click here to connect to the selected IRC network and channel.")));

        QFrame* mainWidget = plainPage();

        m_serverList = new ServerListView(mainWidget);
        QWhatsThis::add(m_serverList, i18n("This shows the listof configured IRC networks. An IRC network is a collection of cooperating servers. You need only connect to one of the servers in the network to be connected to the entire IRC network. Once connected, Konversation will automatically join the channels shown. When Konversation is started for the first time, the Freenode network and the <i>#kde</i> channel are already entered for you."));
        m_serverList->setAllColumnsShowFocus(true);
        m_serverList->setRootIsDecorated(true);
        m_serverList->setResizeMode(QListView::AllColumns);
        m_serverList->addColumn(i18n("Network"));
        m_serverList->addColumn(i18n("Identity"));
        m_serverList->addColumn(i18n("Channels"));
        m_serverList->setSelectionModeExt(KListView::Extended);
        m_serverList->setShowSortIndicator(true);
        m_serverList->setSortColumn(0);
        m_serverList->setDragEnabled(true);
        m_serverList->setAcceptDrops(true);
        m_serverList->setDropVisualizer(true);
        m_serverList->header()->setMovingEnabled(false);

        m_addButton = new QPushButton(i18n("&New..."), mainWidget);
        QWhatsThis::add(m_addButton, i18n("Click here to define a new Network, including the server to connect to, and the Channels to automatically join once connected."));
        m_editButton = new QPushButton(i18n("&Edit..."), mainWidget);
        m_delButton = new QPushButton(i18n("&Delete"), mainWidget);

        QGridLayout* layout = new QGridLayout(mainWidget, 1, 2, 0, spacingHint());

        layout->addMultiCellWidget(m_serverList, 0, 3, 0, 0);
        layout->addWidget(m_addButton, 0, 1);
        layout->addWidget(m_editButton, 1, 1);
        layout->addWidget(m_delButton, 2, 1);
        layout->setRowStretch(3, 10);

        m_editedItem = false;
        m_editedServer = ServerSettings("");

        // Load server list
        updateServerList();

        connect(m_serverList, SIGNAL(aboutToMove()), this, SLOT(slotAboutToMove()));
        connect(m_serverList, SIGNAL(moved()), this, SLOT(slotMoved()));
        connect(m_serverList, SIGNAL(doubleClicked(QListViewItem *, const QPoint&, int)), this, SLOT(slotOk()));
        connect(m_serverList, SIGNAL(selectionChanged()), this, SLOT(updateButtons()));
        connect(m_serverList, SIGNAL(expanded(QListViewItem*)), this, SLOT(slotSetGroupExpanded(QListViewItem*)));
        connect(m_serverList, SIGNAL(collapsed(QListViewItem*)), this, SLOT(slotSetGroupCollapsed(QListViewItem*)));
        connect(m_addButton, SIGNAL(clicked()), this, SLOT(slotAdd()));
        connect(m_editButton, SIGNAL(clicked()), this, SLOT(slotEdit()));
        connect(m_delButton, SIGNAL(clicked()), this, SLOT(slotDelete()));

        updateButtons();

        KConfig* config = kapp->config();
        config->setGroup("ServerListDialog");
        QSize newSize = size();
        newSize = config->readSizeEntry("Size", &newSize);
        resize(newSize);

        m_serverList->setSelected(m_serverList->firstChild(), true);
    }

    ServerListDialog::~ServerListDialog()
    {
        KConfig* config = kapp->config();
        config->setGroup("ServerListDialog");
        config->writeEntry("Size", size());
    }

    void ServerListDialog::slotClose()
    {
        slotApply();
        accept();
    }

    void ServerListDialog::slotOk()
    {
        QPtrList<QListViewItem> selected = m_serverList->selectedItems();
        ServerListItem * item = static_cast<ServerListItem*>(selected.first());

        while (item)
        {
            if (item->isServer())
            {
                emit connectToServer(item->serverGroupId(),item->server());
            }
            else
            {
                emit connectToServer(item->serverGroupId());
            }

            item = static_cast<ServerListItem*>(selected.next());
        }
    }

    void ServerListDialog::slotAdd()
    {
        ServerGroupDialog dlg(i18n("New Network"), this);

        if(dlg.exec() == KDialog::Accepted)
        {
            addServerGroup(dlg.serverGroupSettings());
            emit serverGroupsChanged();

            if(dlg.identitiesNeedsUpdate())
            {
                updateServerList();
            }
        }
    }

    void ServerListDialog::slotEdit()
    {
        ServerListItem* item = static_cast<ServerListItem*>(m_serverList->selectedItems().first());

        if (item)
        {
            Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(item->serverGroupId());

            if (serverGroup)
            {
                ServerGroupDialog dlg(i18n("Edit Network"), this);

                dlg.setServerGroupSettings(serverGroup);

                if (item->isServer())
                {
                    if(dlg.execAndEditServer(item->server()) == KDialog::Accepted)
                    {
                        delete item;

                        m_editedItem = true;
                        m_editedServerGroupId = serverGroup->id();
                        m_editedServer = dlg.editedServer();

                        *serverGroup = *(dlg.serverGroupSettings());
                        updateServerList();

                        emit serverGroupsChanged();
                    }
                }
                else
                {
                    if(dlg.exec() == KDialog::Accepted)
                    {
                        delete item;

                        m_editedItem = true;
                        m_editedServerGroupId = serverGroup->id();
                        m_editedServer = ServerSettings("");

                        *serverGroup = *(dlg.serverGroupSettings());
                        updateServerList();

                        emit serverGroupsChanged();
                    }
                }
            }
        }
    }

    void ServerListDialog::slotDelete()
    {
        QPtrList<QListViewItem> selectedItems = m_serverList->selectedServerListItems();

        if (selectedItems.isEmpty())
            return;

        ServerListItem* item = static_cast<ServerListItem*>(selectedItems.first());
        ServerListItem* parent = 0;

        // Make sure we're not deleting a network's only servers
        while (item)
        {
            if (item->isServer())
            {
                parent = static_cast<ServerListItem*>(item->parent());

                if (parent && parent->childCount() == 1)
                {
                    KMessageBox::error(this, i18n("You cannot delete %1.\n\nThe network %2 needs to have at least one server.").arg(item->name()).arg(parent->name()));
                    return;
                }
                else if (parent && parent->childCount() == parent->selectedChildrenCount())
                {
                    KMessageBox::error(this, i18n("You cannot delete the selected servers.\n\nThe network %1 needs to have at least one server.").arg(parent->name()));
                    return;
                }
            }

            item = static_cast<ServerListItem*>(selectedItems.next());
        }

        // Reset item
        item = static_cast<ServerListItem*>(selectedItems.first());

        // Ask the user if he really wants to delete what he selected
        QString question;

        if (selectedItems.count()>1)
            question = i18n("Do you really want to delete the selected entries?");
        else
            question = i18n("Do you really want to delete %1?").arg(item->name());

        if (KMessageBox::warningContinueCancel(this,question) == KMessageBox::Cancel)
        {
            return;
        }

        QListViewItem* itemBelow = 0;
        QListViewItem* itemAbove = 0;

        // Have fun deleting
        while (item)
        {
            itemBelow = item->nextSibling();
            itemAbove = item->itemAbove();

            if (item->isServer())
            {
                Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(item->serverGroupId());
                serverGroup->removeServer(item->server());
                delete item;
            }
            else
            {
                Preferences::removeServerGroup(item->serverGroupId());
                delete item;
            }

            item = static_cast<ServerListItem*>(selectedItems.next());
        }

        if (itemBelow)
        {
            m_serverList->setSelected(itemBelow,true);
            m_serverList->setCurrentItem(itemBelow);
        }
        else if (itemAbove)
        {
            m_serverList->setSelected(itemAbove,true);
            m_serverList->setCurrentItem(itemAbove);
        }
        else
        {
            if (m_serverList->firstChild())
            {
                m_serverList->setSelected(m_serverList->firstChild(),true);
                m_serverList->setCurrentItem(m_serverList->firstChild());
            }
        }
        emit serverGroupsChanged();
   }

    void ServerListDialog::slotSetGroupExpanded(QListViewItem* item)
    {
        ServerListItem* listItem = static_cast<ServerListItem*>(item);
        Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(listItem->serverGroupId());
        serverGroup->setExpanded(true);
    }

    void ServerListDialog::slotSetGroupCollapsed(QListViewItem* item)
    {
        ServerListItem* listItem = static_cast<ServerListItem*>(item);
        Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(listItem->serverGroupId());
        serverGroup->setExpanded(false);
    }

    void ServerListDialog::slotAboutToMove()
    {
        m_lastSortColumn = m_serverList->sortColumn();
        m_lastSortOrder = m_serverList->sortOrder();
        m_serverList->setSortColumn(-1);
    }

    void ServerListDialog::slotMoved()
    {
        Konversation::ServerGroupList newServerGroupList;

        ServerListItem* item = static_cast<ServerListItem*>(m_serverList->firstChild());
        int newSortIndex = 0;

        while (item)
        {
            Konversation::ServerGroupSettingsPtr serverGroup = Preferences::serverGroupById(item->serverGroupId());
            serverGroup->setSortIndex(newSortIndex);

            newServerGroupList.append(serverGroup);

            item->setSortIndex(newSortIndex);

            ++newSortIndex;
            item = static_cast<ServerListItem*>(item->nextSibling());
        }

        Preferences::setServerGroupList(newServerGroupList);

        m_serverList->setSortColumn(m_lastSortColumn);
        m_serverList->setSortOrder(m_lastSortOrder);

        emit serverGroupsChanged();
    }

    void ServerListDialog::updateButtons()
    {
        int count = m_serverList->selectedItems().count();
        bool enable = (count > 0);

        enableButtonOK(enable);
        m_delButton->setEnabled(enable);

        enable = (count == 1);
        m_editButton->setEnabled(enable);
    }

    void ServerListDialog::addServerGroup(ServerGroupSettingsPtr serverGroup)
    {
        if (m_serverList->lastChild())
        {
            ServerListItem* lastChild = static_cast<ServerListItem*>(m_serverList->lastChild());
            serverGroup->setSortIndex(lastChild->sortIndex() + 1);
        }

        Preferences::addServerGroup(serverGroup);
        QListViewItem* item = insertServerGroup(serverGroup);
        m_serverList->clearSelection();
        m_serverList->setSelected(item,true);
        m_serverList->setCurrentItem(item);
        m_serverList->ensureItemVisible(item);
    }

    void ServerListDialog::updateServerList()
    {
        m_serverList->clear();

        Konversation::ServerGroupList serverGroups = Preferences::serverGroupList();
        Konversation::ServerGroupList::iterator it;

        QListViewItem* networkItem = 0;

        for(it = serverGroups.begin(); it != serverGroups.end(); ++it)
        {
            networkItem = insertServerGroup((*it));

            // The method was called by slotEdit() ... initialize a pointer to the new
            // location of the edited server group
            if (m_editedItem && m_editedServer.server().isEmpty() && (*it)->id()==m_editedServerGroupId)
            {
                m_editedItemPtr = networkItem;
            }
        }

        // Highlight the last edited item
        if (m_editedItem)
        {
            m_serverList->setSelected(m_editedItemPtr,true);
            m_serverList->setCurrentItem(m_editedItemPtr);
            m_editedItem = false;
        }
    }

    QListViewItem* ServerListDialog::insertServerGroup(ServerGroupSettingsPtr serverGroup)
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

        QListViewItem* networkItem = 0;

        // Insert the server group into the list
        networkItem = new ServerListItem(m_serverList,
                                  serverGroup->id(),
                                  serverGroup->sortIndex(),
                                  serverGroup->name(),
                                  serverGroup->identity()->getName(),
                                  channels);

        // Recreate expanded/collapsed state
        if (serverGroup->expanded())
            networkItem->setOpen(true);

        // Produce a list of this server group's servers and iterate over it
        Konversation::ServerList serverList = serverGroup->serverList(true);
        Konversation::ServerList::iterator serverIt;

        QListViewItem* serverItem = 0;
        int i = 0;

        for (serverIt = serverList.begin(); serverIt != serverList.end(); ++serverIt)
        {
            // Produce a string representation of the server object
            QString name = (*serverIt).server();

            if ((*serverIt).port() != 6667)
                name += ':' + QString::number((*serverIt).port());

            if ((*serverIt).SSLEnabled())
                name += + " (SSL)";

            // Insert the server into the list, as child of the server group list item
            serverItem = new ServerListItem(networkItem,
                                            serverGroup->id(),
                                            i,
                                            name,
                                            (*serverIt));

            // The listview shouldn't allow this to be dragged
            serverItem->setDragEnabled(false);

            // Initialize a pointer to the new location of the last edited server
            if (m_editedItem && m_editedServer==(*serverIt))
                m_editedItemPtr = serverItem;

            ++i;
        }

        return networkItem;
    }

}

#include "serverlistdialog.moc"
