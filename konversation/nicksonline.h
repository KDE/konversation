/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicksonline.h  -  shows a user tree of friends per server
  begin:     Sam Aug 31 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICKSONLINE_H
#define NICKSONLINE_H

// Qt includes.
#include <qvbox.h>
#include <qiconset.h>
#include <qpair.h>

// Konversation includes.
#include "nickinfo.h"
#include "chatwindow.h"
#include "linkaddressbook/nicksonlinetooltip.h"
/*
  @author Dario Abatianni
*/

class KListView;
class QPushButton;
class QPopupMenu;

class NicksOnline : public ChatWindow
{
  Q_OBJECT

  public:
    // Columns of the NickListView.
    enum NickListViewColumn {
      nlvcGroupNickChannel = 0,
      nlvcGroup = 0,
      nlvcNick = 0,
      nlvcChannel = 0,
      nlvcKabc = 1,
      nlvcAdditionalInfo = 1,
      nlvcServerName = 2            // hidden
    };
    // Ids associated with menu/button commands.
    enum CommandIDs
    {
      ciAddressbookChange, ciAddressbookNew, ciAddressbookDelete, ciAddressbookEdit
    };

    
#ifdef USE_MDI
    NicksOnline(QString caption);
#else
    NicksOnline(QWidget* parent);
#endif
    ~NicksOnline();
    KListView* getNickListView();
	
    NickInfoPtr NicksOnline::getNickInfo(const QListViewItem* item);

  signals:
    void editClicked();
    /**
    * Emitted whenever user double-clicks a nick in the Nicks Online tab.
    */
    void doubleClicked(const QString& server,const QString& nick);

  public slots:

  protected slots:
    /**
    * When a user double-clicks a nickname in the nicklistview, let server know so that
    * it can perform the user's chosen default action for that.
    */
    void processDoubleClick(QListViewItem* item);
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

  protected:
    /**
    * Refresh the nicklistview for a single server.
    * @param server            The server to be refreshed.
    */
    void updateServerOnlineList(Server* server);
#ifdef USE_MDI
    virtual void closeYourself(ChatWindow*);
#endif
    /** Called from ChatWindow adjustFocus */
    virtual void childAdjustFocus();

    KListView* m_nickListView;
    QTimer* m_timer;
    QIconSet m_kabcIconSet;
    
  private:
    /**
    * Returns the named child of parent item in a KListView.
    * @param parent            Pointer to a QListViewItem.
    * @param name              The name in the desired child QListViewItem.  Name
    *                          is assumed to be in column 0 of the item.
    * @return                  Pointer to the child QListViewItem or 0 if not found.
    */
    QListViewItem* findItemChild(const QListViewItem* parent, const QString& name);
    /**
    * Refresh the nicklistview for all servers.
    */
    void refreshAllServerOnlineLists();
    /**
    * Return a string containing formatted additional information about a nick.
    * @param nickInfo          A pointer to NickInfo structure for the nick.
    * @return                  A string formatted for display containing the information
    *                          about the nick.
    */
    QString getNickAdditionalInfo(NickInfoPtr nickInfo);

	
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
    * Determines if a nick is online in any of the servers in a group and returns
    * a NickInfo if found, otherwise 0.
    * @param groupName          Server group name.
    * @param nickname           Nick name.
    * @return                   NickInfo if nick is online in any server, otherwise 0.
    */
    NickInfoPtr getOnlineNickInfo(QString& groupName, QString& nickname);
    /**
    * Requests a WHOIS in all servers for a specified server group and nickname.
    * @param groupName          Server group name.
    * @param nickname           Nick name.
    */
    void requestWhois(QString& groupName, QString& nickname);
    
    QPushButton* m_editContactButton;
    QPushButton* m_changeAssociationButton;
    QPushButton* m_deleteAssociationButton;
    QPopupMenu* m_popupMenu;
    Konversation::KonversationNicksOnlineToolTip *m_tooltip;
    // A string containing internationalized "Offline".
    QString c_i18nOffline;
    
    /* Set to False every 8 seconds so that we generate a WHOIS on watch nicks that
       lack information.*/
    bool m_whoisRequested;
};

#endif
