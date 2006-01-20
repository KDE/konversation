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

#include <qpainter.h>
#include <qnamespace.h>
#include <qwhatsthis.h>
#include <qsignalmapper.h>
#include <qpoint.h>

#include <konvisettingsdialog.h>
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
#include <kabc/addressbook.h>
#include <kabc/errorhandler.h>
#include <kaccelmanager.h>

#include <config.h>
#ifdef  USE_KNOTIFY
#include <knotifydialog.h>
#endif


#include "linkaddressbook/addressbook.h"
#include "konversationmainwindow.h"
#include "konvibookmarkhandler.h"
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
#include "irccharsets.h"
#include "konviiphelper.h"
#include "images.h"
#include "konvisqueezedtextlabel.h"

KonversationMainWindow::KonversationMainWindow() : KMainWindow(0,"main_window", WStyle_ContextHelp | WType_TopLevel | WDestructiveClose)
{
    // Init variables before anything else can happen
    m_hasDirtySettings = false;
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
    m_popupTabIndex = -1;
    m_settingsDialog = NULL;

    images = KonversationApplication::instance()->images();

    dccTransferHandler=new DccTransferHandler(this);

    nicksOnlinePanel=0;

    viewContainer = new KTabWidget(this, "main_window_tab_widget");
    viewContainer->setTabReorderingEnabled(true);
    viewContainer->setTabCloseActivatePrevious(true);
    #if KDE_IS_VERSION(3,4,0)
    viewContainer->setAutomaticResizeTabs(true);
    #endif
    //  viewContainer->setHoverCloseButtonDelayed(false);
    setCentralWidget(viewContainer);
    updateTabPlacement();

    viewContainer->setHoverCloseButton(Preferences::closeButtons());

    viewContainer->hide();
    KPushButton* closeBtn = new KPushButton(viewContainer);
    closeBtn->setPixmap(KGlobal::iconLoader()->loadIcon("tab_remove", KIcon::Small));
    closeBtn->resize(22, 22);
    closeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    viewContainer->setCornerWidget(closeBtn);
    closeBtn->hide();
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(closeTab()));


    //used for event compression. See header file for resetHasDirtySettings()
    connect(KonversationApplication::instance(), SIGNAL(appearanceChanged()), this, SLOT(resetHasDirtySettings()));
    connect(KonversationApplication::instance(), SIGNAL(appearanceChanged()), this, SLOT(updateTrayIcon()));
                                                  // file_quit
    KStdAction::quit(this,SLOT(quitProgram()),actionCollection());

    setStandardToolBarMenuEnabled(true);
    createStandardStatusBarAction();

    // options_show_menubar
    showMenuBarAction=KStdAction::showMenubar(this,SLOT(showMenubar()),actionCollection());
    KStdAction::configureToolbars(this, SLOT(openToolbars()), actionCollection());

#ifdef USE_KNOTIFY // options_configure_notifications
    KAction *configureNotificationsAction = KStdAction::configureNotifications(this,SLOT(openNotifications()), actionCollection());
