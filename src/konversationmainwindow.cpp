/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005-2007 Eike Hein <hein@kde.org>
*/

#include "konversationmainwindow.h"
#include "konversationapplication.h"
#include "linkaddressbook/addressbook.h"
#include "konvisettingsdialog.h"
#include "viewcontainer.h"
#include "konversationstatusbar.h"
#include "konvibookmarkhandler.h"
#include "trayicon.h"
#include "serverlistdialog.h"
#include "identitydialog.h"
#include "notificationhandler.h"
#include "irccharsets.h"
#include "konviiphelper.h"

#include <qnamespace.h>
#include <qwhatsthis.h>
#include <qsignalmapper.h>
#include <qobjectlist.h>

#include <kaccel.h>
#include <kaccelmanager.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenubar.h>
#include <kkeydialog.h>
#include <kdeversion.h>
#include <kedittoolbar.h>
#include <kpopupmenu.h>
#include <kwin.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <dcopclient.h>
#include <scriptmanager.h>
#include <kabc/addressbook.h>
#include <kabc/errorhandler.h>

#include <config.h>
#ifdef  USE_KNOTIFY
#include <knotifydialog.h>
#endif


KonversationMainWindow::KonversationMainWindow() : KMainWindow(0,"main_window", WStyle_ContextHelp | WType_TopLevel | WDestructiveClose)
{
    m_hasDirtySettings = false;
    m_closeApp = false;
    m_serverListDialog = 0;
    m_settingsDialog = NULL;

    m_viewContainer = new ViewContainer(this);
    setCentralWidget(m_viewContainer->getWidget());

    //used for event compression. See header file for resetHasDirtySettings()
    connect(KonversationApplication::instance(), SIGNAL(appearanceChanged()), this, SLOT(resetHasDirtySettings()));
    connect(KonversationApplication::instance(), SIGNAL(appearanceChanged()), this, SLOT(updateTrayIcon()));


    // Set up view container
    connect(KonversationApplication::instance(), SIGNAL(appearanceChanged()), m_viewContainer, SLOT(updateAppearance()));
    connect(KonversationApplication::instance(), SIGNAL(iconChanged(int)), m_viewContainer, SLOT(updateViewIcons()));
    connect(KonversationApplication::instance(), SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)),
            m_viewContainer, SLOT(updateViews(const Konversation::ServerGroupSettings*)));
    connect(m_viewContainer, SIGNAL(autoJoinToggled(const Konversation::ServerGroupSettings*)),
            KonversationApplication::instance(), SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)));
    connect(m_viewContainer, SIGNAL(setWindowCaption(const QString&)), this, SLOT(setCaption(const QString&)));
    connect(this, SIGNAL(serverStateChanged(Server*, Server::State)), m_viewContainer, SLOT(serverStateChanged(Server*, Server::State)));
    connect(this, SIGNAL(triggerRememberLine()), m_viewContainer, SLOT(insertRememberLine()));
    connect(this, SIGNAL(triggerRememberLines(Server*)), m_viewContainer, SLOT(insertRememberLines(Server*)));
    connect(this, SIGNAL(cancelRememberLine()), m_viewContainer, SLOT(cancelRememberLine()));
    connect(this, SIGNAL(insertMarkerLine()), m_viewContainer, SLOT(insertMarkerLine()));

    // Set up status bar
    m_statusBar = new KonversationStatusBar(this);
    connect(KonversationApplication::instance(), SIGNAL(appearanceChanged()), m_statusBar, SLOT(updateAppearance()));

    createStandardStatusBarAction();

    connect(actionCollection(), SIGNAL(actionStatusText(const QString&)), m_statusBar, SLOT(setMainLabelTempText(const QString&)));
    connect(actionCollection(), SIGNAL(clearStatusText()), m_statusBar, SLOT(clearMainLabelTempText()));
    actionCollection()->setHighlightingEnabled(true);

    connect(m_viewContainer, SIGNAL(resetStatusBar()), m_statusBar, SLOT(resetStatusBar()));
    connect(m_viewContainer, SIGNAL(setStatusBarTempText(const QString&)), m_statusBar, SLOT(setMainLabelTempText(const QString&)));
    connect(m_viewContainer, SIGNAL(clearStatusBarTempText()), m_statusBar, SLOT(clearMainLabelTempText()));
    connect(m_viewContainer, SIGNAL(setStatusBarInfoLabel(const QString&)), m_statusBar, SLOT(updateInfoLabel(const QString&)));
    connect(m_viewContainer, SIGNAL(clearStatusBarInfoLabel()), m_statusBar, SLOT(clearInfoLabel()));
    connect(m_viewContainer, SIGNAL(setStatusBarLagLabelShown(bool)), m_statusBar, SLOT(setLagLabelShown(bool)));
    connect(m_viewContainer, SIGNAL(updateStatusBarLagLabel(Server*, int)), m_statusBar, SLOT(updateLagLabel(Server*, int)));
    connect(m_viewContainer, SIGNAL(resetStatusBarLagLabel()), m_statusBar, SLOT(resetLagLabel()));
    connect(m_viewContainer, SIGNAL(setStatusBarLagLabelTooLongLag(Server*, int)), m_statusBar, SLOT(setTooLongLag(Server*, int)));
    connect(m_viewContainer, SIGNAL(updateStatusBarSSLLabel(Server*)), m_statusBar, SLOT(updateSSLLabel(Server*)));
    connect(m_viewContainer, SIGNAL(removeStatusBarSSLLabel()), m_statusBar, SLOT(removeSSLLabel()));


    // Actions
    KStdAction::quit(this,SLOT(quitProgram()),actionCollection());

    hideMenuBarAction = KStdAction::showMenubar(this, SLOT(toggleMenubar()), actionCollection());

    setStandardToolBarMenuEnabled(true);
    KStdAction::configureToolbars(this, SLOT(configureToolbar()), actionCollection());

    KStdAction::keyBindings(this, SLOT(openKeyBindings()), actionCollection());
    KAction *preferencesAction = KStdAction::preferences(this, SLOT(openPrefsDialog()), actionCollection());

