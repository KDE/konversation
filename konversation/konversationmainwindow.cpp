/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationmainwindow.cpp  -  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
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
#if KDE_VERSION >= 310
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

KonversationMainWindow::KonversationMainWindow() : KMainWindow()
{
  kdDebug() << "KonversationMainWindow::KonversationMainWindow()" << endl;

  // Init variables before anything else can happen
  frontView=0;
  searchView=0;
  frontServer=0;
  urlCatcherPanel=0;
  dccPanel=0;
  dccPanelOpen=false;

  dccTransferHandler=new DccTransferHandler(this);

  nicksOnlineWindow=0;

  viewContainer=new LedTabWidget(this,"main_window_tab_widget");
  updateTabPlacement();

  setCentralWidget(viewContainer);

  KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); // file_quit

  showToolBarAction=KStdAction::showToolbar(this,SLOT(showToolbar()),actionCollection()); // options_show_toolbar
  showStatusBarAction=KStdAction::showStatusbar(this,SLOT(showStatusbar()),actionCollection()); // options_show_statusbar
  showMenuBarAction=KStdAction::showMenubar(this,SLOT(showMenubar()),actionCollection()); // options_show_menubar
#if KDE_VERSION >= 310
  KStdAction::configureNotifications(this,SLOT(openNotifications()), actionCollection());  // options_configure_notifications
#endif

  KStdAction::keyBindings(this,SLOT(openKeyBindings()),actionCollection()); // options_configure_key_binding
  KStdAction::preferences(this,SLOT(openPreferences()),actionCollection()); // options_configure

  new KAction(i18n("Server List"), 0, 0, this, SLOT(openServerList()), actionCollection(), "open_server_list");

  new KAction(i18n("Nicks Online"), 0, 0, this, SLOT(openNicksOnlineWindow()), actionCollection(), "open_nicksonline_window");
  new KAction(i18n("Channel List"), 0, 0, this, SLOT(openChannelList()), actionCollection(), "open_channel_list");
  new KAction(i18n("Open Konsole"), 0, 0, this, SLOT(addKonsolePanel()), actionCollection(), "open_konsole");
  new KAction(i18n("Open URL Catcher"), 0, 0, this, SLOT(addUrlCatcher()), actionCollection(), "open_url_catcher");

  // Actions to navigate through the different pages
  new KAction(i18n("Next Tab"),0,KShortcut("Alt+Right"),this,SLOT(nextTab()),actionCollection(),"next_tab");
  new KAction(i18n("Previous Tab"),0,KShortcut("Alt+Left"),this,SLOT(previousTab()),actionCollection(),"previous_tab");
  new KAction(i18n("Go to Tab Number %1").arg( 1),0,KShortcut("Alt+1"),this,SLOT(goToTab0()),actionCollection(),"go_to_tab_1");
  new KAction(i18n("Go to Tab Number %1").arg( 2),0,KShortcut("Alt+2"),this,SLOT(goToTab1()),actionCollection(),"go_to_tab_2");
  new KAction(i18n("Go to Tab Number %1").arg( 3),0,KShortcut("Alt+3"),this,SLOT(goToTab2()),actionCollection(),"go_to_tab_3");
  new KAction(i18n("Go to Tab Number %1").arg( 4),0,KShortcut("Alt+4"),this,SLOT(goToTab3()),actionCollection(),"go_to_tab_4");
  new KAction(i18n("Go to Tab Number %1").arg( 5),0,KShortcut("Alt+5"),this,SLOT(goToTab4()),actionCollection(),"go_to_tab_5");
  new KAction(i18n("Go to Tab Number %1").arg( 6),0,KShortcut("Alt+6"),this,SLOT(goToTab5()),actionCollection(),"go_to_tab_6");
  new KAction(i18n("Go to Tab Number %1").arg( 7),0,KShortcut("Alt+7"),this,SLOT(goToTab6()),actionCollection(),"go_to_tab_7");
  new KAction(i18n("Go to Tab Number %1").arg( 8),0,KShortcut("Alt+8"),this,SLOT(goToTab7()),actionCollection(),"go_to_tab_8");
  new KAction(i18n("Go to Tab Number %1").arg( 9),0,KShortcut("Alt+9"),this,SLOT(goToTab8()),actionCollection(),"go_to_tab_9");
  new KAction(i18n("Go to Tab Number %1").arg(10),0,KShortcut("Alt+0"),this,SLOT(goToTab9()),actionCollection(),"go_to_tab_0");

  new KAction(i18n("Find Text"),0,KShortcut("F3"),this,SLOT(findTextShortcut()),actionCollection(),"find_text");
  new KAction(i18n("&Insert IRC Color"), "colorize", CTRL+Key_K, this, SLOT(addIRCColor()), actionCollection(), "irc_colors");

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

  createGUI();
  readOptions();
}

