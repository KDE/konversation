/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni, Peter Simonsson
  email:     eisfuchs@tigress.com, psn@linux.se
*/

#include "konversationmainwindow.h"
#include "konvibookmarkhandler.h"

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
#include <kiconloader.h>
#include <kwin.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <dcopclient.h>
#include <scriptmanager.h>
#include <ktabwidget.h>
#include <kpushbutton.h>

#include <qpainter.h>
#include <qnamespace.h>
#include <qwhatsthis.h>
#include <qsignalmapper.h>

#include <kabc/addressbook.h>
#include <kabc/errorhandler.h>
#include "linkaddressbook/addressbook.h"

#ifdef  USE_KNOTIFY
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
#include "dccchat.h"
#include "serverlistdialog.h"
#include "insertchardialog.h"
#include "logfilereader.h"
#include "identitydialog.h"
#include "joinchanneldialog.h"
#include "notificationhandler.h"
#include "common.h"

KonversationMainWindow::KonversationMainWindow() : KMainWindow(0,"main_window", WStyle_ContextHelp | WType_TopLevel | WDestructiveClose)
{
  // Init variables before anything else can happen
  m_frontView=0;
  previousFrontView=0;
  searchView=0;
  frontServer=0;
  urlCatcherPanel=0;
  dccPanel=0;
  dccPanelOpen=false;
  m_closeApp = false;
  m_insertCharDialog = 0;
  m_serverListDialog = 0;

  dccTransferHandler=new DccTransferHandler(this);

  nicksOnlinePanel=0;

  viewContainer = new KTabWidget(this, "main_window_tab_widget");
  viewContainer->setTabReorderingEnabled(true);
  viewContainer->setTabCloseActivatePrevious(true);
  viewContainer->setAutomaticResizeTabs(true);
//  viewContainer->setHoverCloseButtonDelayed(false);
  setCentralWidget(viewContainer);
  updateTabPlacement();
  viewContainer->hide();
  KPushButton* closeBtn = new KPushButton(viewContainer);
  closeBtn->setPixmap(KGlobal::iconLoader()->loadIcon("tab_remove", KIcon::Small));
  closeBtn->resize(22, 22);
  closeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  viewContainer->setCornerWidget(closeBtn);
  closeBtn->hide();
  connect(closeBtn, SIGNAL(clicked()), this, SLOT(closeTab()));

  KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); // file_quit

  setStandardToolBarMenuEnabled(true);
  createStandardStatusBarAction();
  showMenuBarAction=KStdAction::showMenubar(this,SLOT(showMenubar()),actionCollection()); // options_show_menubar
  KStdAction::configureToolbars(this, SLOT(openToolbars()), actionCollection());
#ifdef USE_KNOTIFY
  KAction *configureNotificationsAction = KStdAction::configureNotifications(this,SLOT(openNotifications()), actionCollection());  // options_configure_notifications