#endif

    // options_configure_key_binding
    KStdAction::keyBindings(this,SLOT(openKeyBindings()),actionCollection());
    // options_configure
    KAction *preferencesAction = KStdAction::preferences(this,SLOT(openPrefsDialog()),actionCollection());

    KAction* action;

    (new KAction(i18n("&Server List..."), "server", KShortcut("F2"), this, SLOT(openServerList()),
        actionCollection(), "open_server_list"))->setToolTip(i18n("Manage networks and servers"));
    (new KAction(i18n("Quick &Connect..."), "connect_creating", KShortcut("F7"), this, SLOT(openQuickConnectDialog()),
        actionCollection(), "quick_connect_dialog"))->setToolTip(i18n("Type in the address of a new IRC server to connect to"));

    action = new KAction(i18n("&Reconnect"), "connect_creating", 0, this, SLOT(reconnectCurrentServer()), actionCollection(), "reconnect_server");
    action->setEnabled(false);
    action->setToolTip("Reconnect to the current server.");

    (new KAction(i18n("&Identities..."), "identity", KShortcut("F8"), this, SLOT(openIdentitiesDialog()),
        actionCollection(), "identities_dialog"))->setToolTip(i18n("Manage your nick, away and other identity settings"));

    new KToggleAction(i18n("&Watched Nicks Online"), 0, KShortcut("F4"), this, SLOT(openNicksOnlinePanel()), actionCollection(), "open_nicksonline_window");
    action = new KAction(i18n("&Open Logfile"), "history", KShortcut("Ctrl+O"), this, SLOT(openLogfile()), actionCollection(), "open_logfile");
    action->setEnabled(false);
    action->setToolTip(i18n("Open the known history for this channel in a new tab"));

    action = new KAction(i18n("&Channel List"), 0, KShortcut("F5"), this, SLOT(openChannelList()), actionCollection(), "open_channel_list");
    action->setEnabled(false);
    action->setToolTip(i18n("Show a list of all the known channels on this server"));
    action = new KToggleAction(i18n("&URL Catcher"), 0, KShortcut("F6"), this, SLOT(addUrlCatcher()), actionCollection(), "open_url_catcher");
    action->setToolTip(i18n("List all URLs that have been mentioned recently in a new tab"));

    action = new KAction(i18n("New &Konsole"), "openterm", 0, this, SLOT(addKonsolePanel()), actionCollection(), "open_konsole");
    action->setToolTip(i18n("Open a terminal in a new tab"));


    // Actions to navigate through the different pages
    KShortcut nextShortcut = KStdAccel::tabNext();
    nextShortcut.setSeq(1, KKeySequence("Alt+Right"));
    KShortcut prevShortcut = KStdAccel::tabPrev();
    prevShortcut.setSeq(1, KKeySequence("Alt+Left"));
    action = new KAction(i18n("&Next Tab"), QApplication::reverseLayout() ? "previous" : "next",
        QApplication::reverseLayout() ? prevShortcut : nextShortcut,
        this,SLOT(nextTab()), actionCollection(), "next_tab");
    action->setEnabled(false);
    action = new KAction(i18n("&Previous Tab"), QApplication::reverseLayout() ? "next" : "previous",
        QApplication::reverseLayout() ? nextShortcut : prevShortcut,
        this,SLOT(previousTab()),actionCollection(),"previous_tab");
    action->setEnabled(false);
    action = new KAction(i18n("Close &Tab"),"tab_remove",KShortcut("Ctrl+w"),this,SLOT(closeTab()),actionCollection(),"close_tab");
    action->setEnabled(false);

    action = new KAction(i18n("Move Tab Left"), "1leftarrow", KShortcut("Alt+Shift+Left"),
        this, SLOT(moveTabLeft()), actionCollection(), "move_tab_left");
    action->setEnabled(false);
    action->setToolTip("Move the current tab");
    action = new KAction(i18n("Move Tab Right"), "1rightarrow", KShortcut("Alt+Shift+Right"),
        this, SLOT(moveTabRight()), actionCollection(), "move_tab_right");
    action->setEnabled(false);
    action->setToolTip("Move the current tab");

    action = new KToggleAction(i18n("Enable Notifications"), 0, 0, this, SLOT(toggleTabNotifications()), actionCollection(), "tab_notifications");
    action->setEnabled(false);


    KSelectAction* selectAction = new KSelectAction(i18n("Set Encoding"), "charset", 0, actionCollection(), "tab_encoding");
    selectAction->setEditable(false);
    QStringList encodingDescs = Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames();
    encodingDescs.prepend(i18n("Default"));
    selectAction->setItems(encodingDescs);
    selectAction->setEnabled(false);
    connect(selectAction, SIGNAL(activated(int)), this, SLOT(changeTabCharset(int)));

    selectAction = new KSelectAction(i18n("Switch To"), 0, 0, actionCollection(), "switch_to_tab");
    selectAction->setEditable(false);
    connect(selectAction, SIGNAL(activated(int)), this, SLOT(goToTab(int)));

    QSignalMapper* tabSelectionMapper = new QSignalMapper(this);
    connect(tabSelectionMapper, SIGNAL(mapped(int)), this, SLOT(goToTab(int)));

    for(uint i = 1; i <= 10; ++i)
    {
        KAction* tabSelectionAction = new KAction(i18n("Go to Tab %1").arg(i), 0, KShortcut(QString("Alt+%1").arg(i%10)),
            tabSelectionMapper, SLOT(map()), actionCollection(), QString("go_to_tab_%1").arg(i).local8Bit());
        tabSelectionMapper->setMapping( tabSelectionAction, i-1);
    }

    action = new KAction(i18n("&Clear Window"),0,KShortcut("Ctrl+L"),this,SLOT(clearWindow()),actionCollection(),"clear_window");
    action->setToolTip(i18n("Clear the contents of the current tab"));
    action->setEnabled(false);
    action = new KAction(i18n("Clear &All Windows"),0,KShortcut("CTRL+SHIFT+L"),this,SLOT(clearTabs()),actionCollection(),"clear_tabs");
    action->setToolTip(i18n("Clear the contents of all open tabs"));
    action->setEnabled(false);

    KAction* awayAction = new KAction(i18n("Set &Away Globally")/*, "konversationaway"*/, KShortcut("Ctrl+Alt+A"),
        static_cast<KonversationApplication *>(kapp), SLOT(toggleAway()), actionCollection(),"toggle_away");
    awayAction->setEnabled(false);
    awayAction->setToolTip("Switch to Away mode in all open connections");

    action = new KAction(i18n("&Join Channel..."), 0, KShortcut("Ctrl+J"), this, SLOT(showJoinChannelDialog()), actionCollection(), "join_channel");
    action->setEnabled(false);
    action->setToolTip("Join a new channel on this server");

    action = KStdAction::find(this, SLOT(findText()), actionCollection());
    action->setEnabled(false);
    action = KStdAction::findNext(this, SLOT(findNextText()), actionCollection());
    action->setEnabled(false);

    action = new KAction(i18n("&IRC Color..."), "colorize", CTRL+Key_K, this, SLOT(addIRCColor()), actionCollection(), "irc_colors");
    action->setToolTip(i18n("Set the color of your current IRC message"));
    action->setEnabled(false);
    action = new KAction(i18n("&Remember Line"), 0,  KShortcut("Ctrl+R") , this, SLOT(insertRememberLine()), actionCollection(), "insert_remember_line");
    action->setToolTip(i18n("Insert a horizontal line into the current tab that only you can see"));
    action->setEnabled(false);
    action = new KAction(i18n("Special &Character..."), "char", KShortcut("Alt+Shift+C"), this, SLOT(insertCharacter()), actionCollection(), "insert_character");
    action->setToolTip(i18n("Insert any character into your current IRC message"));
    action->setEnabled(false);

    new KAction(i18n("Close &All Open Queries"), 0, KShortcut("F11"), this, SLOT(closeQueries()), actionCollection(), "close_queries");
    hideNicklistAction = new KToggleAction(i18n("Hide Nicklist"), 0, KShortcut("Ctrl+H"), this, SLOT(hideNicknameList()), actionCollection(), "hide_nicknamelist");
    if(!Preferences::showNickList())
        hideNicklistAction->setChecked(true);


    // Initialize KMainWindow->statusBar()
    statusBar();
    m_sslLabel = new SSLLabel(statusBar(),"sslLabel");
    m_sslLabel->setPixmap(SmallIcon("encrypted"));
    m_sslLabel->hide();
    QWhatsThis::add(m_sslLabel, i18n("All communication with the server is encrypted.  This makes it harder for someone to listen in on your communications."));

    int statH = fontMetrics().height()+2;

    m_generalInfoLabel = new KonviSqueezedTextLabel(i18n("Ready."), statusBar());
    m_generalInfoLabel->setSizePolicy(QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ));
    m_generalInfoLabel->setMinimumWidth( 0 );
    m_generalInfoLabel->setFixedHeight( statH );

    m_channelInfoLabel = new QLabel(statusBar(), "channelInfoLabel");
    QWhatsThis::add(m_channelInfoLabel, i18n("<qt>This shows the number of users in the channel, and the number of those that are operators (ops).<p>A channel operator is a user that has special privileges, such as the ability to kick and ban users, change the channel modes, make other users operators</qt>"));

    m_lagInfoLabel = new QLabel(i18n("Lag: Unknown"), statusBar(), "lagInfoLabel");

    statusBar()->addWidget(m_generalInfoLabel, 1, false);
    statusBar()->addWidget(m_channelInfoLabel, 0, true);
    statusBar()->addWidget(m_lagInfoLabel, 0, true);

    statusBar()->addWidget(m_sslLabel, 0, true);

    QWhatsThis::add(statusBar(), i18n("<qt>The status bar shows various messages, including any problems connecting to the server.  On the far right the current delay to the server is shown.  The delay is the time it takes for messages from you to reach the server, and from the server back to you.</qt>"));

    actionCollection()->setHighlightingEnabled(true);
    connect(actionCollection(), SIGNAL( actionStatusText( const QString & ) ),
        m_generalInfoLabel, SLOT( setTempText( const QString & ) ) );
    connect(actionCollection(), SIGNAL( clearStatusText() ),
        m_generalInfoLabel, SLOT( clearTempText() ) );

    connect( viewContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changeView(QWidget*)) );
    connect( viewContainer, SIGNAL(closeRequest(QWidget*)), this, SLOT(closeView(QWidget*)));
    connect( viewContainer, SIGNAL(contextMenu(QWidget*, const QPoint&)), this, SLOT(showTabContextMenu(QWidget*, const QPoint&)));

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

    resize(700, 500);                             // Give the app a sane default size
    setAutoSaveSettings();
    showMenuBarAction->setChecked(Preferences::showMenuBar());
    showMenubar(true);

    // set up KABC with a nice gui error dialog
    KABC::GuiErrorHandler *m_guiErrorHandler = new KABC::GuiErrorHandler(this);
    kapp->dcopClient()->setAcceptCalls( false );
    Konversation::Addressbook::self()->getAddressBook()->setErrorHandler(m_guiErrorHandler);
    kapp->dcopClient()->setAcceptCalls( true );

    // demo how to add additional dock windows
    //  QListView* dockList=new QListView(this);
    //  addToolWindow(dockList,KDockWidget::DockLeft,getMainDockWidget());

    if(Preferences::useNotify() && Preferences::openWatchedNicksAtStartup())
    {
        openNicksOnlinePanel();
    }
}