#ifdef USE_KNOTIFY // options_configure_notifications
    KAction *configureNotificationsAction = KStdAction::configureNotifications(this,SLOT(openNotifications()), actionCollection());
#endif

    KAction* action;

    (new KAction(i18n("&Server List..."), "server", KShortcut("F2"), this, SLOT(openServerList()),
        actionCollection(), "open_server_list"))->setToolTip(i18n("Manage networks and servers"));
    (new KAction(i18n("Quick &Connect..."), "connect_creating", KShortcut("F7"), this, SLOT(openQuickConnectDialog()),
        actionCollection(), "quick_connect_dialog"))->setToolTip(i18n("Type in the address of a new IRC server to connect to"));

    action = new KAction(i18n("&Reconnect"), "connect_creating", 0, m_viewContainer, SLOT(reconnectFrontServer()), actionCollection(), "reconnect_server");
    action->setEnabled(false);
    action->setToolTip(i18n("Reconnect to the current server."));

    action = new KAction(i18n("&Disconnect"), "connect_no", 0, m_viewContainer, SLOT(disconnectFrontServer()), actionCollection(), "disconnect_server");
    action->setEnabled(false);
    action->setToolTip(i18n("Disconnect from the current server."));

    (new KAction(i18n("&Identities..."), "identity", KShortcut("F8"), this, SLOT(openIdentitiesDialog()),
        actionCollection(), "identities_dialog"))->setToolTip(i18n("Manage your nick, away and other identity settings"));

    new KToggleAction(i18n("&Watched Nicks Online"), "kontact_contacts", KShortcut("F4"), m_viewContainer, SLOT(openNicksOnlinePanel()), actionCollection(), "open_nicksonline_window");
    new KToggleAction(i18n("&DCC Status"), "2rightarrow", KShortcut("F9"), m_viewContainer, SLOT(toggleDccPanel()), actionCollection(), "open_dccstatus_window");
    action = new KAction(i18n("&Open Logfile"), "history", KShortcut("Ctrl+O"), m_viewContainer, SLOT(openLogFile()), actionCollection(), "open_logfile");
    action->setEnabled(false);
    action->setToolTip(i18n("Open the known history for this channel in a new tab"));

    action = new KAction(i18n("&Channel Settings..."), "edit", m_viewContainer, SLOT(openChannelSettings()), actionCollection(), "channel_settings");
    action->setEnabled(false);
    action->setToolTip(i18n("Open the channel settings dialog for this tab"));

    KToggleAction* channelListAction = new KToggleAction(i18n("Channel &List"), "view_text", KShortcut("F5"), m_viewContainer, SLOT(openChannelList()), actionCollection(), "open_channel_list");
    channelListAction->setEnabled(false);
    channelListAction->setToolTip(i18n("Show a list of all the known channels on this server"));

    action = new KToggleAction(i18n("&URL Catcher"), "enhanced_browsing", KShortcut("F6"), m_viewContainer, SLOT(addUrlCatcher()), actionCollection(), "open_url_catcher");
    action->setToolTip(i18n("List all URLs that have been mentioned recently in a new tab"));

    if (kapp->authorize("shell_access"))
    {
        action = new KAction(i18n("New &Konsole"), "openterm", 0, m_viewContainer, SLOT(addKonsolePanel()), actionCollection(), "open_konsole");
        action->setToolTip(i18n("Open a terminal in a new tab"));
    }

    // Actions to navigate through the different pages
    KShortcut nextShortcut = KStdAccel::tabNext();
    nextShortcut.setSeq(1, KKeySequence("Alt+Right"));
    KShortcut prevShortcut = KStdAccel::tabPrev();
    prevShortcut.setSeq(1, KKeySequence("Alt+Left"));
    action = new KAction(i18n("&Next Tab"), QApplication::reverseLayout() ? "previous" : "next",
        QApplication::reverseLayout() ? prevShortcut : nextShortcut,
        m_viewContainer, SLOT(showNextView()), actionCollection(), "next_tab");
    action->setEnabled(false);
    action = new KAction(i18n("&Previous Tab"), QApplication::reverseLayout() ? "next" : "previous",
        QApplication::reverseLayout() ? nextShortcut : prevShortcut,
        m_viewContainer, SLOT(showPreviousView()),actionCollection(),"previous_tab");
    action->setEnabled(false);
    action = new KAction(i18n("Close &Tab"),"tab_remove",KShortcut("Ctrl+w"), m_viewContainer, SLOT(closeCurrentView()),actionCollection(),"close_tab");
    action->setEnabled(false);
    action = new KAction(i18n("Next Active Tab"), 0, KShortcut("Ctrl+Alt+Space"), m_viewContainer, SLOT(showNextActiveView()),
                         actionCollection(), "next_active_tab");
    action->setEnabled(false);

    if (Preferences::tabPlacement()==Preferences::Left)
    {
        action = new KAction(i18n("Move Tab Up"), "1uparrow", KShortcut("Alt+Shift+Left"),
            m_viewContainer, SLOT(moveViewLeft()), actionCollection(), "move_tab_left");
        action->setEnabled(false);
        action->setToolTip("Move this tab");
        action = new KAction(i18n("Move Tab Down"), "1downarrow", KShortcut("Alt+Shift+Right"),
            m_viewContainer, SLOT(moveViewRight()), actionCollection(), "move_tab_right");
        action->setEnabled(false);
        action->setToolTip("Move this tab");
    }
    else
    {
        KAction* action2;
        if (QApplication::reverseLayout())
        {
            action2 = new KAction(i18n("Move Tab Right"), "1rightarrow", KShortcut("Alt+Shift+Right"),
                          m_viewContainer, SLOT(moveViewLeft()), actionCollection(), "move_tab_left");
            action  = new KAction(i18n("Move Tab Left"), "1leftarrow", KShortcut("Alt+Shift+Left"),
                          m_viewContainer, SLOT(moveViewRight()), actionCollection(), "move_tab_right");
        }
        else
        {
            action  = new KAction(i18n("Move Tab Left"), "1leftarrow", KShortcut("Alt+Shift+Left"),
                          m_viewContainer, SLOT(moveViewLeft()), actionCollection(), "move_tab_left");
                action2 = new KAction(i18n("Move Tab Right"), "1rightarrow", KShortcut("Alt+Shift+Right"),
                          m_viewContainer, SLOT(moveViewRight()), actionCollection(), "move_tab_right");
        }

        action->setEnabled(false);
        action->setToolTip("Move this tab");
        action2->setEnabled(false);
        action2->setToolTip("Move this tab");
    }

    action = new KToggleAction(i18n("Enable Notifications"), 0, 0, m_viewContainer, SLOT(toggleViewNotifications()), actionCollection(), "tab_notifications");
    action->setEnabled(false);

    action = new KToggleAction(i18n("Join on Connect"), 0, 0, m_viewContainer, SLOT(toggleAutoJoin()), actionCollection(), "tab_autojoin");
    action->setEnabled(false);

    KSelectAction* selectAction = new KSelectAction(i18n("Set Encoding"), "charset", 0, actionCollection(), "tab_encoding");
    selectAction->setEditable(false);
    QStringList encodingDescs = Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames();
    encodingDescs.prepend(i18n("Default"));
    selectAction->setItems(encodingDescs);
    selectAction->setEnabled(false);
    connect(selectAction, SIGNAL(activated(int)), m_viewContainer, SLOT(changeViewCharset(int)));

    QSignalMapper* tabSelectionMapper = new QSignalMapper(this);
    connect(tabSelectionMapper, SIGNAL(mapped(int)), m_viewContainer, SLOT(goToView(int)));

    for (uint i = 1; i <= 10; ++i)
    {
        KAction* tabSelectionAction = new KAction(i18n("Go to Tab %1").arg(i), 0, KShortcut(QString("Alt+%1").arg(i%10)),
            tabSelectionMapper, SLOT(map()), actionCollection(), QString("go_to_tab_%1").arg(i).local8Bit());
        tabSelectionMapper->setMapping( tabSelectionAction, i-1);
    }

    action = new KAction(i18n("Clear &Marker Lines"), 0, KShortcut("CTRL+SHIFT+R"), m_viewContainer, SLOT(clearViewLines()),actionCollection(),"clear_lines");
    action->setToolTip(i18n("Clear marker lines in the current tab"));
    action->setEnabled(false);
    action = new KAction(i18n("&Clear Window"), 0, KShortcut("Ctrl+L"), m_viewContainer, SLOT(clearView()),actionCollection(),"clear_window");
    action->setToolTip(i18n("Clear the contents of the current tab"));
    action->setEnabled(false);
    action = new KAction(i18n("Clear &All Windows"),0,KShortcut("CTRL+SHIFT+L"), m_viewContainer, SLOT(clearAllViews()),actionCollection(),"clear_tabs");
    action->setToolTip(i18n("Clear the contents of all open tabs"));
    action->setEnabled(false);

    KAction* awayAction = new KAction(i18n("Set &Away Globally")/*, "konversationaway"*/, KShortcut("Ctrl+Shift+A"),
        static_cast<KonversationApplication *>(kapp), SLOT(toggleAway()), actionCollection(),"toggle_away");
    awayAction->setEnabled(false);

    action = new KAction(i18n("&Join Channel..."), "add", KShortcut("Ctrl+J"), m_viewContainer, SLOT(showJoinChannelDialog()), actionCollection(), "join_channel");
    action->setEnabled(false);
    action->setToolTip("Join a new channel on this server");

    action = KStdAction::find(m_viewContainer, SLOT(findText()), actionCollection());
    action->setEnabled(false);
    action = KStdAction::findNext(m_viewContainer, SLOT(findNextText()), actionCollection());
    action->setEnabled(false);
    action = KStdAction::findPrev(m_viewContainer, SLOT(findPrevText()), actionCollection());
    action->setEnabled(false);

    action = new KAction(i18n("&IRC Color..."), "colorize", CTRL+Key_K, m_viewContainer, SLOT(insertIRCColor()), actionCollection(), "irc_colors");
    action->setToolTip(i18n("Set the color of your current IRC message"));
    action->setEnabled(false);
    action = new KAction(i18n("&Marker Line"), 0,  KShortcut("Ctrl+R") , m_viewContainer, SLOT(insertMarkerLine()), actionCollection(), "insert_marker_line");
    action->setToolTip(i18n("Insert a horizontal line into the current tab that only you can see"));
    action->setEnabled(false);
    action = new KAction(i18n("Special &Character..."), "char", KShortcut("Alt+Shift+C"), m_viewContainer, SLOT(insertCharacter()), actionCollection(), "insert_character");
    action->setToolTip(i18n("Insert any character into your current IRC message"));
    action->setEnabled(false);

    action = new KAction(i18n("Close &All Open Queries"), 0, KShortcut("F11"), m_viewContainer, SLOT(closeQueries()), actionCollection(), "close_queries");
    action->setEnabled(false);

    KToggleAction* toggleChannelNickListsAction = new KToggleAction(i18n("Hide Nicklist"), 0,
        KShortcut("Ctrl+H"), m_viewContainer, SLOT(toggleChannelNicklists()), actionCollection(), "hide_nicknamelist");
    if (!Preferences::showNickList())
        toggleChannelNickListsAction->setChecked(true);

    // set up system tray
    m_trayIcon = new Konversation::TrayIcon(this);
    connect(this, SIGNAL(endNotification()), m_trayIcon, SLOT(endNotification()));
    connect(KonversationApplication::instance(), SIGNAL(iconChanged(int)), m_trayIcon, SLOT(updateAppearance()));
    connect(m_trayIcon, SIGNAL(quitSelected()), this, SLOT(quitProgram()));
    KPopupMenu *trayMenu = m_trayIcon->contextMenu();
    #ifdef USE_KNOTIFY
    configureNotificationsAction->plug(trayMenu);
    #endif
    preferencesAction->plug(trayMenu);
    awayAction->plug(trayMenu);

    // decide whether to show the tray icon or not
    updateTrayIcon();

    createGUI(NULL, false);

    resize(700, 500);                             // Give the app a sane default size
    setAutoSaveSettings();

    // Apply menubar show/hide pref
    hideMenuBarAction->setChecked(Preferences::showMenuBar());
    toggleMenubar(true);

    // Bookmarks
    m_bookmarkHandler = new KonviBookmarkHandler(this);
    connect(m_bookmarkHandler,SIGNAL(openURL(const QString&,const QString&)),this,SLOT(openURL(const QString&,const QString&)));

    // set up KABC with a nice gui error dialog
    KABC::GuiErrorHandler *m_guiErrorHandler = new KABC::GuiErrorHandler(this);
    kapp->dcopClient()->setAcceptCalls( false );
    Konversation::Addressbook::self()->getAddressBook()->setErrorHandler(m_guiErrorHandler);
    kapp->dcopClient()->setAcceptCalls( true );

    if (Preferences::useNotify() && Preferences::openWatchedNicksAtStartup())
        m_viewContainer->openNicksOnlinePanel();

}