#endif

  KStdAction::keyBindings(this,SLOT(openKeyBindings()),actionCollection()); // options_configure_key_binding
  KAction *preferencesAction = KStdAction::preferences(this,SLOT(openPreferences()),actionCollection()); // options_configure

  KAction* action;

  (new KAction(i18n("&Server List..."), "server", KShortcut("F2"), this, SLOT(openServerList()),
    actionCollection(), "open_server_list"))->setToolTip(i18n("Connect to a new server..."));
  (new KAction(i18n("Quick &Connect..."), "connect_creating", KShortcut("F7"), this, SLOT(openQuickConnectDialog()),
    actionCollection(), "quick_connect_dialog"))->setToolTip(i18n("Type in the address of a new IRC server to connect to..."));

  action = new KAction(i18n("&Reconnect"), "connect_creating", 0, this, SLOT(reconnectCurrentServer()), actionCollection(), "reconnect_server");
  action->setEnabled(false);

  (new KAction(i18n("&Identities..."), "identity", KShortcut("F8"), this, SLOT(openIdentitiesDialog()),
    actionCollection(), "identities_dialog"))->setToolTip(i18n("Set your nick, away message, etc..."));

  new KToggleAction(i18n("&Watched Nicks Online"), 0, KShortcut("F4"), this, SLOT(openNicksOnlinePanel()), actionCollection(), "open_nicksonline_window");
  action = new KAction(i18n("&Open Logfile"), "history", KShortcut("Ctrl+O"), this, SLOT(openLogfile()), actionCollection(), "open_logfile");
  action->setEnabled(false);

  action = new KAction(i18n("&Channel List"), 0, KShortcut("F5"), this, SLOT(openChannelList()), actionCollection(), "open_channel_list");
  action->setEnabled(false);
  new KToggleAction(i18n("&URL Catcher"), 0, KShortcut("F6"), this, SLOT(addUrlCatcher()), actionCollection(), "open_url_catcher");

  new KAction(i18n("New &Konsole"), "openterm", 0, this, SLOT(addKonsolePanel()), actionCollection(), "open_konsole");

  // Actions to navigate through the different pages
  KShortcut nextShortcut = KStdAccel::tabNext();
  nextShortcut.setSeq(1, KKeySequence("Alt+Right"));
  KShortcut prevShortcut = KStdAccel::tabPrev();
  prevShortcut.setSeq(1, KKeySequence("Alt+Left"));
  new KAction(i18n("&Next Tab"), QApplication::reverseLayout() ? "previous" : "next",
    QApplication::reverseLayout() ? prevShortcut : nextShortcut,
    this,SLOT(nextTab()), actionCollection(), "next_tab");
  new KAction(i18n("&Previous Tab"), QApplication::reverseLayout() ? "next" : "previous",
    QApplication::reverseLayout() ? nextShortcut : prevShortcut,
    this,SLOT(previousTab()),actionCollection(),"previous_tab");
  new KAction(i18n("Close &Tab"),"tab_remove",KShortcut("Ctrl+w"),this,SLOT(closeTab()),actionCollection(),"close_tab");
  
  QSignalMapper* tabSelectionMapper = new QSignalMapper(this);
  connect(tabSelectionMapper, SIGNAL(mapped(int)), this, SLOT(goToTab(int)));
  
  for(uint i = 1; i <= 10; ++i) {
    KAction* tabSelectionAction = new KAction(i18n("Go to Tab %1").arg(i), 0, KShortcut(QString("Alt+%1").arg(i%10)),
      tabSelectionMapper, SLOT(map()), actionCollection(), QString("go_to_tab_%1").arg(i).local8Bit());
    tabSelectionMapper->setMapping( tabSelectionAction, i-1);
  }

  action = new KAction(i18n("&Clear Window"),0,KShortcut("Ctrl+L"),this,SLOT(clearWindow()),actionCollection(),"clear_window");
  action->setToolTip(i18n("Clear content of current window"));
  action->setEnabled(false);
  action = new KAction(i18n("Clear &All Windows"),0,KShortcut("CTRL+SHIFT+L"),this,SLOT(clearTabs()),actionCollection(),"clear_tabs");
  action->setToolTip(i18n("Clear contents of all windows"));
  action->setEnabled(false);

  KAction* awayAction = new KAction(i18n("Set &Away Globally")/*, "konversationaway"*/, KShortcut("Ctrl+Alt+A"),
    static_cast<KonversationApplication *>(kapp), SLOT(toggleAway()), actionCollection(),"toggle_away");  //string must be the same as that used in server.cpp
  awayAction->setEnabled(false);
  action = new KAction(i18n("&Join Channel..."), 0, KShortcut("Ctrl+J"), this, SLOT(showJoinChannelDialog()), actionCollection(), "join_channel");
  action->setEnabled(false);

  action = KStdAction::find(this, SLOT(findText()), actionCollection());
  action->setEnabled(false);
  action = KStdAction::findNext(this, SLOT(findNextText()), actionCollection());
  action->setEnabled(false);

  action = new KAction(i18n("&IRC Color..."), "colorize", CTRL+Key_K, this, SLOT(addIRCColor()), actionCollection(), "irc_colors");
  action->setToolTip(i18n("Set the color of your current IRC message."));
  action->setEnabled(false);
  action = new KAction(i18n("&Remember Line"), 0,  KShortcut("Ctrl+R") , this, SLOT(insertRememberLine()), actionCollection(), "insert_remember_line");
  action->setToolTip(i18n("Add a horizontal line that only you can see."));
  action->setEnabled(false);
  action = new KAction(i18n("Special &Character..."), "char", KShortcut("Alt+Shift+C"), this, SLOT(insertCharacter()), actionCollection(), "insert_character");
  action->setToolTip(i18n("Insert any character into your current IRC message. "));
  action->setEnabled(false);

  new KAction(i18n("Close &All Open Queries"), 0, KShortcut("F11"), this, SLOT(closeQueries()), actionCollection(), "close_queries");
  hideNicklistAction = new KToggleAction(i18n("Hide Nicklist"), 0, KShortcut("Ctrl+H"), this, SLOT(hideNicknameList()), actionCollection(), "hide_nicknamelist");
  
  // Initialize KMainWindow->statusBar()
  statusBar();
  m_sslLabel = new SSLLabel(statusBar(),"sslLabel");
  m_sslLabel->setPixmap(SmallIcon("encrypted"));
  m_sslLabel->hide();
  QWhatsThis::add(m_sslLabel, i18n("All communication with the server is encrypted.  This makes it harder for someone to listen in on your communications."));

  m_channelInfoLabel = new QLabel(statusBar(), "channelInfoLabel");
  QWhatsThis::add(m_channelInfoLabel, i18n("<qt>This shows the number of users in the channel, and the number of those that are operators (ops).<p>A channel operator is a user that has special privileges, such as the ability to kick and ban users, change the channel modes, make other users operators</qt>"));

  statusBar()->insertItem(i18n("Ready."), StatusText, 1);
  statusBar()->addWidget(m_channelInfoLabel, 0, true);
  statusBar()->insertItem("lagometer", LagOMeter, 0, true);
  statusBar()->addWidget(m_sslLabel, 0, true);
  QWhatsThis::add(statusBar(), i18n("<qt>The status bar shows various messages, including any problems connecting to the server.  On the far right the current delay to the server is shown.  The delay is the time it takes for messages from you to reach the server, and from the server back to you.</qt>"));

  // Show "Lag unknown"
  resetLag();
  statusBar()->setItemAlignment(StatusText,QLabel::AlignLeft);

  actionCollection()->setHighlightingEnabled(true);
  connect(actionCollection(), SIGNAL( actionStatusText( const QString & ) ),
                 statusBar(), SLOT( message( const QString & ) ) );
  connect(actionCollection(), SIGNAL( clearStatusText() ),
                 statusBar(), SLOT( clear() ) );

  connect( viewContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changeView(QWidget*)) );
  connect( viewContainer, SIGNAL(closeRequest(QWidget*)), this, SLOT(closeView(QWidget*)));

  // set up system tray
  tray = new Konversation::TrayIcon(this);
  connect(this, SIGNAL(endNotification()), tray, SLOT(endNotification()));
  connect(tray, SIGNAL(quitSelected()), this, SLOT(quitProgram()));
  KPopupMenu *trayMenu = tray->contextMenu();
#ifdef USE_KNOTIFY
  configureNotificationsAction->plug(trayMenu);
