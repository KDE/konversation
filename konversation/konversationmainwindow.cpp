/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationmainwindow.cpp  -  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni, Peter Simonsson
  email:     eisfuchs@tigress.com, psn@linux.se
*/

#include "konversationmainwindow.h"

#include <kaccel.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kmenubar.h>
#include <kkeydialog.h>
#include <kdeversion.h>
#include <kedittoolbar.h>
#include <kpopupmenu.h>

#ifndef KDE_MAKE_VERSION
#define KDE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))
#endif

#ifndef KDE_IS_VERSION
#define KDE_IS_VERSION(a,b,c) ( KDE_VERSION >= KDE_MAKE_VERSION(a,b,c) )
#endif

#if KDE_IS_VERSION(3,1,1)
#define USE_KNOTIFY
#include <knotifydialog.h>
#endif

#include "ledtabwidget.h"
#include "chatwindow.h"
#include "konversationapplication.h"
#include "ircview.h"
#include "server.h"
#include "statuspanel.h"
#include "channel.h"
#include "query.h"
#include "rawlog.h"
#include "channellistpanel.h"
#include "dccpanel.h"
#include "dcctransferhandler.h"
#include "nicksonline.h"
#include "konsolepanel.h"
#include "urlcatcher.h"
#include "irccolorchooser.h"
#include "trayicon.h"
#include "tabaction.h"
#include "dccchat.h"

KonversationMainWindow::KonversationMainWindow() : KMainWindow()
{
  // Init variables before anything else can happen
  frontView=0;
  searchView=0;
  frontServer=0;
  urlCatcherPanel=0;
  dccPanel=0;
  dccPanelOpen=false;
  m_closeApp = false;

  dccTransferHandler=new DccTransferHandler(this);

  nicksOnlinePanel=0;

  viewContainer=new LedTabWidget(this,"main_window_tab_widget");
  updateTabPlacement();

  setCentralWidget(viewContainer);

  KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); // file_quit

#if KDE_IS_VERSION(3, 1, 0)
  setStandardToolBarMenuEnabled(true);
#else
  showToolBarAction=KStdAction::showToolbar(this,SLOT(showToolbar()),actionCollection()); // options_show_toolbar
#endif
#if KDE_IS_VERSION(3, 1, 90)
  createStandardStatusBarAction();
#else
  showStatusBarAction=KStdAction::showStatusbar(this,SLOT(showStatusbar()),actionCollection()); // options_show_statusbar
#endif
  showMenuBarAction=KStdAction::showMenubar(this,SLOT(showMenubar()),actionCollection()); // options_show_menubar
  KStdAction::configureToolbars(this, SLOT(openToolbars()), actionCollection());
#ifdef USE_KNOTIFY
  KAction *configureNotificationsAction = KStdAction::configureNotifications(this,SLOT(openNotifications()), actionCollection());  // options_configure_notifications