KonversationMainWindow::~KonversationMainWindow()
{
    Preferences::writeConfig();
}

int KonversationMainWindow::confirmQuit()
{
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(kapp);

    QPtrList<Server> serverList = konvApp->getServerList();
    if (serverList.isEmpty()) return KMessageBox::Continue;

    int result=KMessageBox::warningContinueCancel(
        this,
        i18n("<qt>Are you sure you want to quit <b>Konversation</b>?</qt>"),
        i18n("Confirm Quit"),
        i18n("Quit"),
        "systemtrayquitKonversation");
    return result;
}

void KonversationMainWindow::quitProgram()
{
    if (Preferences::showTrayIcon() &&
        sender() != m_trayIcon &&
        confirmQuit() == KMessageBox::Cancel) return;

    // will call queryClose()
    m_closeApp = true;
    close();
}

bool KonversationMainWindow::queryClose()
{
    KonversationApplication* konv_app = static_cast<KonversationApplication*>(kapp);

    if (!konv_app->sessionSaving())
    {
        if (sender() == m_trayIcon)
            m_closeApp = true;

        if (Preferences::showTrayIcon() && !m_closeApp)
        {
            KMessageBox::information( this,
                i18n("<p>Closing the main window will keep Konversation running in the system tray. "
                "Use <b>Quit</b> from the <b>Konversation</b> menu to quit the application.</p>"),
                i18n( "Docking in System Tray" ),  "HideOnCloseInfo" );
            hide();

            return false;
        }

        if (!Preferences::showTrayIcon() && confirmQuit() == KMessageBox::Cancel)
            return false;
    }

    m_viewContainer->prepareShutdown();

    // send quit to all servers
    emit quitServer();

    if(isHidden() && Preferences::showTrayIcon())
    {
        Preferences::setHiddenToTray(true);
    }
    else
    {
        Preferences::setHiddenToTray(false);
    }

    return true;
}