KonversationMainWindow::~KonversationMainWindow()
{
  kdDebug() << "KonversationMainWindow::~KonversationMainWindow()" << endl;

  if(nicksOnlineWindow) nicksOnlineWindow->closeButton();

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
  if(showToolBarAction->isChecked()) toolBar("mainToolBar")->show();
  else toolBar("mainToolBar")->hide();

  KonversationApplication::preferences.mainWindowToolBarStatus=showToolBarAction->isChecked();
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

  KonversationApplication::preferences.mainWindowMenuBarStatus=showMenuBarAction->isChecked();
}

void KonversationMainWindow::showStatusbar()
{
  if(showStatusBarAction->isChecked()) statusBar()->show();
  else statusBar()->hide();

  KonversationApplication::preferences.mainWindowStatusBarStatus=showStatusBarAction->isChecked();
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
    newText(serverView,QString::null);
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
//  kdDebug() << "KonversationMainWindow::closeView(" << viewToClose << ")" << endl;

  ChatWindow* view=static_cast<ChatWindow*>(viewToClose);
  if(view)
  {
    ChatWindow::WindowType viewType=view->getType();

    QString viewName=view->getName();
    // the views should know by themselves how to close

    if(viewType==ChatWindow::Status)            view->closeYourself();
    else if(viewType==ChatWindow::Channel)      view->closeYourself();
    else if(viewType==ChatWindow::ChannelList)  view->closeYourself();
    else if(viewType==ChatWindow::Query)        view->closeYourself();
    else if(viewType==ChatWindow::RawLog)       view->closeYourself();

    else if(viewType==ChatWindow::DccPanel)     closeDccPanel();
    else if(viewType==ChatWindow::Konsole)      closeKonsolePanel(view);
    else if(viewType==ChatWindow::UrlCatcher)   closeUrlCatcher();

/*
    else if(viewType==ChatWindow::DccChat);
    else if(viewType==ChatWindow::Notice);
    else if(viewType==ChatWindow::SNotice);
*/
  }
  // if this view was the front view, delete the pointer
  if(view==frontView) frontView=0;
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
}