#endif
  preferencesAction->plug(trayMenu);
  awayAction->plug(trayMenu);

  // decide whether to show the tray icon or not
  updateTrayIcon();

  createGUI();

  // Bookmarks
  m_bookmarkHandler = new KonviBookmarkHandler(this);
  connect(m_bookmarkHandler,SIGNAL(openURL(const QString&,const QString&)),this,SLOT(openURL(const QString&,const QString&)));

  resize(700, 500);  // Give the app a sane default size
  setAutoSaveSettings();
  showMenuBarAction->setChecked(KonversationApplication::preferences.getShowMenuBar());
  showMenubar(true);

  // set up KABC with a nice gui error dialog
  KABC::GuiErrorHandler *m_guiErrorHandler = new KABC::GuiErrorHandler(this);
  kapp->dcopClient()->setAcceptCalls( false );
  Konversation::Addressbook::self()->getAddressBook()->setErrorHandler(m_guiErrorHandler);
  kapp->dcopClient()->setAcceptCalls( true );

  // demo how to add additional dock windows
//  QListView* dockList=new QListView(this);
//  addToolWindow(dockList,KDockWidget::DockLeft,getMainDockWidget());

  if(KonversationApplication::preferences.getOpenWatchedNicksAtStartup()) {
    openNicksOnlinePanel();
  }
}

KonversationMainWindow::~KonversationMainWindow()
{
  deleteDccPanel();
  delete dccTransferHandler;
}

void KonversationMainWindow::updateTabPlacement()
{
  viewContainer->setTabPosition((KonversationApplication::preferences.getTabPlacement()==Preferences::Top) ?
                                 QTabWidget::Top : QTabWidget::Bottom);
//  getViewContainer()->setHoverCloseButton(KonversationApplication::preferences.getCloseButtonsOnTabs());
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
}

void KonversationMainWindow::focusAndShowErrorMessage(const QString &errorMsg) {
  show();
  KWin::demandAttention(winId());
  KWin::activateWindow(winId());
  KMessageBox::error(this, errorMsg);
}

void KonversationMainWindow::showMenubar(bool dontShowWarning)
{
  if(showMenuBarAction->isChecked()) menuBar()->show();
  else
  {
    if(!dontShowWarning) {
      QString accel=showMenuBarAction->shortcut().toString();
      KMessageBox::information(this,i18n("<qt>This will hide the menu bar completely."
                                        "You can show it again by typing %1.</qt>").arg(accel),
                                        "Hide menu bar","HideMenuBarWarning");
    }
    menuBar()->hide();
  }

  KonversationApplication::preferences.setShowMenuBar(showMenuBarAction->isChecked());
}

void KonversationMainWindow::showStatusbar()
{
}

/** Call this when you have already put a message in the serverView window, 
 *   and want a message in the front most window if it's on the same server, but not put the message twice.
 */
void KonversationMainWindow::appendToFrontmostIfDifferent(const QString& type,const QString& message,ChatWindow* serverView)
{
  Q_ASSERT(serverView); if(!serverView) return;
  updateFrontView();
  if(m_frontView && (ChatWindow *)m_frontView != serverView &&
		  m_frontView->getServer()==serverView->getServer() &&
		  !KonversationApplication::preferences.getRedirectToStatusPane()
		  )
    m_frontView->appendServerMessage(type,message);
}
void KonversationMainWindow::appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView)
{
  if( !serverView) serverView = m_frontView->getServer()->getStatusView();

  Q_ASSERT(m_frontView && m_frontView->getServer() == frontServer);  //if this fails, we need to fix frontServer

  Q_ASSERT(serverView); if(!serverView) return;
  updateFrontView();
  if(!m_frontView ||                                                  // Check if the m_frontView can actually display text or ...
     serverView->getServer()!=m_frontView->getServer() ||             // if it does not belong to this server or...
     KonversationApplication::preferences.getRedirectToStatusPane())// if the user decided to force it.
  {
    // if not, take server specified fallback view instead
    serverView->appendServerMessage(type,message);
    // FIXME: this signal should be sent from the status panel instead, so it
    //        can be using the correct highlight color, would be more consistent
    //        anyway!
    newText(serverView,QString::null,true);
  }
  else
    m_frontView->appendServerMessage(type,message);
}

void KonversationMainWindow::addView(ChatWindow* view,int color,const QString& label,bool on, bool weinitiated)
{
  // TODO: Make sure to add DCC status tab at the end of the list and all others
  // before the DCC tab. Maybe we should also make sure to order Channels
  // Queries and DCC chats in groups
  ChatWindow *tmp_ChatWindow;
  int placed = 0;
  ChatWindow::WindowType wtype;

  switch (view->getType())
  {
    case ChatWindow::Channel:
      for (int sindex = 0; sindex < viewContainer->count(); sindex++)
      {
        tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(sindex));
  
        if (tmp_ChatWindow->getType() == ChatWindow::Status && tmp_ChatWindow->getServer() == view->getServer())
        {
          for (int index = sindex + 1; index < viewContainer->count(); index++)
          {
            tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(index));
            wtype = tmp_ChatWindow->getType();

            if (wtype != ChatWindow::Channel && wtype != ChatWindow::RawLog)
            {
              viewContainer->insertTab(view, label, index);
              placed = 1;
              break;
            }
          }

          break;
        }
      }

      break;

    case ChatWindow::RawLog:
      for (int sindex = 0; sindex < viewContainer->count(); sindex++)
      {
        tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(sindex));
  
        if (tmp_ChatWindow->getType() == ChatWindow::Status && tmp_ChatWindow->getServer() == view->getServer())
        {
          viewContainer->insertTab(view, label, sindex + 1);
          placed = 1;

          break;
        }
      }

      break;

    case ChatWindow::Query:
      for (int sindex = 0; sindex < viewContainer->count(); sindex++)
      {
        tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(sindex));
  
        if (tmp_ChatWindow->getType() == ChatWindow::Status && tmp_ChatWindow->getServer() == view->getServer())
        {
          for (int index = sindex + 1; index < viewContainer->count(); index++)
          {
            tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(index));
            wtype = tmp_ChatWindow->getType();

            if (wtype != ChatWindow::Channel && wtype != ChatWindow::RawLog && wtype != ChatWindow::Query)
            {
              viewContainer->insertTab(view, label, index);
              placed = 1;
              break;
            }
          }

          break;
        }
      }

      break;

    case ChatWindow::DccChat:
      for (int sindex = 0; sindex < viewContainer->count(); sindex++)
      {
        tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(sindex));
  
        if (tmp_ChatWindow->getType() == ChatWindow::Status && tmp_ChatWindow->getServer() == view->getServer())
        {
          for (int index = sindex + 1; index < viewContainer->count(); index++)
          {
            tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(index));
            wtype = tmp_ChatWindow->getType();

            if (wtype != ChatWindow::Channel && wtype != ChatWindow::RawLog &&
                wtype != ChatWindow::Query && wtype != ChatWindow::DccChat)
            {
              viewContainer->insertTab(view, label, index);
              placed = 1;
              break;
            }
          }

          break;
        }
      }

     default:
       break;
  }

  if (!placed) 
    viewContainer->insertTab(view, label);
  viewContainer->show();

  // Check, if user was typing in old input line
  bool doBringToFront=true;

  // make sure that bring to front only works when the user wasn't typing something
  if(m_frontView && view->getType() != ChatWindow::UrlCatcher &&
		  view->getType() != ChatWindow::Konsole)
  {
    if(!m_frontView->getTextInLine().isEmpty()) doBringToFront=false;
  }
  
  if(!KonversationApplication::preferences.getFocusNewQueries() && view->getType()==ChatWindow::Query && !weinitiated)
	  doBringToFront = false;

  // bring view to front unless it's a raw log window or the user was typing
  if(KonversationApplication::preferences.getBringToFront() && doBringToFront &&
    view->getType()!=ChatWindow::RawLog)
  {
    showView(view);
  }