void KonversationMainWindow::hideEvent(QHideEvent *e)
{
    emit triggerRememberLine();

    m_statusBar->clearMainLabelTempText();

    KMainWindow::hideEvent(e);
}

void KonversationMainWindow::showEvent(QShowEvent *e)
{
    emit cancelRememberLine();

    KMainWindow::showEvent(e);
}

void KonversationMainWindow::leaveEvent(QEvent* e)
{
    m_statusBar->clearMainLabelTempText();

    KMainWindow::leaveEvent(e);
}

bool KonversationMainWindow::event(QEvent* e)
{
    if (e->type() == QEvent::WindowActivate)
    {
        emit endNotification();
        emit cancelRememberLine();
    }
    else if(e->type() == QEvent::WindowDeactivate)
    {
        m_statusBar->clearMainLabelTempText();

        if (kapp->activeModalWidget() == 0)
            emit triggerRememberLine();
    }

    return KMainWindow::event(e);
}

void KonversationMainWindow::settingsChangedSlot()
{
    // This is for compressing the events. m_hasDirtySettings is set to true
    // when the settings have changed, then set to false when the app reacts to it
    // via the appearanceChanged signal.  This prevents a series of settingsChanged signals
    // causing the app expensively rereading its settings many times.
    // The appearanceChanged signal is connected to resetHasDirtySettings to reset this bool
    if (!m_hasDirtySettings) 
    {
        QTimer::singleShot(0, KonversationApplication::instance(), SIGNAL(appearanceChanged()));
        m_hasDirtySettings = true;
    }
}

