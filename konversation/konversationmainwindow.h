/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationmainwindow.h  -  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#ifndef _KONVERSATIONMAINWINDOW_H_
#define _KONVERSATIONMAINWINDOW_H_

#include <qstringlist.h>

#ifdef USE_MDI
#include <kmdimainfrm.h>
#include "images.h"
#define MAIN_TYPE KMdiMainFrm
#else
#include <kmainwindow.h>
#define MAIN_TYPE KMainWindow
#endif

#include "preferences.h"

/*
 Dario Abatianni
*/

class KToggleAction;
class KMdiChildView; // USE_MDI

class Server;
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
class TrayIcon;
#ifndef USE_MDI
class LedTabWidget;
#endif

namespace Konversation
{
  class InsertCharDialog;
}

class KonversationMainWindow : public MAIN_TYPE // USE_MDI
{
  Q_OBJECT

  public:
    KonversationMainWindow();
    ~KonversationMainWindow();

    StatusPanel* addStatusView(Server* server);
    RawLog* addRawLog(Server* server);
    ChannelListPanel* addChannelListPanel(Server* server);
    Channel* addChannel(Server* server,const QString& name);
    Query* addQuery(Server* server,const QString& name, bool weinitiated=true);

    DccPanel* getDccPanel();
    void showView(ChatWindow* view);

    void appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView);
    void appendToFrontmostIfDifferent(const QString& type,const QString& message,ChatWindow* serverView);

    void updateFonts();
    void updateTabPlacement();

  signals:
    void prefsChanged();
    void startNotifyTimer(int msec);
    void openPrefsDialog();
    void openPrefsDialog(Preferences::Pages page);
    void showQuickConnectDialog();
    void quitServer();
    void nicksNowOnline(const QString& serverName,const QStringList& list,bool changed);
    void closeTab(int id);
    void startNotification(QWidget*);
    void endNotification(QWidget*);

  public slots:
    void addDccPanel();     // connected in server class
    void addKonsolePanel(); // connected in server class
    void addUrlCatcher();
    void addDccChat(const QString& myNick,const QString& nick,const QString& numericalIp,const QStringList& arguments,bool listen);
    void insertRememberLine();

    void resetLag();
    void updateLag(Server* lagServer,int msec);
    void tooLongLag(Server* lagServer,int msec);
    void channelPrefsChanged();
    void setOnlineList(Server* notifyServer,const QStringList& list, bool changed);
    void updateTrayIcon();
    void serverQuit(Server* server);
    void setShowTabBarCloseButton(bool s);
    
    void openLogFile(const QString& caption, const QString& file);

    virtual void switchToTabPageMode();    // USE_MDI
    virtual void switchToIDEAlMode();      // USE_MDI
    virtual void switchToChildframeMode(); // USE_MDI
    virtual void switchToToplevelMode();   // USE_MDI
    
    void slotPrefsChanged();

  protected slots:
    void openPreferences();
    void openKeyBindings();
    void openServerList();
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
    void showMenubar();

    void changeView(QWidget* view);
    void closeView(QWidget* view);

    void changeToView(KMdiChildView* view); // USE_MDI
    void setWindowNotification(ChatWindow* view,const QIconSet& iconSet,const QString& color); // USE_MDI
    void closeWindow(ChatWindow* view); // USE_MDI
    void closeActiveWindow(); // USE_MDI
    void setTabOnline(ChatWindow* view,bool online); // USE_MDI

    void closeKonsolePanel(ChatWindow* konsolePanel);

    void newText(QWidget* view,const QString& highlightColor,bool important);
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
    
    void insertCharacter();
    void insertChar(const QChar& chr);

  protected:
    enum StatusID
    {
      StatusText,LagOMeter
    };

    bool queryClose();

#ifdef USE_MDI
    void addMdiView(ChatWindow* view,int color,bool on=true, bool weinitiated=true);
#else
    void addView(ChatWindow* view,int color,const QString& label,bool on=true, bool weinitiated=true);
#endif
    void updateFrontView();

    void closeUrlCatcher();
    void closeDccPanel();
    void deleteDccPanel();

    virtual bool event(QEvent* e);
    virtual void resizeEvent(QResizeEvent* ev);

#ifdef USE_MDI
    Images images;
#else
    LedTabWidget* getViewContainer();
    LedTabWidget* viewContainer;
#endif

    Server* frontServer;
    QGuardedPtr<ChatWindow> frontView;
    QGuardedPtr<ChatWindow> previousFrontView;
    ChatWindow* searchView;

    UrlCatcher* urlCatcherPanel;
    DccPanel* dccPanel;
    bool dccPanelOpen;

    KToggleAction* showToolBarAction;
    KToggleAction* showStatusBarAction;
    KToggleAction* showMenuBarAction;

    NicksOnline* nicksOnlinePanel;

    DccTransferHandler* dccTransferHandler;

    TrayIcon* tray;

    bool m_closeApp;
    
    Konversation::InsertCharDialog* m_insertCharDialog;
};

#endif