// FIXME  connect(view,SIGNAL (online(ChatWindow*,bool)),viewContainer,SLOT (setTabOnline(ChatWindow*,bool)) );
}

void KonversationMainWindow::showView(ChatWindow* view)
{
  // Don't bring Tab to front if TabWidget is hidden. Otherwise QT gets confused
  // and shows the Tab as active but will display the wrong pane
  if(viewContainer->isVisible())
  {
    // TODO: add adjustFocus() here?
    viewContainer->showPage(view);  //This does will changeView(view) via slots
  }
}

void KonversationMainWindow::closeView(QWidget* viewToClose)
{
  ChatWindow* view=static_cast<ChatWindow*>(viewToClose);
  if(view)
  {

    ChatWindow::WindowType viewType=view->getType();

    QString viewName=view->getName();

    // the views should know by themselves how to close

    bool confirmClose = true;
    if(viewType==ChatWindow::Status)            confirmClose = view->closeYourself();
    else if(viewType==ChatWindow::Channel)      confirmClose = view->closeYourself();
    else if(viewType==ChatWindow::ChannelList)  confirmClose = view->closeYourself();
    else if(viewType==ChatWindow::Query)        confirmClose = view->closeYourself();
    else if(viewType==ChatWindow::RawLog)       confirmClose = view->closeYourself();
    else if(viewType==ChatWindow::DccChat)      confirmClose = view->closeYourself();

    else if(viewType==ChatWindow::DccPanel)     closeDccPanel();
    else if(viewType==ChatWindow::Konsole)      closeKonsolePanel(view);
    else if(viewType==ChatWindow::UrlCatcher)   closeUrlCatcher();
    else if(viewType==ChatWindow::NicksOnline)  closeNicksOnlinePanel();
    else if(viewType == ChatWindow::LogFileReader) view->closeYourself();

    if(!confirmClose)
	    return; //We haven't done anything yet, so safe to return

    // if this view was the front view, delete the pointer
    if(view==previousFrontView) previousFrontView=0;
    if(view==m_frontView) m_frontView=previousFrontView;

    viewContainer->removePage(view);

    if(viewContainer->count() <= 0) {
      viewContainer->hide();
    }
  }
}

void KonversationMainWindow::openLogfile()
{
  if(m_frontView)
  {
    ChatWindow* view=static_cast<ChatWindow*>(m_frontView);
    ChatWindow::WindowType viewType=view->getType();
    if(viewType==ChatWindow::Channel ||
       viewType==ChatWindow::Query ||
       viewType==ChatWindow::Status)
    {
      openLogFile(view->getName(), view->logFileName());
    }
  }
}

void KonversationMainWindow::openLogFile(const QString& caption, const QString& file)
{
  if(!file.isEmpty()) {
    LogfileReader* logReader = new LogfileReader(getViewContainer(), file);
    addView(logReader, 3, i18n("Logfile of %1").arg(caption));
    logReader->setServer(frontServer);
  }
}

void KonversationMainWindow::addKonsolePanel()
{
  KonsolePanel* panel=new KonsolePanel(getViewContainer());
  addView(panel,3,i18n("Konsole"));
  connect(panel,SIGNAL (deleted(ChatWindow*)),this,SLOT (closeKonsolePanel(ChatWindow*)) );
  panel->setMainWindow(this);
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
    if(panel) {
      getViewContainer()->showPage(panel);
    } else {
      int ret = KMessageBox::warningContinueCancel(this,i18n("Using this function may result in a lot "
                                         "of network traffic. If your connection is not fast "
                                         "enough, it is possible that your client will be "
                                         "disconnected by the server."), i18n("Channel List Warning"),
                                         KStdGuiItem::cont(), "ChannelListWarning");

      if(ret == KMessageBox::Continue) {
        frontServer->addChannelListPanel();
      }
    }
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
    urlCatcherPanel->setMainWindow(this);
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
    (dynamic_cast<KToggleAction*>(actionCollection()->action("open_url_catcher")))->setChecked(true);
  } else if((ChatWindow *)m_frontView != (ChatWindow *)urlCatcherPanel){
    showView(urlCatcherPanel);
    (dynamic_cast<KToggleAction*>(actionCollection()->action("open_url_catcher")))->setChecked(true);
  } else {
    closeUrlCatcher();
  }
}