KonversationMainWindow::~KonversationMainWindow()
{
    Preferences::writeConfig();
    deleteDccPanel();
    delete dccTransferHandler;
}

void KonversationMainWindow::updateTabPlacement()
{
    viewContainer->setTabPosition((Preferences::tabPlacement()==Preferences::Top) ?
        QTabWidget::Top : QTabWidget::Bottom);
}

void KonversationMainWindow::openPrefsDialog()
{
    //An instance of your dialog could be already created and could be cached,
    //in which case you want to display the cached dialog instead of creating
    //another one
    if(!m_settingsDialog)
    {
        m_settingsDialog = new KonviSettingsDialog(this);
        //User edited the configuration - update your local copies of the
        //configuration data
        connect(m_settingsDialog, SIGNAL(settingsChanged()), this, SLOT(settingsChangedSlot()));
    }
    m_settingsDialog->show();
}

void KonversationMainWindow::settingsChangedSlot()
{
    // This is for compressing the events. m_hasDirtySettings is set to true
    // when the settings have changed, then set to false when the app reacts to it
    // via the appearanceChanged signal.  This prevents a series of settingsChanged signals
    // causing the app expensively rereading its settings many times.
    // The appearanceChanged signal is connected to resetHasDirtySettings to reset this bool
    if(!m_hasDirtySettings) 
    {
        QTimer::singleShot(0, KonversationApplication::instance(), SIGNAL(appearanceChanged()));
        m_hasDirtySettings = true;
    }
}

void KonversationMainWindow::resetHasDirtySettings() 
{
    m_hasDirtySettings = false;
}

void KonversationMainWindow::openKeyBindings()
{
    KKeyDialog::configure(actionCollection());
}

void KonversationMainWindow::showToolbar()
{
}

void KonversationMainWindow::focusAndShowErrorMessage(const QString &errorMsg)
{
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
        if(!dontShowWarning)
        {
            QString accel=showMenuBarAction->shortcut().toString();
            KMessageBox::information(this,i18n("<qt>This will hide the menu bar completely."
                "You can show it again by typing %1.</qt>").arg(accel),
                "Hide menu bar","HideMenuBarWarning");
        }
        menuBar()->hide();
    }

    Preferences::setShowMenuBar(showMenuBarAction->isChecked());
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
        !Preferences::redirectServerAndAppMsgToStatusPane()
        )
        m_frontView->appendServerMessage(type,message);
}

void KonversationMainWindow::appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView)
{
    if( !serverView) serverView = m_frontView->getServer()->getStatusView();

                                                  //if this fails, we need to fix frontServer
    Q_ASSERT(m_frontView && m_frontView->getServer() == frontServer);

    Q_ASSERT(serverView); if(!serverView) return;
    updateFrontView();
    if(!m_frontView ||                            // Check if the m_frontView can actually display text or ...
                                                  // if it does not belong to this server or...
        serverView->getServer()!=m_frontView->getServer() ||
                                                  // if the user decided to force it.
        Preferences::redirectServerAndAppMsgToStatusPane())
    {
        // if not, take server specified fallback view instead
        serverView->appendServerMessage(type,message);
        // FIXME: this signal should be sent from the status panel instead, so it
        //        can be using the correct highlight color, would be more consistent
        //        anyway!
        // FIXME newText(serverView,QString::null,true);
    }
    else
        m_frontView->appendServerMessage(type,message);
}

void KonversationMainWindow::updateAppearance()
{
    updateTabPlacement();
    setShowTabBarCloseButton(Preferences::showTabBarCloseButton());

    int statH = fontMetrics().height()+2;
    m_generalInfoLabel->setFixedHeight( statH );

    hideNicklistAction->setChecked(!Preferences::showNickList());
}

void KonversationMainWindow::addView(ChatWindow* view, const QString& label, bool weinitiated)
{
    // TODO: Make sure to add DCC status tab at the end of the list and all others
    // before the DCC tab. Maybe we should also make sure to order Channels
    // Queries and DCC chats in groups
    ChatWindow *tmp_ChatWindow;
    int placement = -1;
    ChatWindow::WindowType wtype;
    QIconSet iconSet;

    connect(view, SIGNAL( actionStatusText( const QString & ) ),
            m_generalInfoLabel, SLOT( setTempText( const QString & ) ) );
    connect(view, SIGNAL( clearStatusText() ),
            m_generalInfoLabel, SLOT( clearTempText() ) );

    switch (view->getType())
    {
        case ChatWindow::Channel:
            if (Preferences::tabNotificationsLeds())
                iconSet = images->getMsgsLed(false);
            else if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();

            for (int sindex = 0; sindex < viewContainer->count(); sindex++)
            {
                tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(sindex));

                if (tmp_ChatWindow->getType() == ChatWindow::Status && tmp_ChatWindow->getServer() == view->getServer())
                {
                    for (int index = sindex + 1; index < viewContainer->count(); index++)
                    {
                        tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(index));
                        wtype = tmp_ChatWindow->getType();

                        if (wtype != ChatWindow::Channel)
                        {
                            placement = index;
                            break;
                        }
                    }

                    break;
                }
            }

            break;

        case ChatWindow::RawLog:
            if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();

            for (int sindex = 0; sindex < viewContainer->count(); sindex++)
            {
                tmp_ChatWindow = static_cast<ChatWindow *>(viewContainer->page(sindex));

                if (tmp_ChatWindow->getType() == ChatWindow::Status && tmp_ChatWindow->getServer() == view->getServer())
                {
                    placement = sindex;
                    break;
                }
            }

            break;

        case ChatWindow::Query:
            if(Preferences::tabNotificationsLeds())
                iconSet = images->getMsgsLed(false);
            else if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();

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
                            placement = index;
                            break;
                        }
                    }

                    break;
                }
            }

            break;

        case ChatWindow::DccChat:
            if(Preferences::tabNotificationsLeds())
                iconSet = images->getMsgsLed(false);
            else if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();

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
                            placement = index;
                            break;
                        }
                    }

                    break;
                }
            }
        case ChatWindow::Status:
            if(Preferences::tabNotificationsLeds())
                iconSet = images->getServerLed(false);
            else if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();
            break;

        case ChatWindow::Konsole:
            if(Preferences::tabNotificationsLeds())
                iconSet = images->getMsgsLed(false);
            else if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();
            break;

        default:
            if (Preferences::closeButtons())
                iconSet = images->getCloseIcon();
            break;
    }

    viewContainer->insertTab(view, iconSet, label, placement);
    viewContainer->show();

    // Check, if user was typing in old input line
    bool doBringToFront=true;

    // make sure that bring to front only works when the user wasn't typing something
    if(m_frontView && view->getType() != ChatWindow::UrlCatcher &&
        view->getType() != ChatWindow::Konsole)
    {
        if(!m_frontView->getTextInLine().isEmpty()) doBringToFront=false;
    }

    if(!Preferences::focusNewQueries() && view->getType()==ChatWindow::Query && !weinitiated)
        doBringToFront = false;

    // bring view to front unless it's a raw log window or the user was typing
    if(Preferences::bringToFront() && doBringToFront &&
        view->getType()!=ChatWindow::RawLog)
    {
        showView(view);
    }

    updateTabMoveActions(getViewContainer()->currentPageIndex());
    updateSwitchTabAction();

    // FIXME  connect(view,SIGNAL (online(ChatWindow*,bool)),viewContainer,SLOT (setTabOnline(ChatWindow*,bool)) );
}