void KonversationMainWindow::resetHasDirtySettings()
{
    m_hasDirtySettings = false;
}

void KonversationMainWindow::updateTrayIcon()
{
    m_trayIcon->setNotificationEnabled(Preferences::trayNotify());

    if (Preferences::showTrayIcon())
        m_trayIcon->show();
    else
        m_trayIcon->hide();
}

void KonversationMainWindow::toggleMenubar(bool dontShowWarning)
{
    if (hideMenuBarAction->isChecked())
        menuBar()->show();
    else
    {
        if (!dontShowWarning)
        {
            QString accel = hideMenuBarAction->shortcut().toString();
            KMessageBox::information(this,
                i18n("<qt>This will hide the menu bar completely. You can show it again by typing %1.</qt>").arg(accel),
                "Hide menu bar","HideMenuBarWarning");
        }
        menuBar()->hide();
    }

    Preferences::setShowMenuBar(hideMenuBarAction->isChecked());
}

int KonversationMainWindow::configureToolbar()
{
    saveMainWindowSettings(KGlobal::config());
    KEditToolbar dlg(actionCollection(), xmlFile(), true, this);
    connect(&dlg, SIGNAL(newToolbarConfig()), SLOT(saveToolbarConfig()));
    return dlg.exec();
}

void KonversationMainWindow::saveToolbarConfig()
{
    createGUI(xmlFile(), false);
    applyMainWindowSettings(KGlobal::config());
}