void KonversationMainWindow::closeUrlCatcher()
{
  // if there actually is a dcc panel
  if(urlCatcherPanel)
  {
    delete urlCatcherPanel;
    urlCatcherPanel=0;    
    (dynamic_cast<KToggleAction*>(actionCollection()->action("open_url_catcher")))->setChecked(false);
  }
}

void KonversationMainWindow::addDccPanel()
{
  // if the panel wasn't open yet
  if(dccPanel==0)
  {
    dccPanel=new DccPanel(getViewContainer());
    addView(dccPanel,3,i18n("DCC Status"));
    dccPanel->setMainWindow(this);
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
  if(!listen) // Someone else initiated dcc chat
    {
      KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
      konv_app->notificationHandler()->dccChat(m_frontView,nick);
    }
    
  if(frontServer)
  {
    DccChat* dccChatPanel=new DccChat(getViewContainer(),frontServer,myNick,nick,arguments,listen);
    addView(dccChatPanel,3,dccChatPanel->getName());
    connect(dccChatPanel,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );
    if(listen) 
      frontServer->queue(QString("PRIVMSG %1 :\x01%2 CHAT chat %3 %4\x01").arg(nick).arg("DCC").arg(numericalIp).arg(dccChatPanel->getPort()));
  }
}

StatusPanel* KonversationMainWindow::addStatusView(Server* server)
{
  StatusPanel* statusView=new StatusPanel(getViewContainer());

  // first set up internal data ...
  statusView->setServer(server);
  statusView->setIdentity(server->getIdentity());
  statusView->setName(server->getServerName());

  // SSL icon stuff
  QObject::connect(server,SIGNAL(sslInitFailure()),this,SLOT(removeSSLIcon()));
  QObject::connect(server,SIGNAL(sslConnected(Server*)),this,SLOT(updateSSLInfo(Server*)));

  // ... then put it into the tab widget, otherwise we'd have a race with server member
  addView(statusView,2,server->getServerName(),false);

  connect(statusView,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );
  connect(statusView,SIGNAL (sendFile()),server,SLOT (requestDccSend()) );
  // TODO: Why was this here?  Delete channelPrefsChanged method as it does not
  // appear to do anything since statusView never emits signal prefsChanged.
  // connect(statusView,SIGNAL (prefsChanged()),this,SLOT (channelPrefsChanged()) );
  connect(server,SIGNAL (awayState(bool)),statusView,SLOT (indicateAway(bool)) );

  // make sure that frontServer gets set on adding the first status panel, too,
  // since there won't be a changeView happening
  if(!frontServer) frontServer=server;

  return statusView;
}

Channel* KonversationMainWindow::addChannel(Server* server, const QString& name)
{
  // Some IRC channels begin with ampersands (server local channels)
  // Accelerators don't belong in channel names, filter ampersands

  // Copy the name first
  QString newname = name;
  newname.replace('&', "&&");

  Channel* channel=new Channel(getViewContainer());
  channel->setServer(server);
  channel->setName(name);
  addView(channel,1,newname);

  connect(channel,SIGNAL (newText(QWidget*,const QString&,bool)),this,SLOT (newText(QWidget*,const QString&,bool)) );
  // TODO: Why is this here?  Delete channelPrefsChanged as it does not appear to
  // do anything since channel never emits prefsChanged.
  // connect(channel,SIGNAL (prefsChanged()),this,SLOT (channelPrefsChanged()) );
  connect(server,SIGNAL (awayState(bool)),channel,SLOT (indicateAway(bool)) );

  connect(channel, SIGNAL(splitterMoved(Channel*)), this, SIGNAL(channelSplittersMoved(Channel*)));
  connect(this, SIGNAL(channelSplittersMoved(Channel*)), channel, SLOT(updateSplitters(Channel*)));

  return channel;
}

Query* KonversationMainWindow::addQuery(Server* server, const NickInfoPtr& nickInfo, bool weinitiated)
{
  QString name = nickInfo->getNickname();
  Query* query=new Query(getViewContainer());
  query->setServer(server);
  query->setNickInfo(nickInfo);
  addView(query,0,name, true, weinitiated);

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

  if(view != getViewContainer()->currentPage())
    getViewContainer()->setTabColor(view, QColor(highlightColor));

}

void KonversationMainWindow::updateFrontView()
{
  ChatWindow* view = static_cast<ChatWindow*>(getViewContainer()->currentPage());
  KAction* action;

  if(view) {
    // Make sure that only views with info output get to be the m_frontView
    if(m_frontView) {
      previousFrontView = m_frontView;
      disconnect(m_frontView, SIGNAL(updateInfo(const QString &)), this, SLOT(updateChannelInfo(const QString &)));
    }

    if(view->canBeFrontView()) {
      m_frontView = view;

      connect(view, SIGNAL(updateInfo(const QString &)), this, SLOT(updateChannelInfo(const QString &)));
      view->emitUpdateInfo();
    } else {
      if( view->getName() != "ChatWindowObject" )
        m_channelInfoLabel->setText(Konversation::removeIrcMarkup(view->getName()));
      else
        m_channelInfoLabel->setText(QString::null);
    }

    // Make sure that only text views get to be the searchView
    if(view->searchView()) {
      searchView = view;
    }

    action = actionCollection()->action("insert_remember_line");
    if(action) action->setEnabled(view->getTextView() != 0);

    action = actionCollection()->action("insert_character");
    if(action) action->setEnabled(view->isInsertCharacterSupported());

    action = actionCollection()->action("irc_colors");
    if(action) action->setEnabled(view->areIRCColorsSupported());

    action = actionCollection()->action("clear_window");
    if(action) action->setEnabled(view->getTextView() != 0);

    action = actionCollection()->action("edit_find");
    if(action) action->setEnabled(view->searchView());

    action = actionCollection()->action("edit_find_next");
    if(action) action->setEnabled(view->searchView());

    action = actionCollection()->action("open_channel_list");
    if(action) {
      if(view->getServer()) {
        action->setEnabled(true);
        action->setText(i18n("&Channel List for %1").arg(view->getServer()->getServerGroup()));
      } else {
        action->setEnabled(false);
        action->setText(i18n("&Channel List"));
      }
    }

    action = actionCollection()->action("join_channel");
    if(action) action->setEnabled(view->getServer() != 0);

    action = actionCollection()->action("open_logfile");
    if(action) {
      action->setEnabled(!view->logFileName().isEmpty());
      if(view->logFileName().isEmpty())
        action->setText(i18n("&Open Logfile"));
      else
        action->setText(i18n("&Open Logfile for %1").arg(view->getName()));
    }

    action = actionCollection()->action("clear_tabs");
    if(action) action->setEnabled(true);

    action = actionCollection()->action("toggle_away");
    if(action) action->setEnabled(true);

    action = actionCollection()->action("reconnect_server");
    if(action) {
      Server* server = view->getServer();

      if(server && !server->isConnected()) {
        action->setEnabled(true);
      } else {
        action->setEnabled(false);
      }
    }
  } else {
    action = actionCollection()->action("insert_remember_line");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("insert_character");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("irc_colors");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("clear_window");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("clear_tabs");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("edit_find");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("edit_find_next");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("open_channel_list");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("open_logfile");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("toggle_away");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("join_channel");
    if(action) action->setEnabled(false);

    action = actionCollection()->action("reconnect_server");
    if(action) action->setEnabled(false);
  }
}

void KonversationMainWindow::changeView(QWidget* viewToChange)
{
  ChatWindow* view = static_cast<ChatWindow*>(viewToChange);
  
  if(m_frontView) {
    previousFrontView = m_frontView;
    disconnect(m_frontView, SIGNAL(updateInfo(const QString &)), this, SLOT(updateChannelInfo(const QString &)));
  }
  
  m_frontView = 0;
  searchView = 0;

  frontServer = view->getServer();
  // display this server's lag time
  if(frontServer) {
    updateSSLInfo(frontServer);
    updateLag(frontServer,frontServer->getLag());
  }


  updateFrontView();

  viewContainer->setTabColor(view, QColor());
  view->adjustFocus();
}

bool KonversationMainWindow::queryClose()
{
  KonversationApplication* konv_app = static_cast<KonversationApplication*>(kapp);

  if(konv_app->sessionSaving() || sender() == tray) {
    m_closeApp = true;
  }

  if(KonversationApplication::preferences.getShowTrayIcon() && !m_closeApp) {

      // Compute size and position of the pixmap to be grabbed:
      QPoint g = tray->mapToGlobal( tray->pos() );
      int desktopWidth  = kapp->desktop()->width();
      int desktopHeight = kapp->desktop()->height();
      int tw = tray->width();
      int th = tray->height();
      int w = desktopWidth / 4;
      int h = desktopHeight / 9;
      int x = g.x() + tw/2 - w/2; // Center the rectange in the systray icon
      int y = g.y() + th/2 - h/2;
      if ( x < 0 )                 x = 0; // Move the rectangle to stay in the desktop limits
      if ( y < 0 )                 y = 0;
      if ( x + w > desktopWidth )  x = desktopWidth - w;
      if ( y + h > desktopHeight ) y = desktopHeight - h;

      // Grab the desktop and draw a circle arround the icon:
      QPixmap shot = QPixmap::grabWindow( qt_xrootwin(),  x,  y,  w,  h );
      QPainter painter( &shot );
      const int MARGINS = 6;
      const int WIDTH   = 3;
      int ax = g.x() - x - MARGINS -1;
      int ay = g.y() - y - MARGINS -1;
      painter.setPen(  QPen( Qt::red,  WIDTH ) );
      painter.drawArc( ax,  ay,  tw + 2*MARGINS,  th + 2*MARGINS,  0,  16*360 );
      painter.end();

      // Associate source to image and show the dialog:
      QMimeSourceFactory::defaultFactory()->setPixmap( "systray_shot",  shot );
      KMessageBox::information( this,
      i18n( "<p>Closing the main window will keep Konversation running in the system tray. "
            "Use <b>Quit</b> from the <b>Konversation</b> menu to quit the application.</p>"
            "<p><center><img source=\"systray_shot\"></center></p>" ),
      i18n( "Docking in System Tray" ),  "HideOnCloseInfo" );
      hide();

      return false;
  }

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
    nicksOnlinePanel->setMainWindow(this);
    connect(nicksOnlinePanel,SIGNAL (editClicked()),this,SLOT (openNotify()) );

    connect(nicksOnlinePanel,SIGNAL (doubleClicked(const QString&,const QString&)),this,SLOT (notifyAction(const QString&,const QString&)) );

    connect(this,SIGNAL (nicksNowOnline(Server*)),nicksOnlinePanel,SLOT (updateServerOnlineList(Server*)) );
    (dynamic_cast<KToggleAction*>(actionCollection()->action("open_nicksonline_window")))->setChecked(true);
  } else if((ChatWindow *)m_frontView != (ChatWindow *)nicksOnlinePanel){
    showView(nicksOnlinePanel);
    (dynamic_cast<KToggleAction*>(actionCollection()->action("open_nicksonline_window")))->setChecked(true);
  } else {
    closeNicksOnlinePanel();
    (dynamic_cast<KToggleAction*>(actionCollection()->action("open_nicksonline_window")))->setChecked(false);
  }

}

