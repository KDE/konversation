/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef NICKSONLINE_H
#define NICKSONLINE_H

#include "nickinfo.h"
#include "nicksonlineitem.h"
#include "chatwindow.h"

#include <QIcon>

#include <QMenu>

class ChatWindow;


class QTreeWidget;

class KToolBar;

/**
 * Shows a user tree of friends per server
 */
class NicksOnline : public ChatWindow
{
    Q_OBJECT

    public:
        // Columns of the NickListView.
        enum NickListViewColumn
        {
            nlvcNetworkNickChannel = 0,
            nlvcNetwork = 0,
            nlvcNick = 0,
            nlvcChannel = 0,
            nlvcAdditionalInfo = 1,
            nlvcServerName = 2                     // hidden
        };

        explicit NicksOnline(QWidget* parent);
        ~NicksOnline() override;

        // These are here for the benefit of NicksOnlineTooltip.
        NickInfoPtr getNickInfo(const QTreeWidgetItem* item) const;

        bool canBeFrontView() const override { return true; }

    public Q_SLOTS:
        /**
         * Refresh the nicklistview for a single server.
         * @param server            The server to be refreshed.
         */
        void updateServerOnlineList(Server* server);

    Q_SIGNALS:
        /**
         * Emitted whenever user double-clicks a nick in the Watched Nicks tab.
         */
        void doubleClicked(int connectionId,const QString& nick);

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;
        //! Reimplemented for dynamic tooltips
        bool eventFilter(QObject*obj, QEvent *ev) override;

    private Q_SLOTS:
        /**
         * When a user double-clicks a nickname in the nicklistview, let server know so that
         * it can perform the user's chosen default action for that.
         */
        void processDoubleClick(QTreeWidgetItem* item, int column);
        /**
         * Timer used to refresh display.
         */
        void timerFired();
        /**
         * Received when user selects a different item in the nicklistview.
         */
        void slotNickListView_SelectionChanged();
        /**
         * Received when right-clicking an item in the NickListView.
         */
        void slotCustomContextMenuRequested(const QPoint& point);
        /**
         * Received from server when a NickInfo changes its information.
         */
        void slotNickInfoChanged(Server* server, const NickInfoPtr &nickInfo);
        /**
         * Received from popup menu when user chooses something.
         */
        void slotPopupMenu_Activated(QAction* id);
        /**
         * Received when user added a new nick to the watched nicks.
         */
        void slotAddNickname(int serverGroupId, const QString& nickname);

    private:
        /**
        * Returns the named child of parent item in a NicksOnlineItem
        * @param parent            Pointer to a NicksOnlineItem.
        * @param name              The name in the desired child QListViewItem, must be in column 0.
        * @param type              The type of entry to be found
        * @return                  Pointer to the child QListViewItem or 0 if not found.
        */
        QTreeWidgetItem* findItemChild(const QTreeWidgetItem* parent, const QString& name,
                                       NicksOnlineItem::NickListViewColumn type) const;
        /**
        * Returns the first occurrence of a child item of a given type in a NicksOnlineItem
        * @param parent            Pointer to a NicksOnlineItem.
        * @param type              The type of entry to be found
        * @return                  Pointer to the child QListViewItem or 0 if not found.
        */
        QTreeWidgetItem* findItemType(const QTreeWidgetItem* parent, NicksOnlineItem::NickListViewColumn type) const;
        /**
         * Returns a pointer to the network QListViewItem with the given name.
         * @param name              The name of the network, assumed to be in column 0 of the item.
         * @return                  Pointer to the QListViewItem or 0 if not found.
         */
        QTreeWidgetItem* findNetworkRoot(int serverGroupId) const;
        /**
         * Refresh the nicklistview for all servers.
         */
        void refreshAllServerOnlineLists();
        /**
         * Refreshes the information for the given item in the list.
         * @param item               Pointer to listview item.
         */
        void refreshItem(QTreeWidgetItem* item);
        /**
         * Return a string containing formatted additional information about a nick.
         * @param nickInfo          A pointer to NickInfo structure for the nick.
         * @return                  A string formatted for display containing the information
         *                          about the nick.
         * @return needWhois        True if a WHOIS needs to be performed on the nick
         *                          to get additional information.
         */
        QString getNickAdditionalInfo(NickInfoPtr nickInfo, bool& needWhois) const;
        /**
         * Returns the server name and nickname of the specified nicklistview item.
         * @param item              The nicklistview item.
         * @return serverName       Name of the server for the nick at the item, or Null if not a nick.
         * @return nickname         The nickname at the item.
         */
        bool getItemServerAndNick(const QTreeWidgetItem* item, QString& serverName, QString& nickname) const;
        /**
         * Given a server name and nickname, returns the item in the Nick List View displaying
         * the nick.
         * @param serverName        Name of server.Server
         * @param nickname          Nick name.
         * @return                  Pointer to QListViewItem displaying the nick, or 0 if not found.
         *
         * @see getItemServerAndNick
         */
        QTreeWidgetItem* getServerAndNickItem(const QString& serverName, const QString& nickname) const;
        /**
         * Perform an command.
         * @param id                The command id.  @ref CommandIDs.
         *
         * The operation is performed on the nickname at the currently-selected item in
         * the nicklistview.
         *
         * Also refreshes the nicklistview display to reflect the new state
         * for the nick.
         */
        void doCommand(QAction* id);
        /**
         * Sets up toolbar actions based on the given item.
         * @param item              Item of the nicklistview.
         */
        void setupToolbarActions(NicksOnlineItem *item);
        /**
         * Sets up popup menu actions based on the given item.
         * @param item              Item of the nicklistview.
         */
        void setupPopupMenuActions(NicksOnlineItem *item);
        /**
         * Determines if a nick is online in any of the servers in a network and returns
         * a NickInfo if found, otherwise 0.
         * @param networkName        Server network name.
         * @param nickname           Nick name.
         * @return                   NickInfo if nick is online in any server, otherwise 0.
         */
        NickInfoPtr getOnlineNickInfo(const QString& networkName, const QString& nickname) const;
        /**
         * Requests a WHOIS for a specified server network and nickname.
         * The request is sent to the first server found in the network.
         * @param groupName          Server group name.
         * @param nickname           Nick name.
         */
        void requestWhois(const QString& networkName, const QString& nickname);
        /**
         * Updates the notify list based on the current state of the tree
         */
        void updateNotifyList();

    private:
        // The main display of networks, nicks, and channels.
        QTreeWidget* m_nickListView;
        // Context menu when right-clicking a nick.
        QMenu* m_popupMenu;
        KToolBar *m_toolBar;
        // A string containing the identifier for the "Offline" listview item
        QString c_offline;
        // Timer for refreshing display and generating WHOISes.
        QTimer* m_timer;
        // Online nick icon
        QIcon m_onlineIcon;
        // Offline nick icon
        QIcon m_offlineIcon;
        /* Set to False every 8 seconds so that we generate a WHOIS on watch nicks that
           lack information.*/
        bool m_whoisRequested;

    QAction* m_addNickname;
    QAction* m_removeNickname;
    QAction* m_whois;
    QAction* m_openQuery;
    QAction* m_joinChannel;

        Q_DISABLE_COPY(NicksOnline)
};

#endif