void KonversationMainWindow::addUrlCatcher()
{
  // if the panel wasn't open yet
  if(urlCatcherPanel==0)
  {
    urlCatcherPanel=new UrlCatcher(getViewContainer());
    addView(urlCatcherPanel,2,i18n("URL catcher"),true);

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
    newText(dccPanel,QString::null);
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

StatusPanel* KonversationMainWindow::addStatusView(Server* server)
{
  StatusPanel* statusView=new StatusPanel(getViewContainer());

  // first set up internal data ...
  statusView->setServer(server);
  statusView->setIdentity(server->getIdentity());

  // ... then put it into the tab widget, otherwise we'd have a race with server member
  addView(statusView,2,server->getServerName(),false);

  connect(statusView,SIGNAL (newText(QWidget*,const QString&)),this,SLOT (newText(QWidget*,const QString&)) );
  connect(statusView,SIGNAL (sendFile()),server,SLOT (requestDccSend()) );
  connect(server,SIGNAL (awayState(bool)),statusView,SLOT (indicateAway(bool)) );

  return statusView;
}

Channel* KonversationMainWindow::addChannel(Server* server, const QString& name)
{
  Channel* channel=new Channel(getViewContainer());
  channel->setServer(server);
  channel->setName(name);

  addView(channel,1,name);

  connect(channel,SIGNAL (newText(QWidget*,const QString&)),this,SLOT (newText(QWidget*,const QString&)) );
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

  connect(query,SIGNAL (newText(QWidget*,const QString&)),this,SLOT (newText(QWidget*,const QString&)) );
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
  kdDebug() << "KonversationMainWindow::addChannelListPanel()" << endl;

  ChannelListPanel* channelListPanel=new ChannelListPanel(getViewContainer());
  channelListPanel->setServer(server);

  addView(channelListPanel,2,i18n("Channel list"));

  return channelListPanel;
}

void KonversationMainWindow::newText(QWidget* view,const QString& highlightColor)
{
  if(view!=getViewContainer()->currentPage())
  {
    getViewContainer()->changeTabState(view,true,highlightColor);
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

  viewContainer->changeTabState(view,false,QString::null);
}

void KonversationMainWindow::readOptions()
{
  // Tool bar settings
  showToolBarAction->setChecked(KonversationApplication::preferences.mainWindowToolBarStatus);
  toolBar("mainToolBar")->setBarPos((KToolBar::BarPosition) KonversationApplication::preferences.mainWindowToolBarPos);
  toolBar("mainToolBar")->setIconText((KToolBar::IconText) KonversationApplication::preferences.mainWindowToolBarIconText);
  toolBar("mainToolBar")->setIconSize(KonversationApplication::preferences.mainWindowToolBarIconSize);
  showToolbar();

  // Status bar settings
  showStatusBarAction->setChecked(KonversationApplication::preferences.mainWindowStatusBarStatus);
  showStatusbar();

  // Menu bar settings
  showMenuBarAction->setChecked(KonversationApplication::preferences.mainWindowMenuBarStatus);
  showMenubar();

  QSize size=KonversationApplication::preferences.getMainWindowSize();
  if(!size.isEmpty())
  {
    resize(size);
  }
}

// Will not actually save the options but write them into the prefs structure
void KonversationMainWindow::saveOptions()
{
  KonversationApplication::preferences.setMainWindowSize(size());

  KonversationApplication::preferences.mainWindowToolBarPos=toolBar("mainToolBar")->barPos();
  KonversationApplication::preferences.mainWindowToolBarIconText=toolBar("mainToolBar")->iconText();
  KonversationApplication::preferences.mainWindowToolBarIconSize=toolBar("mainToolBar")->iconSize();
}

bool KonversationMainWindow::queryClose()
{
  kdDebug() << "KonversationMainWindow::queryClose()" << endl;

  // send quit to all servers
  emit quitServer();
  saveOptions();

  return true;
}

void KonversationMainWindow::quitProgram()
{
  kdDebug() << "KonversationMainWindow::quitProgram()" << endl;
  // will call queryClose()
  close();
}

void KonversationMainWindow::openNicksOnlineWindow()
{
  if(!nicksOnlineWindow)
  {
    nicksOnlineWindow=new NicksOnline(KonversationApplication::preferences.getNicksOnlineSize());

    connect(nicksOnlineWindow,SIGNAL (editClicked()),this,SLOT (openNotify()) );
    connect(nicksOnlineWindow,SIGNAL (closeClicked(QSize)),this,SLOT (closeNicksOnlineWindow(QSize)) );

    connect(nicksOnlineWindow,SIGNAL (doubleClicked(const QString&,const QString&)),this,SLOT (notifyAction(const QString&,const QString&)) );

    connect(this,SIGNAL (nicksNowOnline(const QString&,const QStringList&)),nicksOnlineWindow,SLOT (setOnlineList(const QString&,const QStringList&)) );

    nicksOnlineWindow->show();
  }
}

void KonversationMainWindow::closeNicksOnlineWindow(QSize newSize)
{
  KonversationApplication::preferences.setNicksOnlineSize(newSize);
  emit prefsChanged();

  delete nicksOnlineWindow;
  nicksOnlineWindow=0;
}

void KonversationMainWindow::openServerList()
{
  emit openPrefsDialog(Preferences::ServerListPage);
}

void KonversationMainWindow::openNotify()
{
  emit openPrefsDialog(Preferences::NotifyPage);
}

// TODO: Let an own class handle notify things
void KonversationMainWindow::setOnlineList(Server* notifyServer,const QStringList& list)
{
  emit nicksNowOnline(notifyServer->getServerName(),list);
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
  kdDebug() << "KonversationMainWindow::updateFonts()" << endl;

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
                       i18n("You can only search in text fields!"),
                       i18n("Find Text Information"));
  }
  else
  {
    searchView->getTextView()->search();
  }
}

void KonversationMainWindow::openNotifications()
{
#if KDE_VERSION >= 310
  (void)KNotifyDialog::configure(this);
#endif
}

void KonversationMainWindow::addIRCColor()
{
  IRCColorChooser dlg(this, &(KonversationApplication::preferences));

  if(dlg.exec() == QDialog::Accepted) {
    frontView->appendInputText(dlg.color());
   }
 }

// I hope we can find a better way soon ... this is ridiculous"
void KonversationMainWindow::goToTab0() { goToTab(0); }
void KonversationMainWindow::goToTab1() { goToTab(1); }
void KonversationMainWindow::goToTab2() { goToTab(2); }
void KonversationMainWindow::goToTab3() { goToTab(3); }
void KonversationMainWindow::goToTab4() { goToTab(4); }
void KonversationMainWindow::goToTab5() { goToTab(5); }
void KonversationMainWindow::goToTab6() { goToTab(6); }
void KonversationMainWindow::goToTab7() { goToTab(7); }
void KonversationMainWindow::goToTab8() { goToTab(8); }
void KonversationMainWindow::goToTab9() { goToTab(9); }

#include "konversationmainwindow.moc"
