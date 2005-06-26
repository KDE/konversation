/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef _KONVERSATIONMAINWINDOW_H_
#define _KONVERSATIONMAINWINDOW_H_

#include <qstringlist.h>

#include <kmainwindow.h>

#include "channel.h"
#include "preferences.h"
#include "ssllabel.h"
#include "nickinfo.h"
#include "server.h"

/*
 Dario Abatianni
*/

class KToggleAction;
class KActionMenu;
class KScriptManager;
class KTabWidget;
class QPoint;

class KonviBookmarkHandler;
class StatusPanel;
class ChatWindow;
class Channel;
class Query;
class RawLog;
class ChannelListPanel;
class DccPanel;
class DccTransferHandler;
class Ignore;
class NicksOnline;
class QuickButtonsDialog;
class UrlCatcher;

namespace Konversation
{
  class InsertCharDialog;
  class ServerListDialog;
  class TrayIcon;
}

class KonversationMainWindow : public KMainWindow
{
  Q_OBJECT

  public:
    KonversationMainWindow();
    ~KonversationMainWindow();

    StatusPanel* addStatusView(Server* server);
    RawLog* addRawLog(Server* server);
    ChannelListPanel* addChannelListPanel(Server* server);
    Channel* addChannel(Server* server,const QString& name);
    Query* addQuery(Server* server,const NickInfoPtr & name, bool weinitiated=true);

    DccPanel* getDccPanel();
    /** 
     * Bring this view to the front.
     */
    void showView(ChatWindow* view);

    /** Some errors need to be shown, even when konversation is minimized.
     *  For example, when a kimiface call is recieved to query a person,
     *  (e.g. the user choses "Chat with X" in kmail) but that person isn't
     *  recognised, we need to give immediate feedback to the user.
     */
    void focusAndShowErrorMessage(const QString &errorMsg);
    void appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView);
    void appendToFrontmostIfDifferent(const QString& type,const QString& message,ChatWindow* serverView);

    void updateFonts();
    void updateTabPlacement();
    
    Konversation::TrayIcon* systemTrayIcon() const { return tray; }

    ChatWindow* frontView() { return m_frontView; }


    // Bookmark related functions
    QString currentURL();
    QString currentTitle();

  signals:
    void prefsChanged();
    void startNotifyTimer(int msec);
    void openPrefsDialog();
    void openPrefsDialog(Preferences::Pages page);
    void showQuickConnectDialog();
    void quitServer();
    void nicksNowOnline(Server*);
    void closeTab(int id);
    void endNotification();

    void channelSplittersMoved(Channel*);

  public slots:
    void addDccPanel();     // connected in server class
    void addKonsolePanel(); // connected in server class
    void addUrlCatcher();
    void addDccChat(const QString& myNick,const QString& nick,const QString& numericalIp,const QStringList& arguments,bool listen);
    void insertRememberLine();

    void resetLag();
    void updateLag(Server* lagServer,int msec);
    void updateSSLInfo(Server* server);
    void tooLongLag(Server* lagServer,int msec);
    void channelPrefsChanged();
    void setOnlineList(Server* notifyServer,const QStringList& list, bool changed);
    void updateTrayIcon();
    void serverQuit(Server* server);
    void setShowTabBarCloseButton(bool s);
    
    void openLogFile(const QString& caption, const QString& file);

    void removeSSLIcon();
    void slotPrefsChanged();

    void openServerList();
    void openIdentitiesDialog();

    void serverStateChanged(Server* server, Server::State state);

  protected slots:
    void openPreferences();
    void openKeyBindings();
    void openQuickConnectDialog();
    void openChannelList();
    void openNotify();
    void openLogfile();
    void openNicksOnlinePanel();
    void closeNicksOnlinePanel();
    // it seems that moc does not honor #ifs in compile so we create an
    // empty slot in our .cpp file rather than #if this slot out
    void openNotifications();
    void openToolbars();

    void showToolbar();
    void showStatusbar();
    void showMenubar(bool dontShowWarning = false);
    /**
     * @see showView
     */
    void changeView(QWidget* view);
    void closeView(QWidget* view);

    void closeKonsolePanel(ChatWindow* konsolePanel);

    void newText(QWidget* view, const QString& highlightColor);
    void quitProgram();

    void notifyAction(const QString& serverName,const QString& nick);

    void nextTab();
    void previousTab();
    void closeTab();

    void goToTab(int page);

    void findText();
    void findNextText();
    void addIRCColor();
    void clearWindow();
    void closeQueries();
    void hideNicknameList();
    void clearTabs();

    void insertCharacter();
    void insertChar(const QChar& chr);

    void updateChannelInfo(const QString& info);

    void showJoinChannelDialog();
    
    void reconnectCurrentServer();

    void openURL(const QString& url, const QString& title);

    void showTabContextMenu(QWidget* tab, const QPoint& pos);

  protected:
    enum StatusID
    {
      StatusText,LagOMeter
    };

    bool queryClose();

    void addView(ChatWindow* view, const QString& label, bool weinitiated=true);
    void updateFrontView();

    void closeUrlCatcher();
    void closeDccPanel();
    void deleteDccPanel();

    virtual bool event(QEvent* e);
    
    KTabWidget* getViewContainer();
    KTabWidget* viewContainer;

    Server* frontServer;
    QGuardedPtr<ChatWindow> m_frontView;
    QGuardedPtr<ChatWindow> previousFrontView;
    ChatWindow* searchView;

    UrlCatcher* urlCatcherPanel;
    DccPanel* dccPanel;
    bool dccPanelOpen;

    KToggleAction* showToolBarAction;
    KToggleAction* showStatusBarAction;
    KToggleAction* showMenuBarAction;
    KToggleAction* hideNicklistAction;
    
    NicksOnline* nicksOnlinePanel;

    DccTransferHandler* dccTransferHandler;

    Konversation::TrayIcon* tray;

    bool m_closeApp;
    
    SSLLabel* m_sslLabel;
    QLabel* m_channelInfoLabel;

    Konversation::InsertCharDialog* m_insertCharDialog;
    Konversation::ServerListDialog* m_serverListDialog;

    KPopupMenu* m_bookmarks;
    KonviBookmarkHandler* m_bookmarkHandler;
};

#endif