void KonversationMainWindow::showView(ChatWindow* view)
{
    // Don't bring Tab to front if TabWidget is hidden. Otherwise QT gets confused
    // and shows the Tab as active but will display the wrong pane
    if(viewContainer->isVisible())
    {
        // TODO: add adjustFocus() here?
        viewContainer->showPage(view);            //This does will changeView(view) via slots
    }
}

void KonversationMainWindow::closeView(QWidget* viewToClose)
{
    ChatWindow* view=static_cast<ChatWindow*>(viewToClose);
    if(view)
    {

        ChatWindow::WindowType viewType=view->getType();

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
            return;                               //We haven't done anything yet, so safe to return

        // if this view was the front view, delete the pointer
        if(view==previousFrontView) previousFrontView=0;
        if(view==m_frontView) m_frontView=previousFrontView;

        viewContainer->removePage(view);

        if(viewContainer->count() <= 0)
        {
            viewContainer->hide();
        }
    }

    updateTabMoveActions(getViewContainer()->currentPageIndex());
    updateSwitchTabAction();
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
    if(!file.isEmpty())
    {
        LogfileReader* logReader = new LogfileReader(getViewContainer(), file);
        addView(logReader, i18n("Logfile of %1").arg(caption));
        logReader->setServer(0);
    }
}

void KonversationMainWindow::addKonsolePanel()
{
    KonsolePanel* panel=new KonsolePanel(getViewContainer());
    panel->setName(i18n("Konsole"));
    addView(panel, i18n("Konsole"));
    connect(panel, SIGNAL(updateTabNotification(ChatWindow*,const Konversation::TabNotifyType&)), this, SLOT(setTabNotification(ChatWindow*,const Konversation::TabNotifyType&)));
    connect(panel,SIGNAL (deleted(ChatWindow*)),this,SLOT (closeKonsolePanel(ChatWindow*)) );
    panel->setMainWindow(this);
}

void KonversationMainWindow::closeKonsolePanel(ChatWindow* konsolePanel)
{
    getViewContainer()->removePage(konsolePanel);
    // tell QT to delete the panel during the next event loop since we are inside a signal here
    konsolePanel->deleteLater();
}