void KonversationMainWindow::focusAndShowErrorMessage(const QString &errorMsg)
{
    show();
    KWin::demandAttention(winId());
    KWin::activateWindow(winId());
    KMessageBox::error(this, errorMsg);
}

void KonversationMainWindow::openPrefsDialog()
{
    //An instance of your dialog could be already created and could be cached,
    //in which case you want to display the cached dialog instead of creating
    //another one
    if (!m_settingsDialog)
    {
        m_settingsDialog = new KonviSettingsDialog(this);
        //User edited the configuration - update your local copies of the
        //configuration data
        connect(m_settingsDialog, SIGNAL(settingsChanged()), this, SLOT(settingsChangedSlot()));
    }
    m_settingsDialog->show();
}

void KonversationMainWindow::openKeyBindings()
{
    // Change a number of action names to make them friendlier for the shortcut list.
    actionCollection()->action("tab_notifications")->setText(i18n("Toggle Notifications"));
    actionCollection()->action("toggle_away")->setText(i18n("Toggle Away Globally"));
    actionCollection()->action("irc_colors")->setText(i18n("Insert &IRC Color..."));
    actionCollection()->action("insert_character")->setText(i18n("Insert Special &Character..."));
    actionCollection()->action("insert_marker_line")->setText(i18n("Insert &Marker Line"));
    QString openChannelListString = actionCollection()->action("open_channel_list")->text();
    actionCollection()->action("open_channel_list")->setText(i18n("&Channel List"));
    QString openLogFileString = actionCollection()->action("open_logfile")->text();
    actionCollection()->action("open_logfile")->setText(i18n("&Open Logfile"));

    // Open shortcut configuration dialog.
    KKeyDialog::configure(actionCollection());

    // Reset action names.
    actionCollection()->action("tab_notifications")->setText(i18n("Enable Notifications"));
    actionCollection()->action("toggle_away")->setText(i18n("Set &Away Globally"));
    actionCollection()->action("irc_colors")->setText(i18n("&IRC Color..."));
    actionCollection()->action("insert_character")->setText(i18n("Special &Character..."));
    actionCollection()->action("insert_marker_line")->setText(i18n("&Marker Line"));
    actionCollection()->action("open_channel_list")->setText(openChannelListString);
    actionCollection()->action("open_logfile")->setText(openLogFileString);
}

