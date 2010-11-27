/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  shows a user tree of friends per server
  begin:     Sam Aug 31 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICKSONLINE_H
#define NICKSONLINE_H

#include "nickinfo.h"
#include "nicksonlineitem.h"
#include "chatwindow.h"

#include <QIcon>

#include <KMenu>

class ChatWindow;


class QTreeWidget;

class KToolBar;

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
            nlvcKabc = 1,
            nlvcAdditionalInfo = 1,
            nlvcServerName = 2                     // hidden
        };
        enum NickState
        {
            nsNotANick = 0,                       // User didn't click on a nickname.
            nsNoAddress = 1,                      // Nick does not have an addressbook association.
            nsHasAddress = 2                      // Nick has an associated addressbook entry.
        };

        explicit NicksOnline(QWidget* parent);
        ~NicksOnline();

        // These are here for the benefit of NicksOnlineTooltip.
        QTreeWidget* getNickListView();
        NickInfoPtr getNickInfo(const QTreeWidgetItem* item);

        virtual bool canBeFrontView()   { return true; }

    signals:
        /**
         * Emitted whenever user double-clicks a nick in the Nicks Online tab.
         */
        void doubleClicked(int connectionId,const QString& nick);

        void showView(ChatWindow* view);

    public slots:

        /**
         * Refresh the nicklistview for a single server.
         * @param server            The server to be refreshed.
         */
        void updateServerOnlineList(Server* server);

    protected slots:
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
        void slotNickInfoChanged(Server* server, const NickInfoPtr nickInfo);
        /**
         * Received from popup menu when user chooses something.
         */
        void slotPopupMenu_Activated(QAction* id);
        /**
         * Received when user added a new nick to the watched nicks.
         */
        void slotAddNickname(int serverGroupId, const QString& nickname);

    protected:
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();
    //! Reimplemented for dynamic tooltips
    virtual bool eventFilter(QObject*obj, QEvent *ev);
    private:
        /**
        * Returns the named child of parent item in a NicksOnlineItem
        * @param parent            Pointer to a NicksOnlineItem.
        * @param name              The name in the desired child QListViewItem, must be in column 0.
        * @param type              The type of entry to be found
        * @return                  Pointer to the child QListViewItem or 0 if not found.
        */
        QTreeWidgetItem* findItemChild(const QTreeWidgetItem* parent, const QString& name, NicksOnlineItem::NickListViewColumn type);
        /**
        * Returns the first occurrence of a child item of a given type in a NicksOnlineItem
        * @param parent            Pointer to a NicksOnlineItem.
        * @param type              The type of entry to be found
        * @return                  Pointer to the child QListViewItem or 0 if not found.
        */
        QTreeWidgetItem* findItemType(const QTreeWidgetItem* parent, NicksOnlineItem::NickListViewColumn type);
        /**
         * Returns a pointer to the network QListViewItem with the given name.
         * @param name              The name of the network, assumed to be in column 0 of the item.
         * @return                  Pointer to the QListViewItem or 0 if not found.
         */
        QTreeWidgetItem* findNetworkRoot(int serverGroupId);
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
        QString getNickAdditionalInfo(NickInfoPtr nickInfo, KABC::Addressee addressee,
            bool& needWhois);
        /**
         * Invokes the KAddressBook contact editor for the specified contact id.
         * @param uid               Id of the contact.
         * @return                  False if unable to invoke the Contact editor.
         */
        bool editAddressee(const QString &uid);
        /**
         * Returns the server name and nickname of the specified nicklistview item.
         * @param item              The nicklistview item.
         * @return serverName       Name of the server for the nick at the item, or Null if not a nick.
         * @return nickname         The nickname at the item.
         */
        bool getItemServerAndNick(const QTreeWidgetItem* item, QString& serverName, QString& nickname);
        /**
         * Given a server name and nickname, returns the item in the Nick List View displaying
         * the nick.
         * @param serverName        Name of server.Server
         * @param nickname          Nick name.
         * @return                  Pointer to QListViewItem displaying the nick, or 0 if not found.
         *
         * @see getItemServerAndNick
         */
        QTreeWidgetItem* getServerAndNickItem(const QString& serverName, const QString& nickname);
        /**
         * Perform an addressbook command (edit contact, create new contact,
         * change/delete association.)
         * @param id                The command id.  @ref CommandIDs.
         *
         * The operation is performed on the nickname at the currently-selected item in
         * the nicklistview.
         *
         * Also refreshes the nicklistview display to reflect the new addressbook state
         * for the nick.
         */
        void doCommand(QAction* id);
        /**
         * Get the addressbook state of the nickname at the specified nicklistview item.
         * @param item              Item of the nicklistview.
         * @return                  Addressbook state.
         * 0 = not a nick, 1 = nick has no addressbook association, 2 = nick has association
         */
        int getNickAddressbookState(QTreeWidgetItem* item);
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
        NickInfoPtr getOnlineNickInfo(QString& networkName, QString& nickname);
        /**
         * Requests a WHOIS for a specified server network and nickname.
         * The request is sent to the first server found in the network.
         * @param groupName          Server group name.
         * @param nickname           Nick name.
         */
        void requestWhois(QString& networkName, QString& nickname);
        /**
         * Updates the notify list based on the current state of the tree
         */
        void updateNotifyList();

        // The main display of networks, nicks, and channels.
        QTreeWidget* m_nickListView;
        // Context menu when right-clicking a nick.
        KMenu* m_popupMenu;
        KToolBar *m_toolBar;
        // A string containing the identifier for the "Offline" listview item
        QString c_offline;
        // Timer for refreshing display and generating WHOISes.
        QTimer* m_timer;
        // Addressbook icon.
        QIcon m_kabcIconSet;
        // Online nick icon
        QIcon m_onlineIcon;
        // Offline nick icon
        QIcon m_offlineIcon;
        /* Set to False every 8 seconds so that we generate a WHOIS on watch nicks that
           lack information.*/
        bool m_whoisRequested;


    QAction* m_addNickname;
    QAction* m_removeNickname;
    QAction* m_newContact;
    QAction* m_editContact;
    QAction* m_chooseAssociation;
    QAction* m_changeAssociation;
    QAction* m_deleteAssociation;
    QAction* m_whois;
    QAction* m_openQuery;
    QAction* m_sendMail;
    QAction* m_joinChannel;

};
#endif