void KonversationMainWindow::closeNicksOnlinePanel()
{
  if(nicksOnlinePanel)
  {
    delete nicksOnlinePanel;
    nicksOnlinePanel=0;
  }
  (dynamic_cast<KToggleAction*>(actionCollection()->action("open_nicksonline_window")))->setChecked(false);
}

void KonversationMainWindow::openServerList()
{
  if(!m_serverListDialog) {
    m_serverListDialog = new Konversation::ServerListDialog(this);
    KonversationApplication *konvApp = static_cast<KonversationApplication *>(KApplication::kApplication());
    connect(m_serverListDialog, SIGNAL(connectToServer(int)), konvApp, SLOT(connectToServer(int)));
  }

  m_serverListDialog->show();
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
  emit nicksNowOnline(notifyServer);
  if (changed && nicksOnlinePanel) newText(nicksOnlinePanel, QString::null, true);
}

void KonversationMainWindow::notifyAction(const QString& serverName,const QString& nick)
{
  KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
  Server* server=konv_app->getServerByName(serverName);
  server->notifyAction(nick);
}

void KonversationMainWindow::slotPrefsChanged()
{
  kdDebug() << "KonversationMainWindow::slotPrefsChanged()" << endl;
  emit prefsChanged();
}

void KonversationMainWindow::removeSSLIcon()
{
  disconnect(m_sslLabel,0,0,0);
  m_sslLabel->hide();
}

