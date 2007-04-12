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
#include "linkaddressbook/nicksonlinetooltip.h"

#include <qvbox.h>
#include <qiconset.h>
#include <qpair.h>


class KListView;
class QPushButton;
class QPopupMenu;

class ChatWindow;

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
        // Ids associated with menu/button commands.
        enum CommandIDs
        {
            ciAddressbookChange, ciAddressbookNew, ciAddressbookDelete, ciAddressbookEdit,
            ciSendEmail, ciWhois, ciJoinChannel, ciOpenQuery
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
        KListView* getNickListView();
        NickInfoPtr getNickInfo(const QListViewItem* item);

        virtual bool canBeFrontView()   { return true; }

        signals:
        /**
         * Emitted when user clicks Edit Watch List button.
         */
        void editClicked();
        /**
         * Emitted whenever user double-clicks a nick in the Nicks Online tab.
         */
        void doubleClicked(const QString& server,const QString& nick);

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
        void processDoubleClick(QListViewItem* item);
        /**
         * Timer used to refresh display.
         */
        void timerFired();
        /**
         * Received when user clicks the Edit Contact (or New Contact) button.
         */
        void slotEditContactButton_Clicked();
        /**
         * Received when user clicks the Change Association button.
         */
        void slotChangeAssociationButton_Clicked();
        /**
         * Received when user clicks the Delete Association button.
         */
        void slotDeleteAssociationButton_Clicked();
        /**
         * Received when user selects a different item in the nicklistview.
         */
        void slotNickListView_SelectionChanged();
        /**
         * Received when right-clicking an item in the NickListView.
         */
        void slotNickListView_RightButtonClicked(QListViewItem* item, const QPoint& pt);
        /**
         * Received from server when a NickInfo changes its information.
         */
        void slotNickInfoChanged(Server* server, const NickInfoPtr nickInfo);
        /**
         * Received from popup menu when user chooses something.
         */
        void slotPopupMenu_Activated(int id);

    protected:
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

    private:
        /**
        * Returns the named child of parent item in a NicksOnlineItem
        * @param parent            Pointer to a NicksOnlineItem.
        * @param name              The name in the desired child QListViewItem, must be in column 0.
        * @param type              The type of entry to be found
        * @return                  Pointer to the child QListViewItem or 0 if not found.
        */
        QListViewItem* findItemChild(const QListViewItem* parent, const QString& name, NicksOnlineItem::NickListViewColumn type);
        /**
        * Returns the first occurrence of a child item of a given type in a NicksOnlineItem
        * @param parent            Pointer to a NicksOnlineItem.
        * @param type              The type of entry to be found
        * @return                  Pointer to the child QListViewItem or 0 if not found.
        */
        QListViewItem* findItemType(const QListViewItem* parent, NicksOnlineItem::NickListViewColumn type);
        /**
         * Returns a pointer to the network QListViewItem with the given name.
         * @param name              The name of the network, assumed to be in column 0 of the item.
         * @return                  Pointer to the QListViewItem or 0 if not found.
         */
        QListViewItem* findNetworkRoot(const QString& name);
        /**
         * Refresh the nicklistview for all servers.
         */
        void refreshAllServerOnlineLists();
        /**
         * Refreshes the information for the given item in the list.
         * @param item               Pointer to listview item.
         */
        void refreshItem(QListViewItem* item);
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
        bool getItemServerAndNick(const QListViewItem* item, QString& serverName, QString& nickname);
        /**
         * Given a server name and nickname, returns the item in the Nick List View displaying
         * the nick.
         * @param serverName        Name of server.Server
         * @param nickname          Nick name.
         * @return                  Pointer to QListViewItem displaying the nick, or 0 if not found.
         *
         * @see getItemServerAndNick
         */
        QListViewItem* getServerAndNickItem(const QString& serverName, const QString& nickname);
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
        void doCommand(int id);
        /**
         * Get the addressbook state of the nickname at the specified nicklistview item.
         * @param item              Item of the nicklistview.
         * @return                  Addressbook state.
         * 0 = not a nick, 1 = nick has no addressbook association, 2 = nick has association
         */
        int getNickAddressbookState(QListViewItem* item);
        /**
         * Sets the enabled/disabled state and labels of the addressbook buttons
         * based on the given nick addressbook state.
         * @param nickState         The state of the nick. 1 = not associated with addressbook,
         *                          2 = associated with addressbook.  @ref getNickAddressbookState.
         */
        void setupAddressbookButtons(int nickState);
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

        // The main display of networks, nicks, and channels.
        KListView* m_nickListView;
        // Buttons on screen.
        QPushButton* m_editContactButton;
        QPushButton* m_changeAssociationButton;
        QPushButton* m_deleteAssociationButton;
        // Context menu when right-clicking a nick.
        QPopupMenu* m_popupMenu;
        // Helper to display tooltip information for nicks.
        Konversation::KonversationNicksOnlineToolTip *m_tooltip;
        // A string containing the identifier for the "Offline" listview item
        QString c_offline;
        // Timer for refreshing display and generating WHOISes.
        QTimer* m_timer;
        // Addressbook icon.
        QIconSet m_kabcIconSet;
        /* Set to False every 8 seconds so that we generate a WHOIS on watch nicks that
           lack information.*/
        bool m_whoisRequested;
};
#endif