void KonversationMainWindow::openServerList()
{
    if (!m_serverListDialog)
    {
        m_serverListDialog = new Konversation::ServerListDialog(this);
        KonversationApplication *konvApp = static_cast<KonversationApplication *>(KApplication::kApplication());

        connect(m_serverListDialog, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)),
                konvApp, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)));
        connect(konvApp, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettings*)),
                m_serverListDialog, SLOT(updateServerList()));
        connect(m_serverListDialog, SIGNAL(connectToServer(int)), konvApp, SLOT(connectToServer(int)));
        connect(m_serverListDialog, SIGNAL(connectToServer(int, const QString&, Konversation::ServerSettings)),
                konvApp, SLOT(connectToServer(int, const QString&, Konversation::ServerSettings)));
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
  if (m_settingsDialog) m_settingsDialog->openWatchedNicknamesPage();
}

void KonversationMainWindow::openIdentitiesDialog()
{
    Konversation::IdentityDialog dlg(this);

    if((dlg.exec() == KDialog::Accepted) && m_serverListDialog)
        m_serverListDialog->updateServerList();
}

IdentityPtr KonversationMainWindow::editIdentity(IdentityPtr identity)
{
    IdentityPtr newIdentity;

    Konversation::IdentityDialog dlg(this);
    newIdentity = dlg.setCurrentIdentity(identity);

    if ((dlg.exec() == KDialog::Accepted) && m_serverListDialog)
    {
        m_serverListDialog->updateServerList();
        return newIdentity;
    }
    else
        return 0;
}

void KonversationMainWindow::openNotifications()
{
    #ifdef USE_KNOTIFY
    (void) KNotifyDialog::configure(this);
    #endif
}

void KonversationMainWindow::notifyAction(const QString& serverName,const QString& nick)
{
    KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
    Server* server=konv_app->getServerByName(serverName);
    server->notifyAction(nick);
}


// TODO: Let an own class handle notify things
void KonversationMainWindow::setOnlineList(Server* notifyServer,const QStringList& /*list*/, bool /*changed*/)
{
    emit nicksNowOnline(notifyServer);
    // FIXME  if (changed && nicksOnlinePanel) newText(nicksOnlinePanel, QString::null, true);
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

    if (Preferences::isServerGroup(host))
        KonversationApplication::instance()->connectToServer(Preferences::serverGroupIdByName(host), channel);
    else
        KonversationApplication::instance()->quickConnectToServer(host, port, channel, "");
}

QString KonversationMainWindow::currentURL(bool passNetwork)
{
    return m_viewContainer->currentViewURL(passNetwork);
}

QString KonversationMainWindow::currentTitle()
{
    return m_viewContainer->currentViewTitle();
}

#include "konversationmainwindow.moc"