void KonversationMainWindow::channelPrefsChanged()
{
  emit prefsChanged();
}

KTabWidget* KonversationMainWindow::getViewContainer()
{
  return viewContainer;
}

void KonversationMainWindow::updateFonts()
{
// FIXME  getViewContainer()->updateTabs();
}

void KonversationMainWindow::updateLag(Server* lagServer,int msec)
{
  // show lag only of actual server
  if(lagServer==frontServer)
  {
    statusBar()->changeItem(i18n("Ready."),StatusText);
    QString lagString = lagServer->getServerName() + " - ";

    if (msec == -1) {
      lagString += i18n("Lag: not known");
    } else if(msec < 1000) {
      lagString += i18n("Lag: %1 ms").arg(msec);
    } else {
      lagString += i18n("Lag: %1 s").arg(msec / 1000);
    }
    
    statusBar()->changeItem(lagString, LagOMeter);
  }
}

void KonversationMainWindow::updateSSLInfo(Server* server)
{
  if(server == frontServer && server->getUseSSL() && server->isConnected())
    {
      disconnect(m_sslLabel,0,0,0);
      connect(m_sslLabel,SIGNAL(clicked()),server,SLOT(showSSLDialog()));
      QToolTip::remove(m_sslLabel);
      QToolTip::add(m_sslLabel,server->getSSLInfo());
      m_sslLabel->show();
    }
  else
    m_sslLabel->hide();
}

void KonversationMainWindow::tooLongLag(Server* lagServer,int msec)
{

  if((msec % 5000)==0)
  {
    int seconds  = msec/1000;
    int minutes  = seconds/60;
    int hours    = minutes/60;
    int days     = hours/24;
    QString lagString;

    if(days) {
      const QString daysString = i18n("1 day", "%n days", days);
      const QString hoursString = i18n("1 hour", "%n hours", (hours % 24));
      const QString minutesString = i18n("1 minute", "%n minutes", (minutes % 60));
      const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
      lagString = i18n("%1 = name of server, %2 = (x days), %3 = (x hours), %4 = (x minutes), %5 = (x seconds)", "No answer from server %1 for more than %2, %3, %4, and %5.").arg(lagServer->getServerName())
                     .arg(daysString).arg(hoursString).arg(minutesString).arg(secondsString);
    // or longer than an hour
    } else if(hours) {
      const QString hoursString = i18n("1 hour", "%n hours", hours);
      const QString minutesString = i18n("1 minute", "%n minutes", (minutes % 60));
      const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
      lagString = i18n("%1 = name of server, %2 = (x hours), %3 = (x minutes), %4 = (x seconds)", "No answer from server %1 for more than %2, %3, and %4.").arg(lagServer->getServerName())
                     .arg(hoursString).arg(minutesString).arg(secondsString);
    // or longer than a minute
    } else if(minutes) {
      const QString minutesString = i18n("1 minute", "%n minutes", minutes);
      const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
      lagString = i18n("%1 = name of server, %2 = (x minutes), %3 = (x seconds)", "No answer from server %1 for more than %2 and %3.").arg(lagServer->getServerName())
                     .arg(minutesString).arg(secondsString);
    // or just some seconds
    } else {
      lagString = i18n("No answer from server %1 for more than 1 second.", "No answer from server %1 for more than %n seconds.", seconds).arg(lagServer->getServerName());
    }

    statusBar()->changeItem(lagString,StatusText);
  }
  if(lagServer==frontServer) {
    //show lag only of actual server
    QString lagString(i18n("Lag: %1 s").arg(msec/1000));
    statusBar()->changeItem(lagString,LagOMeter);
  }
}

// TODO: Make this server dependant
void KonversationMainWindow::resetLag()
{
  statusBar()->changeItem(i18n("Lag: not known"),LagOMeter);
}

void KonversationMainWindow::closeTab()
{
  closeView(getViewContainer()->currentPage());
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
    if ( page >= getViewContainer()->count() )
        page = 0;
    else if ( page < 0 )
        page = getViewContainer()->count() - 1;

    if(page>=0)
    {
        getViewContainer()->setCurrentPage(page);
        ChatWindow* newPage=static_cast<ChatWindow*>(getViewContainer()->page(page));
        newPage->adjustFocus();
    }
}

void KonversationMainWindow::findText()
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

void KonversationMainWindow::findNextText()
{
  if(searchView)
  {
    searchView->getTextView()->searchAgain();
  }
}
void KonversationMainWindow::clearWindow()
{
  if (m_frontView)
    m_frontView->getTextView()->clear();
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
    {
      tray->show();
    }
  else
    tray->hide();

  tray->setNotificationEnabled(KonversationApplication::preferences.getTrayNotify());

  if(KonversationApplication::preferences.getShowTrayIcon() &&
     KonversationApplication::preferences.getSystrayOnly())
  {
    KWin::setState(winId(), NET::SkipTaskbar);
  } else {
    KWin::clearState(winId(), NET::SkipTaskbar);
  }
}

