/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverwindow.h  -  description
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

/* QT specific includes */
#include <qtabwidget.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qfile.h>

/* KDE specific includes */
#include <kmainwindow.h>
#include <kaction.h>
#include <kstdaction.h>

/*
  This is the Server Window Class. It shows a window
  with tab folders to store the channels and queries in.

  @author Dario Abatianni
*/

#include "server.h"
#include "ircview.h"
#include "ircinput.h"
#include "highlightdialog.h"
#include "quickbuttonsdialog.h"
#include "notifydialog.h"
#include "ignoredialog.h"
#include "notifydialog.h"
#include "ledtabwidget.h"
#include "colorconfiguration.h"
#include "chatwindow.h"
#include "statuspanel.h"
#include "rawlog.h"
#include "nicksonline.h"
#include "dccpanel.h"
#include "identity.h"

class ServerWindow : public KMainWindow
{
  Q_OBJECT

  public:
    ServerWindow(Server* server);
    ~ServerWindow();

    void appendToStatus(const QString& type,const QString& message);
    void appendToRaw(const QString& message);
    void appendToFrontmost(const QString& type,const QString& message);

    void addView(QWidget* pane,int color,const QString& name,bool on=true);
    void showView(QWidget* pane);

    LedTabWidget* getWindowContainer();

    DccPanel* getDccPanel();

    void setServer(Server* server);
    void setIdentity(const Identity *identity);
    bool isFrontView(const ChatWindow* view);

  signals:
    void prefsChanged();
    void openPrefsDialog();
    void closeChannel(const QString& name);
    void closeQuery(const QString& name);

  public slots:
    void addRawLog();
    void closeRawLog();
    void setNickname(const QString&);
    void newText(QWidget* view);
    void changedView(QWidget* view);
    void channelPrefsChanged();
    void resetLag();
    void updateLag(int msec);
    void updateFonts();
    void tooLongLag(int msec);

  protected slots:
    Server* getServer();
    
    void addStatusView();
    void nextTab();
    void previousTab();
    void closeTab(QWidget* tab);
    void addDccPanel();
    void closeDccPanel();      // remove dcc panel from view, but does not delete it
    void deleteDccPanel();     // deletes dcc panel from memory
    void showToolbar();
    void showStatusbar();
    void showMenubar();
    void openPreferences();
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

    void openNicksOnlineWindow();
    void closeNicksOnlineWindow(QSize newSize);
    void notifyAction(QListViewItem* item);
    void openButtons();
    void applyButtons(QStringList newList);
    void closeButtons(QSize newSize);

    void openColorConfiguration();
    void applyColorConfiguration(QString actionTextColor, QString backlogTextColor, QString channelTextColor,
                                 QString commandTextColor, QString linkTextColor, QString queryTextColor,
                                 QString serverTextColor, QString timeColor, QString backgroundColor);
    void closeColorConfiguration(QSize windowSize);

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
    int spacing();
    int margin();

    void readOptions();
    void saveOptions();
    bool queryExit();

    void goToTab(int page);

    enum StatusID
    {
      StatusText,LagOMeter
    };

    OutputFilter filter;
    LedTabWidget* windowContainer;
    StatusPanel* statusPanel;   // TODO: to be moved into Server class?

    ChatWindow* frontView;
    ChatWindow* searchView;

    RawLog* rawLog;     // the address of the raw log panel
    DccPanel* dccPanel; // the adress of the dcc panel
    bool dccPanelOpen;  // to track if a dcc panel is already open

    Server* server_;
    KToggleAction* showMenuBarAction;
    KToggleAction* showToolBarAction;
    KToggleAction* showStatusBarAction;
    HighlightDialog* hilightDialog;
    NotifyDialog* notifyDialog;
    IgnoreDialog* ignoreDialog;
    QuickButtonsDialog* buttonsDialog;
    ColorConfiguration* colorConfigurationDialog;
    NicksOnline* nicksOnlineWindow;
};

#endif
