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

  $Id$
*/


#ifndef _KONVERSATIONMAINWINDOW_H_
#define _KONVERSATIONMAINWINDOW_H_

#include <qstringlist.h>

#include <kmainwindow.h>

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
class Highlight;
class HighlightDialog;
class Ignore;
class IgnoreDialog;
class NicksOnline;
class NotifyDialog;
class QuickButtonsDialog;
class UrlCatcher;

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
    void channelQuickButtonsChanged();
    void startNotifyTimer(int msec);
    void openPrefsDialog();
    void quitServer();
    void nicksNowOnline(const QString& serverName,const QStringList& list);

  public slots:
    void addDccPanel();     // connected in server class
    void addKonsolePanel(); // connected in server class
    void addUrlCatcher();

    void resetLag();
    void updateLag(Server* lagServer,int msec);
    void tooLongLag(Server* lagServer,int msec);
    void channelPrefsChanged();
    void setOnlineList(Server* notifyServer,const QStringList& list);

  protected slots:
    void openPreferences();
    void openKeyBindings();
    void showToolbar();
    void showStatusbar();
    void showMenubar();
    void changeView(QWidget* view);
    void closeView(QWidget* view);

    void closeKonsolePanel(ChatWindow* konsolePanel);

    void newText(QWidget* view,const QString& highlightColor);
    void quitProgram();

    void openHilight();
    void applyHilight(QPtrList<Highlight> newList);
    void closeHilight(QSize newSize);

    void openIgnore();
    void applyIgnore(QPtrList<Ignore> newList);
    void closeIgnore(QSize newSize);

    void openNotify();
    void applyNotify(QStringList newList,bool use,int delay);
    void closeNotify(QSize newSize);
    void notifyAction(const QString& serverName,const QString& nick);

    void openNicksOnlineWindow();
    void closeNicksOnlineWindow(QSize newSize);
    void openButtons();
    void applyButtons(QStringList newList);
    void closeButtons(QSize newSize);

    void openChannelList();

    void nextTab();
    void previousTab();

    // I hope we find a better way soon!
    void goToTab0();
    void goToTab1();
    void goToTab2();
    void goToTab3();
    void goToTab4();
    void goToTab5();
    void goToTab6();
    void goToTab7();
    void goToTab8();
    void goToTab9();

    void findTextShortcut();

  protected:
    enum StatusID
    {
      StatusText,LagOMeter
    };

    void readOptions();
    void saveOptions();
    bool queryClose();

    void addView(ChatWindow* view,int color,const QString& label,bool on=true);
    void updateFrontView();

    void goToTab(int page);

    void closeUrlCatcher();
    void closeDccPanel();
    void deleteDccPanel();

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

    HighlightDialog* hilightDialog;
    NotifyDialog* notifyDialog;
    IgnoreDialog* ignoreDialog;
    QuickButtonsDialog* buttonsDialog;
    NicksOnline* nicksOnlineWindow;
    QStringList nicksOnlineList;

    DccTransferHandler* dccTransferHandler;
};

#endif