void KonversationMainWindow::openChannelList(const QString& filter, bool getList)
{
    if(frontServer)
    {
        ChannelListPanel* panel = frontServer->getChannelListPanel();

        if(panel)
        {
            getViewContainer()->showPage(panel);
        }
        else
        {
            int ret = KMessageBox::Continue;

            if(filter.isEmpty())
            {
                ret = KMessageBox::warningContinueCancel(this,i18n("Using this function may result in a lot "
                      "of network traffic. If your connection is not fast "
                      "enough, it is possible that your client will be "
                      "disconnected by the server."), i18n("Channel List Warning"),
                      KStdGuiItem::cont(), "ChannelListWarning");
            }

            if(ret != KMessageBox::Continue)
            {
                return;
            }

            panel = frontServer->addChannelListPanel();
        }

        panel->setFilter(filter);

        if(getList)
        {
            panel->applyFilterClicked();
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
        addView(urlCatcherPanel, i18n("URL Catcher"));
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
        }                                         // for
        (dynamic_cast<KToggleAction*>(actionCollection()->action("open_url_catcher")))->setChecked(true);
    }
    else if((ChatWindow *)m_frontView != (ChatWindow *)urlCatcherPanel)
    {
        showView(urlCatcherPanel);
        (dynamic_cast<KToggleAction*>(actionCollection()->action("open_url_catcher")))->setChecked(true);
    }
    else
    {
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
        addView(dccPanel, i18n("DCC Status"));
        dccPanel->setMainWindow(this);
        dccPanelOpen=true;
    }
    // show already opened panel
    else
    {
        if(!dccPanelOpen)
        {
            addView(dccPanel, i18n("DCC Status"));
            dccPanelOpen=true;
        }
        // no highlight color for DCC panels
        // FIXME newText(dccPanel,QString::null,true);
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
        addView(dccChatPanel, dccChatPanel->getName());
        connect(dccChatPanel, SIGNAL(updateTabNotification(ChatWindow*,const Konversation::TabNotifyType&)), this, SLOT(setTabNotification(ChatWindow*,const Konversation::TabNotifyType&)));
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

    // Get group name for tab if available
    QString label = server->getServerGroup();
    if (label.isEmpty())
    {
        label = server->getServerName();
    }

    statusView->setName(label);

    // SSL icon stuff
    QObject::connect(server,SIGNAL(sslInitFailure()),this,SLOT(removeSSLIcon()));
    QObject::connect(server,SIGNAL(sslConnected(Server*)),this,SLOT(updateSSLInfo(Server*)));

    // ... then put it into the tab widget, otherwise we'd have a race with server member
    addView(statusView,label);

    connect(this, SIGNAL(prefsChanged()), statusView, SLOT(updateName()));
    connect(statusView, SIGNAL(updateTabNotification(ChatWindow*,const Konversation::TabNotifyType&)), this, SLOT(setTabNotification(ChatWindow*,const Konversation::TabNotifyType&)));
    connect(statusView,SIGNAL (sendFile()),server,SLOT (requestDccSend()) );
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
    addView(channel, newname);

    connect(this, SIGNAL(updateChannelAppearance()), channel, SLOT(updateAppearance()));
    connect(channel, SIGNAL(updateTabNotification(ChatWindow*,const Konversation::TabNotifyType&)), this, SLOT(setTabNotification(ChatWindow*,const Konversation::TabNotifyType&)));
    connect(server, SIGNAL(awayState(bool)),channel, SLOT(indicateAway(bool)) );

    return channel;
}

Query* KonversationMainWindow::addQuery(Server* server, const NickInfoPtr& nickInfo, bool weinitiated)
{
    QString name = nickInfo->getNickname();
    Query* query=new Query(getViewContainer());
    query->setServer(server);
    query->setNickInfo(nickInfo);
    addView(query, name, weinitiated);

    connect(query, SIGNAL(updateTabNotification(ChatWindow*,const Konversation::TabNotifyType&)), this, SLOT(setTabNotification(ChatWindow*,const Konversation::TabNotifyType&)));
    connect(server, SIGNAL(awayState(bool)), query, SLOT(indicateAway(bool)));

    return query;
}

RawLog* KonversationMainWindow::addRawLog(Server* server)
{
    RawLog* rawLog=new RawLog(getViewContainer());
    rawLog->setServer(server);
    rawLog->setLog(false);
    addView(rawLog, i18n("Raw Log"));

    return rawLog;
}

ChannelListPanel* KonversationMainWindow::addChannelListPanel(Server* server)
{
    ChannelListPanel* channelListPanel=new ChannelListPanel(getViewContainer());
    channelListPanel->setServer(server);
    addView(channelListPanel, i18n("Channel List"));

    return channelListPanel;
}

void KonversationMainWindow::setTabNotification(ChatWindow* view, const Konversation::TabNotifyType& type)
{
    if(view==getViewContainer()->currentPage())
        return;

    if (!Preferences::tabNotificationsLeds() && !Preferences::tabNotificationsText())
        return;

    switch(type)
    {
        case Konversation::tnfNormal:
            if (Preferences::tabNotificationsMsgs())
            {
                if (Preferences::tabNotificationsLeds())
                    getViewContainer()->setTabIconSet(view, images->getMsgsLed(true));
                if (Preferences::tabNotificationsText())
                    getViewContainer()->setTabColor(view, Preferences::tabNotificationsMsgsColor());
            }
            break;

        case Konversation::tnfControl:
            if (Preferences::tabNotificationsEvents())
            {
                if (Preferences::tabNotificationsLeds())
                    getViewContainer()->setTabIconSet(view, images->getEventsLed());
                if (Preferences::tabNotificationsText())
                    getViewContainer()->setTabColor(view, Preferences::tabNotificationsEventsColor());
            }
            break;

        case Konversation::tnfNick:
            if (Preferences::tabNotificationsNick())
            {
                if (Preferences::tabNotificationsOverride() && Preferences::highlightNick())
                {
                    if (Preferences::tabNotificationsLeds())
                        getViewContainer()->setTabIconSet(view, images->getLed(Preferences::highlightNickColor(),true));
                    if (Preferences::tabNotificationsText())
                        getViewContainer()->setTabColor(view, Preferences::highlightNickColor());
                }
                else
                {
                    if (Preferences::tabNotificationsLeds())
                        getViewContainer()->setTabIconSet(view, images->getNickLed());
                    if (Preferences::tabNotificationsText())
                        getViewContainer()->setTabColor(view, Preferences::tabNotificationsNickColor());
                }
            }
            break;

        case Konversation::tnfHighlight:
            if (Preferences::tabNotificationsHighlights())
            {
                if (Preferences::tabNotificationsOverride() && view->highlightColor().isValid())
                {
                    if (Preferences::tabNotificationsLeds())
                        getViewContainer()->setTabIconSet(view, images->getLed(view->highlightColor(),true));
                    if (Preferences::tabNotificationsText())
                        getViewContainer()->setTabColor(view, view->highlightColor());
                }
                else
                {
                    if (Preferences::tabNotificationsLeds())
                        getViewContainer()->setTabIconSet(view, images->getHighlightsLed());
                    if (Preferences::tabNotificationsText())
                        getViewContainer()->setTabColor(view, Preferences::tabNotificationsHighlightsColor());
                }
            }
            break;

        default:
            break;
    }
}

void KonversationMainWindow::unsetTabNotification(ChatWindow* view)
{
    if (Preferences::tabNotificationsLeds())
    {
        switch (view->getType())
        {
            case ChatWindow::Channel:
            case ChatWindow::Query:
            case ChatWindow::DccChat:
            case ChatWindow::Konsole:
                getViewContainer()->setTabIconSet(view, images->getMsgsLed(false));
                break;

            case ChatWindow::Status:
                getViewContainer()->setTabIconSet(view, images->getServerLed(false));
                break;

            default:
                break;
        }
    }

    getViewContainer()->setTabColor(view, colorGroup().foreground());
}

void KonversationMainWindow::updateTabs()
{
    if (Preferences::closeButtons() && !viewContainer->hoverCloseButton())
        viewContainer->setHoverCloseButton(true);

    if (!Preferences::closeButtons() && viewContainer->hoverCloseButton())
        viewContainer->setHoverCloseButton(false);

    for (int i = 0; i < viewContainer->count(); ++i)
    {
        ChatWindow* view = static_cast<ChatWindow*>(viewContainer->page(i));

        if (view->getType()==ChatWindow::Status)
        {
            QString label = view->getServer()->serverGroupSettings()->name();

            if (getViewContainer()->tabLabel(view) != label)
            {
                getViewContainer()->setTabLabel(view,label);
                if (view==m_frontView)
                {
                    m_channelInfoLabel->setText(label);
                    setCaption(label);
                }
            }
        }

        if (!Preferences::tabNotificationsLeds() && !Preferences::closeButtons())
            getViewContainer()->setTabIconSet(view, QIconSet());

        if (Preferences::closeButtons() && !Preferences::tabNotificationsLeds())
            getViewContainer()->setTabIconSet(view, images->getCloseIcon());

        if (!Preferences::tabNotificationsText())
            getViewContainer()->setTabColor(view, colorGroup().foreground());

        if (Preferences::tabNotificationsLeds() || Preferences::tabNotificationsText())
        {
            if (view->currentTabNotification()==Konversation::tnfNone)
                unsetTabNotification(view);
            else if (view->currentTabNotification()==Konversation::tnfNormal && !Preferences::tabNotificationsMsgs())
                unsetTabNotification(view);
            else if (view->currentTabNotification()==Konversation::tnfControl && !Preferences::tabNotificationsEvents())
                unsetTabNotification(view);
            else if (view->currentTabNotification()==Konversation::tnfNick && !Preferences::tabNotificationsNick())
                unsetTabNotification(view);
            else if (view->currentTabNotification()==Konversation::tnfHighlight && !Preferences::tabNotificationsHighlights())
                unsetTabNotification(view);
            else if (view==getViewContainer()->currentPage())
                unsetTabNotification(view);
            else
                setTabNotification(view,view->currentTabNotification());
        }
    }
}

void KonversationMainWindow::updateFrontView()
{
    ChatWindow* view = static_cast<ChatWindow*>(getViewContainer()->currentPage());
    KAction* action;

    if(view)
    {
        // Make sure that only views with info output get to be the m_frontView
        if(m_frontView)
        {
            previousFrontView = m_frontView;
            disconnect(m_frontView, SIGNAL(updateInfo(const QString &)), this, SLOT(updateChannelInfo(const QString &)));
        }

        if(view->canBeFrontView())
        {
            m_frontView = view;

            connect(view, SIGNAL(updateInfo(const QString &)), this, SLOT(updateChannelInfo(const QString &)));
            view->emitUpdateInfo();
        }
        else
        {
            QString tabName = Konversation::removeIrcMarkup(view->getName());

            if( tabName != "ChatWindowObject" )
                m_channelInfoLabel->setText(tabName);
            else
                m_channelInfoLabel->setText(QString::null);
        }

        switch (view->getType())
        {
            case ChatWindow::Channel:
            case ChatWindow::Query:
            case ChatWindow::DccChat:
            case ChatWindow::Status:
                m_lagInfoLabel->show();
                break;

            default:
                m_lagInfoLabel->hide();
                break;
        }

        // Make sure that only text views get to be the searchView
        if(view->searchView())
        {
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
        if(action)
        {
            action->setText(i18n("Find Text..."));
            action->setEnabled(view->searchView());
            action->setToolTip("Search for text in the current tab");
        }

        action = actionCollection()->action("edit_find_next");
        if(action) action->setEnabled(view->searchView());

        action = actionCollection()->action("open_channel_list");
        if(action)
        {
            if(view->getServer())
            {
                action->setEnabled(true);
                action->setText(i18n("&Channel List for %1").arg(view->getServer()->getServerGroup()));
            }
            else
            {
                action->setEnabled(false);
                action->setText(i18n("&Channel List"));
            }
        }

        action = actionCollection()->action("join_channel");
        if(action) action->setEnabled(view->getServer() != 0);

        action = actionCollection()->action("open_logfile");
        if(action)
        {
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
        if(action)
        {
            Server* server = view->getServer();

            if(server && !server->isConnected())
            {
                action->setEnabled(true);
            }
            else
            {
                action->setEnabled(false);
            }
        }
    }
    else
    {
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

    if(m_frontView)
    {
    m_frontView->resetTabNotification();
    m_frontView->lostFocus();
        previousFrontView = m_frontView;
        disconnect(m_frontView, SIGNAL(updateInfo(const QString &)), this, SLOT(updateChannelInfo(const QString &)));
    }

    m_frontView = 0;
    searchView = 0;

    frontServer = view->getServer();
    // display this server's lag time
    if(frontServer)
    {
        updateSSLInfo(frontServer);
        updateLag(frontServer,frontServer->getLag());
    }

    m_generalInfoLabel->clearTempText();

    updateFrontView();

    unsetTabNotification(view);
    view->resetTabNotification();

    view->adjustFocus();

    updateTabMoveActions(getViewContainer()->currentPageIndex());

    if(view)
    {
        KToggleAction* notifyAction = static_cast<KToggleAction*>(actionCollection()->action("tab_notifications"));
        if(notifyAction)
        {
            ChatWindow::WindowType viewType = view->getType();
            notifyAction->setEnabled(viewType == ChatWindow::Channel || viewType == ChatWindow::Query || viewType == ChatWindow::Status);
            notifyAction->setChecked(view->notificationsEnabled());
        }

        updateTabEncoding(view);
    }

    QString tabName = Konversation::removeIrcMarkup(view->getName());

    if( tabName != "ChatWindowObject" )
    {
        setCaption(tabName);
    }
    else
    {
        setCaption(QString::null);
    }
}

bool KonversationMainWindow::queryClose()
{
    KonversationApplication* konv_app = static_cast<KonversationApplication*>(kapp);

    if(konv_app->sessionSaving() || sender() == tray)
    {
        m_closeApp = true;
    }

    if(Preferences::showTrayIcon() && !m_closeApp)
    {

        // Compute size and position of the pixmap to be grabbed:
        QPoint g = tray->mapToGlobal( tray->pos() );
        int desktopWidth  = kapp->desktop()->width();
        int desktopHeight = kapp->desktop()->height();
        int tw = tray->width();
        int th = tray->height();
        int w = desktopWidth / 4;
        int h = desktopHeight / 9;
        int x = g.x() + tw/2 - w/2;               // Center the rectange in the systray icon
        int y = g.y() + th/2 - h/2;
        if ( x < 0 )                 x = 0;       // Move the rectangle to stay in the desktop limits
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
        addView(nicksOnlinePanel, i18n("Nicks Online"));
        nicksOnlinePanel->setMainWindow(this);
        connect(nicksOnlinePanel,SIGNAL (editClicked()),this,SLOT (openNotify()) );

        connect(nicksOnlinePanel,SIGNAL (doubleClicked(const QString&,const QString&)),this,SLOT (notifyAction(const QString&,const QString&)) );

        connect(this,SIGNAL (nicksNowOnline(Server*)),nicksOnlinePanel,SLOT (updateServerOnlineList(Server*)) );
        (dynamic_cast<KToggleAction*>(actionCollection()->action("open_nicksonline_window")))->setChecked(true);
    }
    else
    {
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
    if(!m_serverListDialog)
    {
        m_serverListDialog = new Konversation::ServerListDialog(this);
        KonversationApplication *konvApp = static_cast<KonversationApplication *>(KApplication::kApplication());
        connect(m_serverListDialog, SIGNAL(serverGroupsChanged()), konvApp, SLOT(saveOptions()));
        connect(m_serverListDialog, SIGNAL(serverGroupsChanged()), this, SIGNAL(prefsChanged()));
        connect(m_serverListDialog, SIGNAL(serverGroupsChanged()), this, SLOT(updateTabs()));
        connect(m_serverListDialog, SIGNAL(connectToServer(int)), konvApp, SLOT(connectToServer(int)));
        connect(m_serverListDialog, SIGNAL(connectToServer(int, Konversation::ServerSettings)), konvApp, SLOT(connectToServer(int, Konversation::ServerSettings)));
        connect(konvApp, SIGNAL(closeServerList()), m_serverListDialog, SLOT(slotClose()));
    }

    m_serverListDialog->show();
}

void KonversationMainWindow::openQuickConnectDialog()
{
    emit showQuickConnectDialog();
}

// open the preferences dialog and show the watched nicknames page
void KonversationMainWindow::openNotify()
{
  openPrefsDialog();
  if(m_settingsDialog) m_settingsDialog->openWatchedNicknamesPage();
}

// TODO: Let an own class handle notify things
void KonversationMainWindow::setOnlineList(Server* notifyServer,const QStringList& /*list*/, bool /*changed*/)
{
    emit nicksNowOnline(notifyServer);
    // FIXME  if (changed && nicksOnlinePanel) newText(nicksOnlinePanel, QString::null, true);
}

void KonversationMainWindow::notifyAction(const QString& serverName,const QString& nick)
{
    KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
    Server* server=konv_app->getServerByName(serverName);
    server->notifyAction(nick);
}

void KonversationMainWindow::removeSSLIcon()
{
    disconnect(m_sslLabel,0,0,0);
    m_sslLabel->hide();
}

KTabWidget* KonversationMainWindow::getViewContainer()
{
    return viewContainer;
}

void KonversationMainWindow::updateLag(Server* lagServer,int msec)
{
    // show lag only of actual server
    if(lagServer==frontServer)
    {
        m_generalInfoLabel->setText(i18n("Ready."));
        QString lagString = lagServer->getServerName() + " - ";

        if (msec == -1)
        {
            lagString += i18n("Lag: Unknown");
        }
        else if(msec < 1000)
        {
            lagString += i18n("Lag: %1 ms").arg(msec);
        }
        else
        {
            lagString += i18n("Lag: %1 s").arg(msec / 1000);
        }

        m_lagInfoLabel->setText(lagString);
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

        if(days)
        {
            const QString daysString = i18n("1 day", "%n days", days);
            const QString hoursString = i18n("1 hour", "%n hours", (hours % 24));
            const QString minutesString = i18n("1 minute", "%n minutes", (minutes % 60));
            const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
            lagString = i18n("%1 = name of server, %2 = (x days), %3 = (x hours), %4 = (x minutes), %5 = (x seconds)", "No answer from server %1 for more than %2, %3, %4, and %5.").arg(lagServer->getServerName())
                .arg(daysString).arg(hoursString).arg(minutesString).arg(secondsString);
            // or longer than an hour
        }
        else if(hours)
        {
            const QString hoursString = i18n("1 hour", "%n hours", hours);
            const QString minutesString = i18n("1 minute", "%n minutes", (minutes % 60));
            const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
            lagString = i18n("%1 = name of server, %2 = (x hours), %3 = (x minutes), %4 = (x seconds)", "No answer from server %1 for more than %2, %3, and %4.").arg(lagServer->getServerName())
                .arg(hoursString).arg(minutesString).arg(secondsString);
            // or longer than a minute
        }
        else if(minutes)
        {
            const QString minutesString = i18n("1 minute", "%n minutes", minutes);
            const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
            lagString = i18n("%1 = name of server, %2 = (x minutes), %3 = (x seconds)", "No answer from server %1 for more than %2 and %3.").arg(lagServer->getServerName())
                .arg(minutesString).arg(secondsString);
            // or just some seconds
        }
        else
        {
            lagString = i18n("No answer from server %1 for more than 1 second.", "No answer from server %1 for more than %n seconds.", seconds).arg(lagServer->getServerName());
        }

        m_generalInfoLabel->setText(lagString);
    }
    if(lagServer==frontServer)
    {
        //show lag only of actual server
        QString lagString(i18n("Lag: %1 s").arg(msec/1000));
        m_lagInfoLabel->setText(lagString);
    }
}

// TODO: Make this server dependant
void KonversationMainWindow::resetLag()
{
    m_lagInfoLabel->setText(i18n("Lag: Unknown"));
}

void KonversationMainWindow::closeTab()
{
    if(m_popupTabIndex == -1)
    {
        closeView(getViewContainer()->currentPage());
    }
    else
    {
        closeView(getViewContainer()->page(m_popupTabIndex));
    }

    m_popupTabIndex = -1;
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
    if(page >= getViewContainer()->count())
        page = 0;
    else if(page < 0)
        page = getViewContainer()->count() - 1;

    if(page >= 0)
    {
        getViewContainer()->setCurrentPage(page);
        ChatWindow* newPage=static_cast<ChatWindow*>(getViewContainer()->page(page));
        newPage->adjustFocus();
    }

    m_popupTabIndex = -1;
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
    tray->setNotificationEnabled(Preferences::trayNotify());

    if(Preferences::showTrayIcon())
        tray->show();
    else
        tray->hide();

    if(Preferences::showTrayIcon() && Preferences::systrayOnly())
        KWin::setState(winId(), NET::SkipTaskbar);
    else
        KWin::clearState(winId(), NET::SkipTaskbar);
}

void KonversationMainWindow::addIRCColor()
{
    IRCColorChooser dlg(this);

    if(dlg.exec() == QDialog::Accepted)
    {
        m_frontView->appendInputText(dlg.color());
    }
}

void KonversationMainWindow::insertRememberLine()
{
    if(Preferences::showRememberLineInAllWindows())
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

void KonversationMainWindow::insertRememberLine(Server* server)
{
    for (int i = 0; i < viewContainer->count(); ++i)
    {
        ChatWindow* view = static_cast<ChatWindow*>(viewContainer->page(i));

        if (view->getServer()==server && 
            (view->getType()==ChatWindow::Channel || view->getType()==ChatWindow::Query))
        {
            view->insertRememberLine();
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

        if(nextPage && nextPage->getType()==ChatWindow::Query)
        {
            nextPage->closeYourself();
            --i;                                  /* Tab indexes changed */
        }
        ++operations;
    }
}

void KonversationMainWindow::hideNicknameList()
{
    if (hideNicklistAction->isChecked())
    {
        Preferences::setShowNickList(false);
        Preferences::writeConfig();
        hideNicklistAction->setChecked(true);
    }
    else
    {
        Preferences::setShowNickList(true);
        Preferences::writeConfig();
        hideNicklistAction->setChecked(false);
    }

    emit updateChannelAppearance();
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
    if(e->type() == QEvent::WindowActivate)
    {
        emit endNotification();
    }

    return KMainWindow::event(e);
}

void KonversationMainWindow::serverQuit(Server* server)
{
    if(server == frontServer)
    {
        frontServer = 0;
    }

    if(m_frontView && m_frontView->getServer() == server)
    {
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
    if(s)
    {
        viewContainer->cornerWidget()->show();
    }
    else
    {
        viewContainer->cornerWidget()->hide();
    }
}

void KonversationMainWindow::insertCharacter()
{
    if(!m_insertCharDialog)
    {
        ChatWindow* view = static_cast<ChatWindow*>(viewContainer->currentPage());
        QFont font;

        if(view && view->getTextView())
        {
            font = view->getTextView()->font();
        }
        else if(view)
        {
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

    if(view)
    {
        view->appendInputText(chr);
    }
}

void KonversationMainWindow::openIdentitiesDialog()
{
    Konversation::IdentityDialog dlg(this);

    if((dlg.exec() == KDialog::Accepted) && m_serverListDialog)
    {
        m_serverListDialog->updateServerList();
    }
}

void KonversationMainWindow::updateChannelInfo(const QString &info)
{
    QString tabInfo = Konversation::removeIrcMarkup(info);
    m_channelInfoLabel->setText(tabInfo);
}

void KonversationMainWindow::showJoinChannelDialog()
{
    if(!frontServer)
    {
        return;
    }

    Konversation::JoinChannelDialog dlg(frontServer, this);

    if(dlg.exec() == QDialog::Accepted)
    {
        frontServer->sendJoinCommand(dlg.channel(), dlg.password());
    }
}

void KonversationMainWindow::reconnectCurrentServer()
{
    if(frontServer && !frontServer->isConnected() && !frontServer->isConnecting())
    {
        frontServer->reconnect();
    }
}

void KonversationMainWindow::openURL(const QString& url, const QString& /*title*/)
{
    QString urlN = url;
    urlN.remove("irc://");

    QString host = urlN.section('/',0,0);
    KonviIpHelper hostParser(host);
    host = hostParser.host();

    QString port = (hostParser.port().isEmpty() ? QString("6667") : hostParser.port());

    QString channel = urlN.section('/',1,1);
    QString password;

    if (Preferences::isServerGroup(host))
    {
        Server* newServer = KonversationApplication::instance()->connectToServerGroup(host);

        if (!newServer->isConnected())
        {
            newServer->setAutoJoin(true);
            newServer->setAutoJoinChannel(channel);
            newServer->setAutoJoinChannelKey(password);
        }
        else if(!channel.isEmpty())
        {
            newServer->queue("JOIN " + channel + " " + password);
        }
    }
    else
    {
        KonversationApplication::instance()->quickConnectToServer(host,port,channel,"",password);
    }
}

QString KonversationMainWindow::currentURL(bool passNetwork)
{
    QString url = QString::null;
    QString channel = QString::null;
    QString port = QString::null;
    QString server = QString::null;

    if(frontServer && m_frontView)
    {
        updateFrontView();

        if(m_frontView->getType() == ChatWindow::Channel)
        {
            channel = m_frontView->getName();
        }

        if (passNetwork)
        {
            server = frontServer->getServerGroup();
        }
        else
        {
            server = frontServer->getServerName();

            port = ":"+QString::number(frontServer->getPort());
        }

        if (server.contains(':')) // IPv6
        {
            server = "["+server+"]";
        }

        url = "irc://"+server+port+"/"+channel;
    }

    return url;
}

QString KonversationMainWindow::currentTitle()
{
    if (frontServer)
    {
        if(m_frontView && m_frontView->getType() == ChatWindow::Channel)
        {
            return m_frontView->getName();
        }
        else
        {
            QString serverGroup = frontServer->getServerGroup();
            if (!serverGroup.isEmpty())
            {
                return serverGroup;
            }
            else
            {
                return frontServer->getServerName();
            }
        }
    }
    else
    {
        return QString::null;
    }
}

void KonversationMainWindow::serverStateChanged(Server* server, Server::State state)
{
    KAction* action = actionCollection()->action("reconnect_server");

    if(action && (frontServer == server))
    {

        if(state != Server::SSDisconnected)
        {
            action->setEnabled(false);
        }
        else
        {
            action->setEnabled(true);
        }
    }
}

void KonversationMainWindow::showTabContextMenu(QWidget* tab, const QPoint& pos)
{
    m_popupTabIndex = getViewContainer()->indexOf(tab);
    updateTabMoveActions(m_popupTabIndex);
    QPopupMenu* menu = static_cast<QPopupMenu*>(factory()->container("tabContextMenu", this));

    if(!menu)
    {
        return;
    }

    ChatWindow* view = static_cast<ChatWindow*>(tab);

    if(view)
    {
        KToggleAction* notifyAction = static_cast<KToggleAction*>(actionCollection()->action("tab_notifications"));
        if(notifyAction)
        {
            ChatWindow::WindowType viewType = view->getType();
            notifyAction->setEnabled(viewType == ChatWindow::Channel || viewType == ChatWindow::Query || viewType == ChatWindow::Status);
            notifyAction->setChecked(view->notificationsEnabled());
        }

        updateTabEncoding(view);
    }

    if(menu->exec(pos) == -1)
    {
        m_popupTabIndex = -1;
        view = static_cast<ChatWindow*>(getViewContainer()->currentPage());

        if(view)
        {
            KToggleAction* notifyAction = static_cast<KToggleAction*>(actionCollection()->action("tab_notifications"));
            if(notifyAction)
            {
                ChatWindow::WindowType viewType = view->getType();
                notifyAction->setEnabled(viewType == ChatWindow::Channel || viewType == ChatWindow::Query || viewType == ChatWindow::Status);
                notifyAction->setChecked(view->notificationsEnabled());
            }

            updateTabEncoding(view);
        }
    }

    updateTabMoveActions(getViewContainer()->currentPageIndex());
}

void KonversationMainWindow::moveTabLeft()
{
    int index;

    if(m_popupTabIndex == -1)
    {
        index = getViewContainer()->currentPageIndex();
    }
    else
    {
        index = m_popupTabIndex;
    }

    if(index)
    {
        getViewContainer()->moveTab(index, index - 1);
        updateTabMoveActions(index - 1);
    }

    updateSwitchTabAction();
    m_popupTabIndex = -1;
}

void KonversationMainWindow::moveTabRight()
{
    int index;

    if(m_popupTabIndex == -1)
    {
        index = getViewContainer()->currentPageIndex();
    }
    else
    {
        index = m_popupTabIndex;
    }

    if(index < (getViewContainer()->count() - 1))
    {
        getViewContainer()->moveTab(index, index + 1);
        updateTabMoveActions(index + 1);
    }

    updateSwitchTabAction();
    m_popupTabIndex = -1;
}

void KonversationMainWindow::updateTabMoveActions(int index)
{
    KAction* action;

    if(getViewContainer()->count() > 0)
    {
        action = actionCollection()->action("move_tab_left");

        if(action)
        {
            action->setEnabled(index > 0);
        }

        action = actionCollection()->action("move_tab_right");

        if(action)
        {
            action->setEnabled(index < (getViewContainer()->count() - 1));
        }

        action = actionCollection()->action("tab_notifications");

        if(action)
        {
            action->setEnabled(true);
        }

        action = actionCollection()->action("next_tab");

        if(action)
        {
            action->setEnabled(true);
        }

        action = actionCollection()->action("previous_tab");

        if(action)
        {
            action->setEnabled(true);
        }

        action = actionCollection()->action("close_tab");

        if(action)
        {
            action->setEnabled(true);
        }
    }
    else
    {
        action = actionCollection()->action("move_tab_left");

        if(action)
        {
            action->setEnabled(false);
        }

        action = actionCollection()->action("move_tab_right");

        if(action)
        {
            action->setEnabled(false);
        }

        action = actionCollection()->action("tab_notifications");

        if(action)
        {
            action->setEnabled(false);
        }

        action = actionCollection()->action("next_tab");

        if(action)
        {
            action->setEnabled(false);
        }

        action = actionCollection()->action("previous_tab");

        if(action)
        {
            action->setEnabled(false);
        }

        action = actionCollection()->action("close_tab");

        if(action)
        {
            action->setEnabled(false);
        }

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

void KonversationMainWindow::toggleTabNotifications()
{
    ChatWindow* chatWin;

    if(m_popupTabIndex == -1)
    {
        chatWin = static_cast<ChatWindow*>(getViewContainer()->currentPage());
    }
    else
    {
        chatWin = static_cast<ChatWindow*>(getViewContainer()->page(m_popupTabIndex));
    }

    if(chatWin)
    {
        chatWin->setNotificationsEnabled(!chatWin->notificationsEnabled());
        KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("tab_notifications"));
        if(action) action->setChecked(static_cast<ChatWindow*>(getViewContainer()->currentPage())->notificationsEnabled());
    }

    m_popupTabIndex = -1;
}

void KonversationMainWindow::changeTabCharset(int index)
{
    ChatWindow* chatWin;

    if(m_popupTabIndex == -1)
    {
        chatWin = static_cast<ChatWindow*>(getViewContainer()->currentPage());
    }
    else
    {
        chatWin = static_cast<ChatWindow*>(getViewContainer()->page(m_popupTabIndex));
    }

    if(chatWin)
    {
        if(index == 0)
        {
            chatWin->setChannelEncoding(QString::null);
        }
        else
        {
            chatWin->setChannelEncoding(Konversation::IRCCharsets::self()->availableEncodingShortNames()[index - 1]);
        }
    }

    m_popupTabIndex = -1;
}

void KonversationMainWindow::updateSwitchTabAction()
{
    QStringList tabList;

    for(int i = 0; i < getViewContainer()->count(); ++i)
    {
        tabList << static_cast<ChatWindow*>(getViewContainer()->page(i))->getName();
    }

    KSelectAction* action = static_cast<KSelectAction*>(actionCollection()->action("switch_to_tab"));

    if(action)
    {
        action->setItems(tabList);
        action->setCurrentItem(getViewContainer()->currentPageIndex());
    }
}

void KonversationMainWindow::updateTabEncoding(ChatWindow* view)
{
    if(view)
    {
        ChatWindow::WindowType viewType = view->getType();
        KSelectAction* codecAction = static_cast<KSelectAction*>(actionCollection()->action("tab_encoding"));

        if(codecAction)
        {
            if(viewType == ChatWindow::Channel || viewType == ChatWindow::Query || viewType == ChatWindow::Status)
            {
                codecAction->setEnabled(view->isChannelEncodingSupported());
                QString encoding = view->getChannelEncoding();

                if(frontServer)
                {
                    codecAction->changeItem(0, i18n("Default encoding", "Default ( %1 )").arg(frontServer->getIdentity()->getCodecName()));
                }

                if(encoding.isEmpty())
                {
                    codecAction->setCurrentItem(0);
                }
                else
                {
                    codecAction->setCurrentItem(Konversation::IRCCharsets::self()->shortNameToIndex(encoding) + 1);
                }
            }
            else
            {
                codecAction->setEnabled(false);
            }
        }
    }
}

#include "konversationmainwindow.moc"
