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

#include <kmainwindow.h>
#include <kdeversion.h>
#include <kdeversion.h>

#include "preferences.h"

/*
 Dario Abatianni
*/

class KToggleAction;

class LedTabWidget;
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
    Query* addQuery(Server* server,const QString& name);

    DccPanel* getDccPanel();
    void showView(ChatWindow* view);

    void appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView);

    void updateFonts();
    void updateTabPlacement();

  signals:
    void prefsChanged();
    void startNotifyTimer(int msec);
    void openPrefsDialog();
    void openPrefsDialog(Preferences::Pages page);
    void quitServer();
    void nicksNowOnline(const QString& serverName,const QStringList& list);
    void closeTab(int id);
    void startNotification(QWidget*);
    void endNotification(QWidget*);

  public slots:
    void addDccPanel();     // connected in server class
    void addKonsolePanel(); // connected in server class
    void addUrlCatcher();
    void addDccChat(const QString& myNick,const QString& nick,const QString& numericalIp,const QStringList& arguments,bool listen);

    void resetLag();
    void updateLag(Server* lagServer,int msec);
    void tooLongLag(Server* lagServer,int msec);
    void channelPrefsChanged();
    void setOnlineList(Server* notifyServer,const QStringList& list);
    void updateTrayIcon();
    void serverQuit(Server* server);
    void setShowTabBarCloseButton(bool s);

  protected slots:
    void openPreferences();
    void openKeyBindings();
    void openServerList();
    void openChannelList();
    void openNotify();
    void openNicksOnlineWindow();
    void closeNicksOnlineWindow(QSize newSize);
    // it seems that moc does not honor #ifs in compile so we create an
    // empty slot in our .cpp file rather than #if this slot out
    void openNotifications();
    void openToolbars();

    void showToolbar();
    void showStatusbar();
    void showMenubar();
    void changeView(QWidget* view);
    void closeView(QWidget* view);

    void closeKonsolePanel(ChatWindow* konsolePanel);

    void newText(QWidget* view,const QString& highlightColor,bool important);
    void quitProgram();

    void notifyAction(const QString& serverName,const QString& nick);

    void nextTab();
    void previousTab();
    void closeTab();

    void goToTab(int page);

    void findTextShortcut();
    void addIRCColor();
    void clearWindow();

  protected:
    enum StatusID
    {
      StatusText,LagOMeter
    };

    bool queryClose();

    void addView(ChatWindow* view,int color,const QString& label,bool on=true);
    void updateFrontView();

    void closeUrlCatcher();
    void closeDccPanel();
    void deleteDccPanel();
    
    virtual bool event(QEvent* e);

    LedTabWidget* getViewContainer();

    LedTabWidget* viewContainer;

    Server* frontServer;
    ChatWindow* frontView;
    ChatWindow* searchView;

    UrlCatcher* urlCatcherPanel;
    DccPanel* dccPanel;
    bool dccPanelOpen;

    KToggleAction* showToolBarAction;
    KToggleAction* showStatusBarAction;
    KToggleAction* showMenuBarAction;

    NicksOnline* nicksOnlineWindow;
    QStringList nicksOnlineList;

    DccTransferHandler* dccTransferHandler;

    TrayIcon* tray;
};

#endif