#endif

  KStdAction::keyBindings(this,SLOT(openKeyBindings()),actionCollection()); // options_configure_key_binding
  KAction *preferencesAction = KStdAction::preferences(this,SLOT(openPreferences()),actionCollection()); // options_configure

  new KAction(i18n("Server List"), 0, 0, this, SLOT(openServerList()), actionCollection(), "open_server_list");
  new KAction(i18n("Quick Connect"), "connect_creating", 0, this, SLOT(openQuickConnectDialog()), actionCollection(), "quick_connect_dialog");

  new KAction(i18n("Watched Nicks Online"), 0, 0, this, SLOT(openNicksOnlinePanel()), actionCollection(), "open_nicksonline_window");
  new KAction(i18n("Open Logfile"), 0, 0, this, SLOT(openLogfile()), actionCollection(), "open_logfile");

  new KAction(i18n("Channel List"), 0, 0, this, SLOT(openChannelList()), actionCollection(), "open_channel_list");
  new KAction(i18n("URL Catcher"), 0, 0, this, SLOT(addUrlCatcher()), actionCollection(), "open_url_catcher");

  new KAction(i18n("New Konsole"), "openterm", 0, this, SLOT(addKonsolePanel()), actionCollection(), "open_konsole");

  // Actions to navigate through the different pages
  new KAction(i18n("Next Tab"), "next",KShortcut("Alt+Right"),this,SLOT(nextTab()),actionCollection(),"next_tab");
  new KAction(i18n("Previous Tab"), "previous",KShortcut("Alt+Left"),
    this,SLOT(previousTab()),actionCollection(),"previous_tab");
  new KAction(i18n("Close Tab"),"fileclose",KShortcut("Ctrl+w"),this,SLOT(closeTab()),actionCollection(),"close_tab");
  new TabAction(i18n("Go to Tab Number %1").arg( 1),0,KShortcut("Alt+1"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_1");
  new TabAction(i18n("Go to Tab Number %1").arg( 2),1,KShortcut("Alt+2"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_2");
  new TabAction(i18n("Go to Tab Number %1").arg( 3),2,KShortcut("Alt+3"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_3");
  new TabAction(i18n("Go to Tab Number %1").arg( 4),3,KShortcut("Alt+4"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_4");
  new TabAction(i18n("Go to Tab Number %1").arg( 5),4,KShortcut("Alt+5"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_5");
  new TabAction(i18n("Go to Tab Number %1").arg( 6),5,KShortcut("Alt+6"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_6");
  new TabAction(i18n("Go to Tab Number %1").arg( 7),6,KShortcut("Alt+7"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_7");
  new TabAction(i18n("Go to Tab Number %1").arg( 8),7,KShortcut("Alt+8"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_8");
  new TabAction(i18n("Go to Tab Number %1").arg( 9),8,KShortcut("Alt+9"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_9");
  new TabAction(i18n("Go to Tab Number %1").arg(10),9,KShortcut("Alt+0"),this,SLOT(goToTab(int)),actionCollection(),"go_to_tab_0");

  new KAction(i18n("Clear Window"),0,KShortcut("Ctrl+L"),this,SLOT(clearWindow()),actionCollection(),"clear_window");
  new KAction(i18n("Find Text..."),"find",KShortcut("F3"),this,SLOT(findTextShortcut()),actionCollection(),"find_text");
  new KAction(i18n("&Insert IRC Color..."), "colorize", CTRL+Key_K, this, SLOT(addIRCColor()), actionCollection(), "irc_colors");
  new KAction(i18n("Insert &Remember Line"), 0,  KShortcut("Ctrl+R") , this, SLOT(insertRememberLine()), actionCollection(), "insert_remember_line");
  new KAction(i18n("Close All Open Queries"), 0, KShortcut("F11"), this, SLOT(closeQueries()), actionCollection(), "close_queries");

  // Initialize KMainWindow->statusBar()
  statusBar();
  statusBar()->insertItem(i18n("Ready."),StatusText,1);
  statusBar()->insertItem("lagometer",LagOMeter,0,true);
  // Show "Lag unknown"
  resetLag();
  statusBar()->setItemAlignment(StatusText,QLabel::AlignLeft);

  // Initialize KMainWindow->menuBar()
  showMenubar();

  connect( viewContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changeView(QWidget*)) );
  connect( viewContainer,SIGNAL (closeTab(QWidget*)),this,SLOT (closeView(QWidget*)) );
  connect(this, SIGNAL (closeTab(int)), viewContainer, SLOT (tabClosed(int)));

  // set up system tray
  tray = new TrayIcon(this);
  connect(this, SIGNAL(startNotification(QWidget*)), tray, SLOT(startNotification(QWidget*)));
  connect(this, SIGNAL(endNotification(QWidget*)), tray, SLOT(endNotification(QWidget*)));
  connect(tray, SIGNAL(quitSelected()), this, SLOT(quitProgram()));
  KPopupMenu *trayMenu = tray->contextMenu();
#ifdef USE_KNOTIFY
  configureNotificationsAction->plug(trayMenu);
#endif
  preferencesAction->plug(trayMenu);

  // decide whether to show the tray icon or not
  updateTrayIcon();

  createGUI();
  resize(700, 500);  // Give the app a sane default size
  setAutoSaveSettings();
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 0)
  showToolBarAction->setChecked(KonversationApplication::preferences.getShowToolBar());
  showToolbar();
#endif
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 90)
  showStatusBarAction->setChecked(KonversationApplication::preferences.getShowStatusBar());
  showStatusbar();
#endif
  showMenuBarAction->setChecked(KonversationApplication::preferences.getShowMenuBar());
  showMenubar();
}

KonversationMainWindow::~KonversationMainWindow()
{
  deleteDccPanel();
  if(dccTransferHandler) delete dccTransferHandler;
}

void KonversationMainWindow::updateTabPlacement()
{
  viewContainer->setTabPosition((KonversationApplication::preferences.getTabPlacement()==Preferences::Top) ?
                                 QTabWidget::Top : QTabWidget::Bottom);
}

void KonversationMainWindow::openPreferences()
{
  emit openPrefsDialog();
}

void KonversationMainWindow::openKeyBindings()
{
  KKeyDialog::configure(actionCollection());
}

void KonversationMainWindow::showToolbar()
{
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 0)
  if(showToolBarAction->isChecked()) toolBar("mainToolBar")->show();
  else toolBar("mainToolBar")->hide();

  KonversationApplication::preferences.setShowToolBar(showToolBarAction->isChecked());
#endif
}

void KonversationMainWindow::showMenubar()
{
  if(showMenuBarAction->isChecked()) menuBar()->show();
  else
  {
    QString accel=showMenuBarAction->shortcut().toString();
    KMessageBox::information(this,i18n("<qt>This will hide the menu bar completely."
                                       "You can show it again by typing %1.</qt>").arg(accel),
                                       "Hide menu bar","HideMenuBarWarning");
    menuBar()->hide();
  }

  KonversationApplication::preferences.setShowMenuBar(showMenuBarAction->isChecked());
}

void KonversationMainWindow::showStatusbar()
{
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 90)
  if(showStatusBarAction->isChecked()) statusBar()->show();
  else statusBar()->hide();

  KonversationApplication::preferences.setShowStatusBar(showStatusBarAction->isChecked());
#endif
}

void KonversationMainWindow::appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView)
{
  // TODO: Make it an option to direct all status stuff into the status panel

  updateFrontView();
  if(frontView==0 ||              // Check if the frontView can actually display text or ...
     serverView->getServer()!=frontView->getServer())  // if it does not belong to this server
  {
    // if not, take server specified fallback view instead
    serverView->appendServerMessage(type,message);
    // FIXME: this signal should be sent from the status panel instead, so it
    //        can be using the correct highlight color, would be more consistent
    //        anyway!
    newText(serverView,QString::null,true);
  }
  else
    frontView->appendServerMessage(type,message);
}

void KonversationMainWindow::addView(ChatWindow* view,int color,const QString& label,bool on)
{
  // TODO: Make sure to add DCC status tab at the end of the list and all others
  // before the DCC tab. Maybe we should also make sure to order Channels
  // Queries and DCC chats in groups
  viewContainer->addTab(view,label,color,on);

  // Check, if user was typing in old input line
  bool doBringToFront=true;

  // make sure that bring to front only works when the user wasn't typing something
  if(frontView)
  {
    if(!frontView->getTextInLine().isEmpty()) doBringToFront=false;
  }

  // bring view to front unless it's a raw log window or the user was typing
  if(KonversationApplication::preferences.getBringToFront() && doBringToFront &&
    view->getType()!=ChatWindow::RawLog)
  {
    showView(view);
  }

  connect(view,SIGNAL (online(ChatWindow*,bool)),viewContainer,SLOT (setTabOnline(ChatWindow*,bool)) );
}

void KonversationMainWindow::showView(ChatWindow* view)
{
  // Don't bring Tab to front if TabWidget is hidden. Otherwise QT gets confused
  // and shows the Tab as active but will display the wrong pane
  if(viewContainer->isVisible())
  {
    // TODO: add adjustFocus() here?
    viewContainer->showPage(view);
  }
}

void KonversationMainWindow::closeView(QWidget* viewToClose)
{
  ChatWindow* view=static_cast<ChatWindow*>(viewToClose);
  if(view)
  {
    // if this view was the front view, delete the pointer
    if(view==frontView) frontView=0;
    
    emit endNotification(viewToClose);
    
    ChatWindow::WindowType viewType=view->getType();

    QString viewName=view->getName();
    // the views should know by themselves how to close

    if(viewType==ChatWindow::Status)            view->closeYourself();
    else if(viewType==ChatWindow::Channel)      view->closeYourself();
    else if(viewType==ChatWindow::ChannelList)  view->closeYourself();
    else if(viewType==ChatWindow::Query)        view->closeYourself();
    else if(viewType==ChatWindow::RawLog)       view->closeYourself();
    else if(viewType==ChatWindow::DccChat)      view->closeYourself();

    else if(viewType==ChatWindow::DccPanel)     closeDccPanel();
    else if(viewType==ChatWindow::Konsole)      closeKonsolePanel(view);
    else if(viewType==ChatWindow::UrlCatcher)   closeUrlCatcher();
    else if(viewType==ChatWindow::NicksOnline)  closeNicksOnlinePanel();

/*
    else if(viewType==ChatWindow::Notice);
    else if(viewType==ChatWindow::SNotice);
*/
  }
}

void KonversationMainWindow::openLogfile()
{
  if(frontView)
  {
    ChatWindow* view=static_cast<ChatWindow*>(frontView);
    ChatWindow::WindowType viewType=view->getType();
    if(viewType==ChatWindow::Channel ||
       viewType==ChatWindow::Query ||
       viewType==ChatWindow::Status)
    {
      view->openLogfile();
    }
    else
    {
    }
  }
}

void KonversationMainWindow::addKonsolePanel()
{
  KonsolePanel* panel=new KonsolePanel(getViewContainer());
  addView(panel,3,i18n("Konsole"));
  connect(panel,SIGNAL (deleted(ChatWindow*)),this,SLOT (closeKonsolePanel(ChatWindow*)) );
}

void KonversationMainWindow::closeKonsolePanel(ChatWindow* konsolePanel)
{
  getViewContainer()->removePage(konsolePanel);
  // tell QT to delete the panel during the next event loop since we are inside a signal here
  konsolePanel->deleteLater();
}

void KonversationMainWindow::openChannelList()
{
  if(frontServer)
  {
    ChannelListPanel* panel=frontServer->getChannelListPanel();
    if(panel)
      getViewContainer()->showPage(panel);
    else
      frontServer->addChannelListPanel();
  }
  else
  {
    KMessageBox::information(this,
                             i18n(
                                  "The channel list can only be opened from a "
                                  "query, channel or status window to find out, "
                                  "which server this list belongs to."
                                 ),
                             i18n("Channel List"),
                             "ChannelListNoServerSelected");
  }
}

void KonversationMainWindow::addUrlCatcher()
{
  // if the panel wasn't open yet
  if(urlCatcherPanel==0)
  {
    urlCatcherPanel=new UrlCatcher(getViewContainer());
    addView(urlCatcherPanel,2,i18n("URL Catcher"),true);

    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    connect(konvApp,SIGNAL (catchUrl(const QString&,const QString&)),
        urlCatcherPanel,SLOT (addUrl(const QString&,const QString&)) );
    connect(urlCatcherPanel,SIGNAL (deleteUrl(const QString&,const QString&)),
                      konvApp,SLOT (deleteUrl(const QString&,const QString&)) );
    connect(urlCatcherPanel,SIGNAL (clearUrlList()),
                      konvApp,SLOT (clearUrlList()) );

    QStringList urlList=konvApp->getUrlList();
    for(unsigned int index=0;index<urlList.count();index++)
    {
      QString urlItem=urlList[index];
      urlCatcherPanel->addUrl(urlItem.section(' ',0,0),urlItem.section(' ',1,1));
    } // for
  }
}

void KonversationMainWindow::closeUrlCatcher()
{
  // if there actually is a dcc panel
  if(urlCatcherPanel)
  {
    delete urlCatcherPanel;
    urlCatcherPanel=0;
  }
}

void KonversationMainWindow::addDccPanel()
{
  // if the panel wasn't open yet
  if(dccPanel==0)
  {
    dccPanel=new DccPanel(getViewContainer());
    addView(dccPanel,3,i18n("DCC Status"));
    dccPanelOpen=true;
  }
  // show already opened panel
  else
  {
    if(!dccPanelOpen)
    {
      addView(dccPanel,3,i18n("DCC Status"));
      dccPanelOpen=true;
    }
    // no highlight color for DCC panels
    newText(dccPanel,QString::null,true);
  }
}

DccPanel* KonversationMainWindow::getDccPanel()
{
  return dccPanel;
}

void KonversationMainWindow::closeDccPanel()
{
  // if there actually is a dcc panel
  if(dccPanel)
  {
     // hide it from view, does not delete it
    getViewContainer()->removePage(dccPanel);
    dccPanelOpen=false;
  }
}

void KonversationMainWindow::deleteDccPanel()
{
  if(dccPanel)
  {
    closeDccPanel();
    delete dccPanel;
    dccPanel=0;
  }
}

void KonversationMainWindow::addDccChat(const QString& myNick,const QString& nick,const QString& numericalIp,const QStringList& arguments,bool listen)
{
  if(frontServer)
  {
    DccChat* dccChatPanel=new DccChat(getViewContainer(),frontServer,myNick,nick,arguments,listen);
    addView(dccChatPanel,3,dccChatPanel->getName());

    connect(dccChatPanel,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );

    if(listen) frontServer->queue(QString("PRIVMSG %1 :\x01%2 CHAT chat %3 %4\x01").arg(nick).arg("DCC").arg(numericalIp).arg(dccChatPanel->getPort()));
  }
}

StatusPanel* KonversationMainWindow::addStatusView(Server* server)
{
  StatusPanel* statusView=new StatusPanel(getViewContainer());

  // first set up internal data ...
  statusView->setServer(server);
  statusView->setIdentity(server->getIdentity());
  statusView->setName(server->getServerName());

  // ... then put it into the tab widget, otherwise we'd have a race with server member
  addView(statusView,2,server->getServerName(),false);

  connect(statusView,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );
  connect(statusView,SIGNAL (sendFile()),server,SLOT (requestDccSend()) );
  connect(statusView,SIGNAL (prefsChanged()),this,SLOT (channelPrefsChanged()) );
  connect(server,SIGNAL (awayState(bool)),statusView,SLOT (indicateAway(bool)) );

  // make sure that frontServer gets set on adding the first status panel, too,
  // since there won't be a changeView happening
  if(!frontServer) frontServer=server;

  return statusView;
}

Channel* KonversationMainWindow::addChannel(Server* server, const QString& name)
{
  Channel* channel=new Channel(getViewContainer());
  channel->setServer(server);
  channel->setName(name);

  addView(channel,1,name);

  connect(channel,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );
  connect(channel,SIGNAL (prefsChanged()),this,SLOT (channelPrefsChanged()) );
  connect(server,SIGNAL (awayState(bool)),channel,SLOT (indicateAway(bool)) );

  return channel;
}

Query* KonversationMainWindow::addQuery(Server* server, const QString& name)
{
  Query* query=new Query(getViewContainer());
  query->setServer(server);
  query->setName(name);

  addView(query,0,name);

  connect(query,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );
  connect(server,SIGNAL (awayState(bool)),query,SLOT (indicateAway(bool)) );

  return query;
}

RawLog* KonversationMainWindow::addRawLog(Server* server)
{
  RawLog* rawLog=new RawLog(getViewContainer());

  rawLog->setServer(server);
  rawLog->setLog(false);

  addView(rawLog,2,i18n("Raw Log"),false);

  return rawLog;
}

ChannelListPanel* KonversationMainWindow::addChannelListPanel(Server* server)
{
  ChannelListPanel* channelListPanel=new ChannelListPanel(getViewContainer());
  channelListPanel->setServer(server);

  addView(channelListPanel,2,i18n("Channel List"));

  return channelListPanel;
}

void KonversationMainWindow::newText(QWidget* widget,const QString& highlightColor,bool important)
{
  ChatWindow* view=static_cast<ChatWindow*>(widget);

  if(view!=getViewContainer()->currentPage())
  {
    getViewContainer()->changeTabState(view,true,important,highlightColor);

    emit startNotification(view);
  }
  else if(!isActiveWindow() && view->getServer() && view->getServer()->connected())
  {
    emit startNotification(view);
  }
}

void KonversationMainWindow::updateFrontView()
{
  ChatWindow* view=static_cast<ChatWindow*>(getViewContainer()->currentPage());

  if(view)
  {
    // Make sure that only views with info output get to be the frontView
    if(view->frontView()) frontView=view;
    // Make sure that only text views get to be the searchView
    if(view->searchView()) searchView=view;
  }
}

void KonversationMainWindow::changeView(QWidget* viewToChange)
{
  frontView=0;
  searchView=0;

  ChatWindow* view=static_cast<ChatWindow*>(viewToChange);

  // display this server's lag time
  frontServer=view->getServer();
  if(frontServer) updateLag(frontServer,frontServer->getLag());

  updateFrontView();

  viewContainer->changeTabState(view,false,false,QString::null);
  emit endNotification(viewToChange);
}

bool KonversationMainWindow::queryClose()
{
  // send quit to all servers
  emit quitServer();

  return true;
}

void KonversationMainWindow::quitProgram()
{
  // will call queryClose()
  m_closeApp = true;
  close();
}

void KonversationMainWindow::openNicksOnlinePanel()
{
  if(!nicksOnlinePanel)
  {

    nicksOnlinePanel=new NicksOnline(getViewContainer());
    addView(nicksOnlinePanel, 2, i18n("Nicks Online"), true);

    connect(nicksOnlinePanel,SIGNAL (editClicked()),this,SLOT (openNotify()) );

    connect(nicksOnlinePanel,SIGNAL (doubleClicked(const QString&,const QString&)),this,SLOT (notifyAction(const QString&,const QString&)) );

    connect(this,SIGNAL (nicksNowOnline(const QString&,const QStringList&,bool)),nicksOnlinePanel,SLOT (setOnlineList(const QString&,const QStringList&,bool)) );
  }
}

void KonversationMainWindow::closeNicksOnlinePanel()
{
  if ( nicksOnlinePanel )
  {
    delete nicksOnlinePanel;
    nicksOnlinePanel=0;
  }
}

void KonversationMainWindow::openServerList()
{
  emit openPrefsDialog(Preferences::ServerListPage);
}

void KonversationMainWindow::openQuickConnectDialog()
{
	emit showQuickConnectDialog();
}

void KonversationMainWindow::openNotify()
{
  emit openPrefsDialog(Preferences::NotifyPage);
}

// TODO: Let an own class handle notify things
void KonversationMainWindow::setOnlineList(Server* notifyServer,const QStringList& list, bool changed)
{
  emit nicksNowOnline(notifyServer->getServerName(),list,changed);
  if (changed && nicksOnlinePanel) newText(nicksOnlinePanel, QString::null, true);
}

void KonversationMainWindow::notifyAction(const QString& serverName,const QString& nick)
{
  KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
  Server* server=konv_app->getServerByName(serverName);
  server->notifyAction(nick);
}

void KonversationMainWindow::channelPrefsChanged()
{
  emit prefsChanged();
}

LedTabWidget* KonversationMainWindow::getViewContainer()
{
  return viewContainer;
}

void KonversationMainWindow::updateFonts()
{
  getViewContainer()->updateTabs();
}

void KonversationMainWindow::updateLag(Server* lagServer,int msec)
{
  // show lag only of actual server
  if(lagServer==frontServer)
  {
    statusBar()->changeItem(i18n("Ready."),StatusText);

    QString lagString(i18n("Lag: %1 ms").arg(msec));
    statusBar()->changeItem(lagString,LagOMeter);
  }
}

void KonversationMainWindow::tooLongLag(Server* lagServer,int msec)
{
  if((msec % 5000)==0)
  {
    QString lagString(i18n("No answer from server %1 for more than %2 seconds").arg(lagServer->getServerName()).arg(msec/1000));
    statusBar()->changeItem(lagString,StatusText);
  }

  QString lagString(i18n("Lag: %1 s").arg(msec/1000));
  statusBar()->changeItem(lagString,LagOMeter);
}

// TODO: Make this server dependant
void KonversationMainWindow::resetLag()
{
  statusBar()->changeItem(i18n("Lag: not known"),LagOMeter);
}

void KonversationMainWindow::closeTab()
{
  // -1 = close currently visible tab
  emit closeTab(-1);
}

void KonversationMainWindow::nextTab()
{
  goToTab(getViewContainer()->currentPageIndex()+1);
}

void KonversationMainWindow::previousTab()
{
  goToTab(getViewContainer()->currentPageIndex()-1);
}

void KonversationMainWindow::goToTab(int page)
{
  if(page>=0 && page<getViewContainer()->count())
  {
    getViewContainer()->setCurrentPage(page);
    ChatWindow* newPage=static_cast<ChatWindow*>(getViewContainer()->page(page));
    newPage->adjustFocus();
  }
}

void KonversationMainWindow::findTextShortcut()
{
  if(searchView==0)
  {
    KMessageBox::sorry(this,
                       i18n("You can only search in text fields."),
                       i18n("Find Text Information"));
  }
  else
  {
    searchView->getTextView()->search();
  }
}

void KonversationMainWindow::clearWindow()
{
  if (frontView)
    frontView->getTextView()->clear();
}

void KonversationMainWindow::openNotifications()
{
#ifdef USE_KNOTIFY
  (void) KNotifyDialog::configure(this);
#endif
}

void KonversationMainWindow::updateTrayIcon()
{
  if(KonversationApplication::preferences.getShowTrayIcon())
    tray->show();
  else
    tray->hide();

  tray->setNotificationEnabled(KonversationApplication::preferences.getTrayNotify());
}

void KonversationMainWindow::addIRCColor()
{
  IRCColorChooser dlg(this, &(KonversationApplication::preferences));

  if(dlg.exec() == QDialog::Accepted) {
    frontView->appendInputText(dlg.color());
  }
}

void KonversationMainWindow::insertRememberLine()
{
  if(KonversationApplication::preferences.getShowRememberLineInAllWindows())
  {
    int total = getViewContainer()->count()-1;
    ChatWindow* nextPage;
    
    for(int i = 0; i <= total; ++i)
    {        
      nextPage = static_cast<ChatWindow*>(getViewContainer()->page(i));
      if(nextPage->getType() == ChatWindow::Channel ||
          nextPage->getType() == ChatWindow::Query)
      {
         nextPage->insertRememberLine();
      }
    }
  }
    
  else
  {
    if(frontView->getType() == ChatWindow::Channel ||
        frontView->getType() == ChatWindow::Query)
    {
      frontView->insertRememberLine();
    }
  }
}

void KonversationMainWindow::closeQueries()
{
  int total=getViewContainer()->count()-1;
  int operations=0;
  ChatWindow* nextPage;

  for(int i=0;i<=total;i++)
  {
    if (operations > total)
      break;

    nextPage=static_cast<ChatWindow*>(getViewContainer()->page(i));

    if(nextPage && nextPage->getType()==ChatWindow::Query) {
      nextPage->closeYourself();
      --i; /* Tab indexes changed */
    }
    ++operations;
  }
}

bool KonversationMainWindow::event(QEvent* e)
{
  if(e->type() == QEvent::WindowActivate) {
    emit endNotification(getViewContainer()->currentPage());
  }

  return KMainWindow::event(e);
}

void KonversationMainWindow::serverQuit(Server* server)
{
  if(server == frontServer) {
    frontServer = 0;
  }

  if(frontView->getServer() == server) {
    frontView = 0;
  }

  tray->removeServer(server);
  delete server->getStatusView();
  delete server;
}

void KonversationMainWindow::openToolbars()
{
  KEditToolbar dlg(actionCollection());

  if (dlg.exec())
  {
    createGUI();
  }
}

#if QT_VERSION >= 0x030200
void KonversationMainWindow::setShowTabBarCloseButton(bool s)
{
  if(s) {
    viewContainer->cornerWidget()->show();
  } else {
    viewContainer->cornerWidget()->hide();
  }
}
#else
void KonversationMainWindow::setShowTabBarCloseButton(bool) {}
#endif

void KonversationMainWindow::closeEvent(QCloseEvent* e)
{
  KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
  if ( konv_app->sessionSaving() ) m_closeApp = true;
  
  if(KonversationApplication::preferences.getShowTrayIcon() && !m_closeApp) {
    // Message copied from kopete...
    KMessageBox::information(this,
      i18n( "<qt>Closing the main window will keep Konversation running in the "
      "system tray. Use 'Quit' from the 'File' menu to quit the application.</qt>" ),
      i18n( "Docking in System Tray" ), "hideOnCloseInfo");
  
    hide();
    e->ignore();
  } else {
    KMainWindow::closeEvent(e);
  }
}

#include "konversationmainwindow.moc"