void KonversationMainWindow::addIRCColor()
{
  IRCColorChooser dlg(this, &(KonversationApplication::preferences));

  if(dlg.exec() == QDialog::Accepted) {
    m_frontView->appendInputText(dlg.color());
  }
}

void KonversationMainWindow::insertRememberLine()
{
  kdDebug() << "insertRememberLine in konversationMainWindow" << endl;
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
    if(m_frontView->getType() == ChatWindow::Channel ||
        m_frontView->getType() == ChatWindow::Query)
    {
      m_frontView->insertRememberLine();
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

void KonversationMainWindow::hideNicknameList()
{
    if (m_frontView && m_frontView->getType() == ChatWindow::Channel) {
        Channel* channel = static_cast<Channel*>((ChatWindow*) m_frontView);
        if (hideNicklistAction->isChecked()) {
            channel->showNicknameList(false);
            hideNicklistAction->setChecked(true);
        } else {
            channel->showNicknameList(true);
            hideNicklistAction->setChecked(false);
        }
    }
}


void KonversationMainWindow::clearTabs()
{
  int total=getViewContainer()->count()-1;
  ChatWindow* nextPage;

  for(int i=0;i<=total;i++)
    {
      nextPage=static_cast<ChatWindow*>(getViewContainer()->page(i));

      if(nextPage && nextPage->getTextView())
	nextPage->getTextView()->clear();
    }
}

bool KonversationMainWindow::event(QEvent* e)
{
  if(e->type() == QEvent::WindowActivate) {
    emit endNotification();
  }

  return KMainWindow::event(e);
}

void KonversationMainWindow::serverQuit(Server* server)
{
  if(server == frontServer) {
    frontServer = 0;
  }

  if(m_frontView && m_frontView->getServer() == server) {
    m_frontView = 0;
  }

  delete server->getStatusView();
  delete server;
}

void KonversationMainWindow::openToolbars()
{
  KEditToolbar dlg(actionCollection());

  if(dlg.exec())
    createGUI();
}

void KonversationMainWindow::setShowTabBarCloseButton(bool s)
{
  if(s) {
    viewContainer->cornerWidget()->show();
  } else {
    viewContainer->cornerWidget()->hide();
  }
}

void KonversationMainWindow::insertCharacter()
{
  if(!m_insertCharDialog) {
    ChatWindow* view = static_cast<ChatWindow*>(viewContainer->currentPage());
    QFont font;

    if(view && view->getTextView()) {
      font = view->getTextView()->font();
    } else if(view) {
      font = static_cast<QWidget*>(view)->font();
    }

    m_insertCharDialog = new Konversation::InsertCharDialog(font.family(), this);
    connect(m_insertCharDialog, SIGNAL(insertChar(const QChar&)), this, SLOT(insertChar(const QChar&)));
  }

  m_insertCharDialog->show();
}

void KonversationMainWindow::insertChar(const QChar& chr)
{
  ChatWindow* view = static_cast<ChatWindow*>(viewContainer->currentPage());

  if(view) {
    view->appendInputText(chr);
  }
}

void KonversationMainWindow::openIdentitiesDialog()
{
  Konversation::IdentityDialog dlg(this);

  if((dlg.exec() == KDialog::Accepted) && m_serverListDialog) {
    m_serverListDialog->updateServerGroupList();
  }
}

void KonversationMainWindow::updateChannelInfo(const QString &info)
{
  m_channelInfoLabel->setText(Konversation::removeIrcMarkup(info));
}

void KonversationMainWindow::showJoinChannelDialog()
{
  if(!frontServer) {
    return;
  }

  Konversation::JoinChannelDialog dlg(frontServer, this);

  if(dlg.exec() == QDialog::Accepted) {
    frontServer->sendJoinCommand(dlg.channel(), dlg.password());
  }
}

void KonversationMainWindow::reconnectCurrentServer()
{
  if(frontServer && !frontServer->isConnected() && !frontServer->isConnecting()) {
    frontServer->reconnect();
  }
}

void KonversationMainWindow::openURL(const QString&url, const QString&/* title*/)
{
  QString nUrl = url;
  
  QString serverAndPort = nUrl.remove("irc://").section('/',0,0);
  QString channelAndPassword = nUrl.section('/',1,1);

  QString server = serverAndPort.section(':',0,0);
  QString port = serverAndPort.section(':',1,1);

  QString channel = channelAndPassword.section('?',0,0);
  QString password = channelAndPassword.section('?',1,1);
  
  if(port.isEmpty())
    port = "6667";

  KonversationApplication::instance()->dcopConnectToServer(server,port.toInt(),channel,password);
  
}

QString KonversationMainWindow::currentURL()
{
  QString url = QString::null;
  QString channel = QString::null;

  if(frontServer && m_frontView)
    {
      updateFrontView();
      
      if(m_frontView->getType() == ChatWindow::Channel)
	channel = m_frontView->getName();

      url = "irc://"+frontServer->getServerName()+":"+QString::number(frontServer->getPort())+"/"+channel;
    }
  
  return url;
}

QString KonversationMainWindow::currentTitle()
{
  if(frontServer)
    {
      return frontServer->getServerName();
    }
  else
    return QString::null;
}

void KonversationMainWindow::serverStateChanged(Server* server, Server::State state)
{
  KAction* action = actionCollection()->action("reconnect_server");

  if(action && (frontServer == server)) {

    if(state != Server::SSDisconnected) {
      action->setEnabled(false);
    } else {
      action->setEnabled(true);
    }
  }
}

#include "konversationmainwindow.moc"
