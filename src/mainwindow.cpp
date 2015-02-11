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
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#include "mainwindow.h"
#include "application.h"
#include "settingsdialog.h"
#include "viewcontainer.h"
#include "statusbar.h"
#include "bookmarkhandler.h"
#include "trayicon.h"
#include "serverlistdialog.h"
#include "identitydialog.h"
#include "notificationhandler.h"
#include "irccharsets.h"
#include "connectionmanager.h"
#include "awaymanager.h"
#include "transfermanager.h"

#include <QSignalMapper>
#include <QSplitter>
#include <QMenuBar>

#include <KActionCollection>
#include <QAction>
#include <KToggleAction>
#include <KSelectAction>
#include <KAuthorized>
#include <KStandardAction>
#include <KLocalizedString>
#include <KMessageBox>

#include <QIcon>
#include <QMenu>
#include <KWindowSystem>
#include <KShortcutsDialog>
#include <KStandardShortcut>
#include <KActionMenu>
#include <KNotifyConfigWidget>
#include <KGlobalAccel>
#include <KIconLoader>

MainWindow::MainWindow() : KXmlGuiWindow(0)
{
    m_hasDirtySettings = false;
    m_closeApp = false;
    m_serverListDialog = 0;
    m_trayIcon = 0;
    m_settingsDialog = NULL;

    m_viewContainer = new ViewContainer(this);
    setCentralWidget(m_viewContainer->getWidget());

    //used for event compression. See header file for resetHasDirtySettings()
    connect(Application::instance(), SIGNAL(appearanceChanged()), this, SLOT(resetHasDirtySettings()));
    connect(Application::instance(), SIGNAL(appearanceChanged()), this, SLOT(updateTrayIcon()));


    // Set up view container
    connect(Application::instance(), SIGNAL(appearanceChanged()), m_viewContainer, SLOT(updateAppearance()));
    connect(Application::instance(), SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)),
            m_viewContainer, SLOT(updateViews(Konversation::ServerGroupSettingsPtr)));
    connect(m_viewContainer, SIGNAL(autoJoinToggled(Konversation::ServerGroupSettingsPtr)),
            Application::instance(), SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)));
    connect(m_viewContainer, SIGNAL(autoConnectOnStartupToggled(Konversation::ServerGroupSettingsPtr)),
            Application::instance(), SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)));
    connect(m_viewContainer, SIGNAL(setWindowCaption(QString)), this, SLOT(setCaption(QString)));
    connect(Application::instance()->getConnectionManager(),
            SIGNAL(connectionChangedState(Server*,Konversation::ConnectionState)),
            m_viewContainer, SLOT(connectionStateChanged(Server*,Konversation::ConnectionState)));
    connect(this, SIGNAL(triggerRememberLine()), m_viewContainer, SLOT(insertRememberLine()));
    connect(this, SIGNAL(triggerRememberLines(Server*)), m_viewContainer, SLOT(insertRememberLines(Server*)));
    connect(this, SIGNAL(cancelRememberLine()), m_viewContainer, SLOT(cancelRememberLine()));
    connect(this, SIGNAL(insertMarkerLine()), m_viewContainer, SLOT(insertMarkerLine()));

    // Set up status bar
    m_statusBar = new Konversation::StatusBar(this);
    connect(Application::instance(), SIGNAL(appearanceChanged()), m_statusBar, SLOT(updateAppearance()));

    createStandardStatusBarAction();

    connect(m_viewContainer, SIGNAL(resetStatusBar()), m_statusBar, SLOT(resetStatusBar()));
    connect(m_viewContainer, SIGNAL(setStatusBarTempText(QString)), m_statusBar, SLOT(setMainLabelTempText(QString)));
    connect(m_viewContainer, SIGNAL(clearStatusBarTempText()), m_statusBar, SLOT(clearMainLabelTempText()));
    connect(m_viewContainer, SIGNAL(setStatusBarInfoLabel(QString)), m_statusBar, SLOT(updateInfoLabel(QString)));
    connect(m_viewContainer, SIGNAL(clearStatusBarInfoLabel()), m_statusBar, SLOT(clearInfoLabel()));
    connect(m_viewContainer, SIGNAL(setStatusBarLagLabelShown(bool)), m_statusBar, SLOT(setLagLabelShown(bool)));
    connect(m_viewContainer, SIGNAL(updateStatusBarLagLabel(Server*,int)), m_statusBar, SLOT(updateLagLabel(Server*,int)));
    connect(m_viewContainer, SIGNAL(resetStatusBarLagLabel(Server*)), m_statusBar, SLOT(resetLagLabel(Server*)));
    connect(m_viewContainer, SIGNAL(setStatusBarLagLabelTooLongLag(Server*,int)), m_statusBar, SLOT(setTooLongLag(Server*,int)));
    connect(m_viewContainer, SIGNAL(updateStatusBarSSLLabel(Server*)), m_statusBar, SLOT(updateSSLLabel(Server*)));
    connect(m_viewContainer, SIGNAL(removeStatusBarSSLLabel()), m_statusBar, SLOT(removeSSLLabel()));


    // Actions
    KStandardAction::quit(this,SLOT(quitProgram()),actionCollection());

    m_showMenuBarAction = KStandardAction::showMenubar(this, SLOT(toggleMenubar()), actionCollection());

    setStandardToolBarMenuEnabled(true);
    KStandardAction::configureToolbars(this, SLOT(configureToolbars()), actionCollection());

    KStandardAction::keyBindings(this, SLOT(openKeyBindings()), actionCollection());
    KStandardAction::preferences(this, SLOT(openPrefsDialog()), actionCollection());

    KStandardAction::configureNotifications(this, SLOT(openNotifications()), actionCollection());

    QAction* action;

    action=new QAction(this);
    action->setText(i18n("Restart"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("system-reboot")));
    action->setStatusTip(i18n("Quit and restart the application"));
    connect(action, SIGNAL(triggered()), Application::instance(), SLOT(restart()));
    actionCollection()->addAction(QStringLiteral("restart"), action);

    action=new QAction(this);
    action->setText(i18n("&Server List..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("network-server")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("F2")));
    action->setStatusTip(i18n("Manage networks and servers"));
    connect(action, SIGNAL(triggered()), SLOT(openServerList()));
    actionCollection()->addAction(QStringLiteral("open_server_list"), action);

    action=new QAction(this);
    action->setText(i18n("Quick &Connect..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("network-connect")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("F7")));
    action->setStatusTip(i18n("Type in the address of a new IRC server to connect to"));
    connect(action, SIGNAL(triggered()), SLOT(openQuickConnectDialog()));
    actionCollection()->addAction(QStringLiteral("quick_connect_dialog"), action);

    action=new QAction(this);
    action->setText(i18n("&Reconnect"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Reconnect to the current server."));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(reconnectFrontServer()));
    actionCollection()->addAction(QStringLiteral("reconnect_server"), action);


    action=new QAction(this);
    action->setText(i18n("&Disconnect"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("network-disconnect")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Disconnect from the current server."));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(disconnectFrontServer()));
    actionCollection()->addAction(QStringLiteral("disconnect_server"), action);

    action=new QAction(this);
    action->setText(i18n("&Identities..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("user-identity")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("F8")));
    action->setStatusTip(i18n("Manage your nick, away and other identity settings"));
    connect(action, SIGNAL(triggered()), SLOT(openIdentitiesDialog()));
    actionCollection()->addAction(QStringLiteral("identities_dialog"), action);

    action=new KToggleAction(this);
    action->setText(i18n("&Watched Nicks"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("im-user")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("F4")));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openNicksOnlinePanel()));
    actionCollection()->addAction(QStringLiteral("open_nicksonline_window"), action);


    action=new KToggleAction(this);
    action->setText(i18n("&DCC Status"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right-double")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("F9")));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleDccPanel()));
    actionCollection()->addAction(QStringLiteral("open_dccstatus_window"), action);



    action=new QAction(this);
    action->setText(i18n("&Open Logfile"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+O")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Open the known history for this channel in a new tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openLogFile()));
    actionCollection()->addAction(QStringLiteral("open_logfile"), action);

    action=new QAction(this);
    action->setText(i18n("&Channel Settings..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Open the channel settings dialog for this tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openChannelSettings()));
    actionCollection()->addAction(QStringLiteral("channel_settings"), action);

    action=new KToggleAction(this);
    action->setText(i18n("Channel &List"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-list-text")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("F5")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Show a list of all the known channels on this server"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openChannelList()));
    actionCollection()->addAction(QStringLiteral("open_channel_list"), action);

    action=new KToggleAction(this);
    action->setText(i18n("&URL Catcher"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("text-html")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("F6")));
    action->setStatusTip(i18n("List all URLs that have been mentioned recently in a new tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(addUrlCatcher()));
    actionCollection()->addAction(QStringLiteral("open_url_catcher"), action);

    if (KAuthorized::authorizeKAction(QStringLiteral("shell_access")))
    {
        action=new QAction(this);
        action->setText(i18n("New &Konsole"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
        action->setStatusTip(i18n("Open a terminal in a new tab"));
        connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(addKonsolePanel()));
        actionCollection()->addAction(QStringLiteral("open_konsole"), action);
    }

    // Actions to navigate through the different pages
    QList<QKeySequence> nextShortcut = KStandardShortcut::tabNext();
    QList<QKeySequence> prevShortcut = KStandardShortcut::tabPrev();

    QString nextIcon, prevIcon;
    if (QApplication::isRightToLeft())
    {
        prevShortcut.append(QKeySequence(QStringLiteral("Alt+Right")));
        nextShortcut.append(QKeySequence(QStringLiteral("Alt+Left")));
        nextIcon=QStringLiteral("go-previous-view");
        prevIcon=QStringLiteral("go-next-view");
    }
    else
    {
        nextShortcut.append(QKeySequence(QStringLiteral("Alt+Right")));
        prevShortcut.append(QKeySequence(QStringLiteral("Alt+Left")));
        nextIcon=QStringLiteral("go-next-view");
        prevIcon=QStringLiteral("go-previous-view");
    }

    action=new QAction(this);
    action->setText(i18n("&Next Tab"));
    action->setIcon(QIcon::fromTheme(nextIcon));
    actionCollection()->setDefaultShortcuts(action,nextShortcut);
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showNextView()));
    actionCollection()->addAction(QStringLiteral("next_tab"), action);

    action=new QAction(this);
    action->setText(i18n("&Previous Tab"));
    action->setIcon(QIcon::fromTheme(prevIcon));
    actionCollection()->setDefaultShortcuts(action, prevShortcut);
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showPreviousView()));
    actionCollection()->addAction(QStringLiteral("previous_tab"), action);

    action=new QAction(this);
    action->setText(i18n("Close &Tab"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-close-other")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+w")));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(closeCurrentView()));
    actionCollection()->addAction(QStringLiteral("close_tab"), action);

    action=new QAction(this);
    action->setText(i18n("Last Focused Tab"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Alt+Space")));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showLastFocusedView()));
    actionCollection()->addAction(QStringLiteral("last_focused_tab"), action);

    action=new QAction(this);
    action->setText(i18n("Next Active Tab"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+Alt+Space")));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showNextActiveView()));
    actionCollection()->addAction(QStringLiteral("next_active_tab"), action);
    KGlobalAccel::setGlobalShortcut(action, QList<QKeySequence>());

    if (Preferences::self()->tabPlacement()==Preferences::Left)
    {
        action=new QAction(this);
        action->setText(i18n("Move Tab Up"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
        actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Alt+Shift+Left")));
        action->setEnabled(false);
        action->setStatusTip(i18n("Move this tab"));
        connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewLeft()));
        actionCollection()->addAction(QStringLiteral("move_tab_left"), action);

        action->setEnabled(false);
        action->setStatusTip(i18n("Move this tab"));
        action=new QAction(this);
        action->setText(i18n("Move Tab Down"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
        actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("Alt+Shift+Right")));
        connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewRight()));
        actionCollection()->addAction(QStringLiteral("move_tab_right"), action);
    }
    else
    {
        if (QApplication::isRightToLeft())
        {
            action=new QAction(this);
            action->setText(i18n("Move Tab Right"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
            actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("Alt+Shift+Right")));
            action->setEnabled(false);
            action->setStatusTip(i18n("Move this tab"));
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewLeft()));
            actionCollection()->addAction(QStringLiteral("move_tab_left"), action);

            action=new QAction(this);
            action->setText(i18n("Move Tab Left"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
            actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("Alt+Shift+Left")));
            action->setEnabled(false);
            action->setStatusTip(i18n("Move this tab"));
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewRight()));
            actionCollection()->addAction(QStringLiteral("move_tab_right"), action);

        }
        else
        {
            action=new QAction(this);
            action->setText(i18n("Move Tab Left"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
            actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("Alt+Shift+Left")));
            action->setEnabled(false);
            action->setStatusTip(i18n("Move this tab"));
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewLeft()));
            actionCollection()->addAction(QStringLiteral("move_tab_left"), action);

            action=new QAction(this);
            action->setText(i18n("Move Tab Right"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
            actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("Alt+Shift+Right")));
            action->setEnabled(false);
            action->setStatusTip(i18n("Move this tab"));
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewRight()));
            actionCollection()->addAction(QStringLiteral("move_tab_right"), action);

        }

    }

    action->setEnabled(false);
    action=new QAction(this);
    action->setText(i18n("Rejoin Channel"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(rejoinChannel()));
    actionCollection()->addAction(QStringLiteral("rejoin_channel"), action);

    action->setEnabled(false);
    action=new KToggleAction(this);
    action->setText(i18n("Enable Notifications"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleViewNotifications()));
    actionCollection()->addAction(QStringLiteral("tab_notifications"), action);

    action->setEnabled(false);
    action=new KToggleAction(this);
    action->setText(i18n("Join on Connect"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleAutoJoin()));
    actionCollection()->addAction(QStringLiteral("tab_autojoin"), action);

    action=new KToggleAction(this);
    action->setText(i18n("Connect at Startup"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleConnectOnStartup()));
    actionCollection()->addAction(QStringLiteral("tab_autoconnect"), action);

    QStringList encodingDescs = Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames();
    encodingDescs.prepend(i18n("Default"));
    KSelectAction* selectAction = new KSelectAction(this);
    selectAction->setEditable(false);
    selectAction->setItems(encodingDescs);
    selectAction->setEnabled(false);
    selectAction->setText(i18n("Set Encoding"));
    selectAction->setIcon(QIcon::fromTheme(QStringLiteral("character-set")));
    connect(selectAction, SIGNAL(triggered(int)), m_viewContainer, SLOT(changeViewCharset(int)));
    actionCollection()->addAction(QStringLiteral("tab_encoding"), selectAction);

    QSignalMapper* tabSelectionMapper = new QSignalMapper(this);
    connect(tabSelectionMapper, SIGNAL(mapped(int)), m_viewContainer, SLOT(goToView(int)));

    for (uint i = 1; i <= 10; ++i)
    {

        action=new QAction(this);
        action->setText(i18n("Go to Tab %1",i));
        actionCollection()->setDefaultShortcut(action,QKeySequence(QString(QStringLiteral("Alt+%1")).arg(i%10)));
        connect(action, SIGNAL(triggered()), tabSelectionMapper, SLOT(map()));
        actionCollection()->addAction(QString(QStringLiteral("go_to_tab_%1")).arg(i), action);

        tabSelectionMapper->setMapping(action, i-1);
    }

    action=new QAction(this);
    action->setText(i18n("Clear &Marker Lines"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+Shift+R")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Clear marker lines in the current tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(clearViewLines()));
    actionCollection()->addAction(QStringLiteral("clear_lines"), action);

    action=new QAction(this);
    action->setText(i18n("&Clear Window"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+L")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Clear the contents of the current tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(clearView()));
    actionCollection()->addAction(QStringLiteral("clear_window"), action);

    action=new QAction(this);
    action->setText(i18n("Clear &All Windows"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Clear the contents of all open tabs"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(clearAllViews()));
    actionCollection()->addAction(QStringLiteral("clear_tabs"), action);

    KToggleAction* awayAction = new KToggleAction(this);
    awayAction->setText(i18n("Global Away"));
    actionCollection()->setDefaultShortcut(awayAction,QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    awayAction->setEnabled(false);
    awayAction->setIcon(QIcon::fromTheme(QStringLiteral("im-user-away")));
    connect(awayAction, SIGNAL(triggered(bool)), Application::instance()->getAwayManager(), SLOT(setGlobalAway(bool)));
    actionCollection()->addAction(QStringLiteral("toggle_away"), awayAction);

    action=new QAction(this);
    action->setText(i18n("&Join Channel..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("irc-join-channel")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+J")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Join a new channel on this server"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showJoinChannelDialog()));
    actionCollection()->addAction(QStringLiteral("join_channel"), action);

    action = KStandardAction::find(m_viewContainer, SLOT(findText()), actionCollection());
    action->setEnabled(false);
    action = KStandardAction::findNext(m_viewContainer, SLOT(findNextText()), actionCollection());
    action->setEnabled(false);
    action = KStandardAction::findPrev(m_viewContainer, SLOT(findPrevText()), actionCollection());
    action->setEnabled(false);

    action=new QAction(this);
    action->setText(i18n("&IRC Color..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("format-text-color")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+K")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Set the color of your current IRC message"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(insertIRCColor()));
    actionCollection()->addAction(QStringLiteral("irc_colors"), action);

    action=new QAction(this);
    action->setText(i18n("&Marker Line"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Ctrl+R")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Insert a horizontal line into the current tab that only you can see"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(insertMarkerLine()));
    actionCollection()->addAction(QStringLiteral("insert_marker_line"), action);

    action=new QAction(this);
    action->setText(i18n("Special &Character..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("character-set")));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("Alt+Shift+C")));
    action->setEnabled(false);
    action->setStatusTip(i18n("Insert any character into your current IRC message"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(insertCharacter()));
    actionCollection()->addAction(QStringLiteral("insert_character"), action);

    action=new QAction(this);
    action->setText(i18n("Auto Replace"));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(doAutoReplace()));
    actionCollection()->addAction(QStringLiteral("auto_replace"), action);

    action=new QAction(this);
    action->setText(i18n("Focus Input Box"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(Qt::Key_Escape));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(focusInputBox()));
    actionCollection()->addAction(QStringLiteral("focus_input_box"), action);

    action=new QAction(this);
    action->setText(i18n("Close &All Open Queries"));
    actionCollection()->setDefaultShortcut(action,QKeySequence(QStringLiteral("F11")));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(closeQueries()));
    actionCollection()->addAction(QStringLiteral("close_queries"), action);

    KToggleAction* toggleChannelNickListsAction = new KToggleAction(this);
    if (Preferences::self()->showNickList())
        toggleChannelNickListsAction->setChecked(true);
    toggleChannelNickListsAction->setText(i18n("Show Nicklist"));
    actionCollection()->setDefaultShortcut(toggleChannelNickListsAction, QKeySequence(QStringLiteral("Ctrl+H")));
    connect(toggleChannelNickListsAction, SIGNAL(triggered()), m_viewContainer, SLOT(toggleChannelNicklists()));
    actionCollection()->addAction(QStringLiteral("hide_nicknamelist"), toggleChannelNickListsAction);

    action=new QAction(this);
    action->setText(i18n("Show/Hide Konversation"));
    connect(action, SIGNAL(triggered()), this, SLOT(toggleVisibility()));
    actionCollection()->addAction(QStringLiteral("toggle_mainwindow_visibility"), action);
    KGlobalAccel::setGlobalShortcut(action, QList<QKeySequence>());

    action=new KToggleAction(this);
    action->setEnabled(true);
    action->setChecked(Preferences::self()->useOSD());
    action->setText(i18n("Enable On Screen Display"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("video-display")));
    connect(action, SIGNAL(triggered(bool)), Preferences::self(), SLOT(slotSetUseOSD(bool)));
    actionCollection()->addAction(QStringLiteral("toggle_osd"), action);

    // Bookmarks
    action=new QAction(this);
    action->setText(i18n("Bookmarks"));
    QMenu *menu = new QMenu(this);
    action->setMenu(menu);
    new KonviBookmarkHandler(menu, this);
    actionCollection()->addAction(QStringLiteral("bookmarks") , action);

    // decide whether to show the tray icon or not
    updateTrayIcon();

    createGUI();

    setAutoSaveSettings();

    // Apply menubar show/hide pref
    m_showMenuBarAction->setChecked(Preferences::self()->showMenuBar());
    toggleMenubar(true);

    if (Preferences::self()->useNotify() && Preferences::self()->openWatchedNicksAtStartup())
        m_viewContainer->openNicksOnlinePanel();

}

MainWindow::~MainWindow()
{
}

QSize MainWindow::sizeHint() const
{
    return QSize(700, 500); // Give the app a sane default size
}

int MainWindow::confirmQuit()
{
    Application* konvApp = Application::instance();

    if (konvApp->getConnectionManager()->connectionCount() == 0)
        return KMessageBox::Continue;

    int result = KMessageBox::Cancel;

    if (!KMessageBox::shouldBeShownContinue(QStringLiteral("systemtrayquitKonversation"))
         && konvApp->getDccTransferManager()->hasActiveTransfers())
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("<qt>You have active DCC file transfers. Are you sure you want to quit <b>Konversation</b>?</qt>"),
            i18n("Confirm Quit"),
            KStandardGuiItem::quit(),
            KStandardGuiItem::cancel(),
            QStringLiteral("QuitWithActiveDccTransfers"));
    }
    else
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("<qt>Are you sure you want to quit <b>Konversation</b>?</qt>"),
            i18n("Confirm Quit"),
            KStandardGuiItem::quit(),
            KStandardGuiItem::cancel(),
            QStringLiteral("systemtrayquitKonversation"));
    }

    if (result != KMessageBox::Continue)
        konvApp->abortScheduledRestart();

    return result;
}

void MainWindow::activateAndRaiseWindow()
{
    if (isMinimized())
        KWindowSystem::unminimizeWindow(winId());
    else if (Preferences::self()->showTrayIcon() && !isVisible())
        m_trayIcon->restore();

    KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());
    KWindowSystem::activateWindow(winId());
}

void MainWindow::quitProgram()
{
    if (Preferences::self()->showTrayIcon() &&
        sender() != m_trayIcon &&
        confirmQuit() == KMessageBox::Cancel) return;

    // will call queryClose()
    m_closeApp = true;
    close();
}

bool MainWindow::queryClose()
{
    Application* konvApp = Application::instance();

    if (!konvApp->isSavingSession())
    {
        if (sender() == m_trayIcon)
            m_closeApp = true;

        if (Preferences::self()->showTrayIcon() && !m_closeApp)
        {
            bool doit = KMessageBox::warningContinueCancel(this,
                        i18n("<p>Closing the main window will keep Konversation running in the system tray. "
                        "Use <b>Quit</b> from the <b>Konversation</b> menu to quit the application.</p>"),
                        i18n("Docking in System Tray"),
                        KStandardGuiItem::cont(),
                        KStandardGuiItem::cancel(),
                        QLatin1String("HideOnCloseInfo")) == KMessageBox::Continue;
            if (doit)
                hide();

            return false;
        }

        if (!Preferences::self()->showTrayIcon() && confirmQuit() == KMessageBox::Cancel)
            return false;
    }

    konvApp->prepareShutdown();

    return true;
}

void MainWindow::hideEvent(QHideEvent *e)
{
    emit triggerRememberLine();

    m_statusBar->clearMainLabelTempText();

    KXmlGuiWindow::hideEvent(e);
}

void MainWindow::showEvent(QShowEvent *e)
{
    emit cancelRememberLine();

    KXmlGuiWindow::showEvent(e);
}

void MainWindow::leaveEvent(QEvent* e)
{
    m_statusBar->clearMainLabelTempText();

    KXmlGuiWindow::leaveEvent(e);
}

bool MainWindow::event(QEvent* e)
{
    if (e->type() == QEvent::StyleChange)
    {
        QMetaObject::invokeMethod(Application::instance(), "appearanceChanged");
    }
    else if (e->type() == QEvent::WindowActivate)
    {
        emit endNotification();
        emit cancelRememberLine();
    }
    else if(e->type() == QEvent::WindowDeactivate)
    {
        m_statusBar->clearMainLabelTempText();

        if (qApp->activeModalWidget() == 0)
            emit triggerRememberLine();
    }

    return KXmlGuiWindow::event(e);
}

void MainWindow::settingsChangedSlot()
{
    // This is for compressing the events. m_hasDirtySettings is set to true
    // when the settings have changed, then set to false when the app reacts to it
    // via the appearanceChanged signal.  This prevents a series of settingsChanged signals
    // causing the app expensively rereading its settings many times.
    // The appearanceChanged signal is connected to resetHasDirtySettings to reset this bool
    if (!m_hasDirtySettings)
    {
        QTimer::singleShot(0, Application::instance(), SIGNAL(appearanceChanged()));
        m_hasDirtySettings = true;
    }
}

void MainWindow::resetHasDirtySettings()
{
    m_hasDirtySettings = false;
}

void MainWindow::updateTrayIcon()
{
    if (Preferences::self()->showTrayIcon())
    {
        if (!m_trayIcon)
        {
            // set up system tray
            m_trayIcon = new Konversation::TrayIcon(this);
            connect(this, SIGNAL(endNotification()), m_trayIcon, SLOT(endNotification()));
            connect(KIconLoader::global(), SIGNAL(iconChanged(int)), m_trayIcon, SLOT(updateAppearance()));
            QMenu *trayMenu = qobject_cast<QMenu*>(m_trayIcon->contextMenu());
            trayMenu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::Preferences))));
            trayMenu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::ConfigureNotifications))));
            trayMenu->addAction(actionCollection()->action(QStringLiteral("toggle_away")));
        }

        m_trayIcon->setNotificationEnabled(Preferences::self()->trayNotify());
    }
    else
    {
        delete m_trayIcon;
        m_trayIcon = 0;
    }
}

void MainWindow::toggleMenubar(bool dontShowWarning)
{
    if (m_showMenuBarAction->isChecked())
        menuBar()->show();
    else
    {
        bool doit = true;
        if (!dontShowWarning)
        {
            QString accel = m_showMenuBarAction->shortcut().toString();
            doit = KMessageBox::warningContinueCancel(this,
                    i18n("<qt>This will hide the menu bar completely. You can show it again by typing %1.</qt>", accel),
                    i18n("Hide menu bar"),
                    KStandardGuiItem::cont(),
                    KStandardGuiItem::cancel(),
                    QLatin1String("HideMenuBarWarning")) == KMessageBox::Continue;
        }
        if (doit)
            menuBar()->hide();
        else
            m_showMenuBarAction->setChecked (true);
    }

    Preferences::self()->setShowMenuBar(m_showMenuBarAction->isChecked());
}

void MainWindow::focusAndShowErrorMessage(const QString &errorMsg)
{
    show();
    KWindowSystem::demandAttention(winId());
    KWindowSystem::activateWindow(winId());
    KMessageBox::error(this, errorMsg);
}

void MainWindow::openPrefsDialog()
{
    //An instance of your dialog could be already created and could be cached,
    //in which case you want to display the cached dialog instead of creating
    //another one
    if (!m_settingsDialog)
    {
        m_settingsDialog = new KonviSettingsDialog(this);
        //User edited the configuration - update your local copies of the
        //configuration data
        connect(m_settingsDialog, SIGNAL(settingsChanged(QString)), this, SLOT(settingsChangedSlot()));
    }
    m_settingsDialog->show();
}

void MainWindow::openKeyBindings()
{
    // Change a number of action names to make them friendlier for the shortcut list.
    actionCollection()->action(QStringLiteral("tab_notifications"))->setText(i18n("Toggle Notifications"));
    actionCollection()->action(QStringLiteral("toggle_away"))->setText(i18n("Toggle Away Globally"));
    actionCollection()->action(QStringLiteral("irc_colors"))->setText(i18n("Insert &IRC Color..."));
    actionCollection()->action(QStringLiteral("insert_character"))->setText(i18n("Insert Special &Character..."));
    actionCollection()->action(QStringLiteral("insert_marker_line"))->setText(i18n("Insert &Marker Line"));
    QString openChannelListString = actionCollection()->action(QStringLiteral("open_channel_list"))->text();
    actionCollection()->action(QStringLiteral("open_channel_list"))->setText(i18n("&Channel List"));
    QString openLogFileString = actionCollection()->action(QStringLiteral("open_logfile"))->text();
    actionCollection()->action(QStringLiteral("open_logfile"))->setText(i18n("&Open Logfile"));

    // Open shortcut configuration dialog.
    KShortcutsDialog::configure(actionCollection());

    // Reset action names.
    actionCollection()->action(QStringLiteral("tab_notifications"))->setText(i18n("Enable Notifications"));
    actionCollection()->action(QStringLiteral("toggle_away"))->setText(i18n("Set &Away Globally"));
    actionCollection()->action(QStringLiteral("irc_colors"))->setText(i18n("&IRC Color..."));
    actionCollection()->action(QStringLiteral("insert_character"))->setText(i18n("Special &Character..."));
    actionCollection()->action(QStringLiteral("insert_marker_line"))->setText(i18n("&Marker Line"));
    actionCollection()->action(QStringLiteral("open_channel_list"))->setText(openChannelListString);
    actionCollection()->action(QStringLiteral("open_logfile"))->setText(openLogFileString);
}

void MainWindow::openServerList()
{
    if (!m_serverListDialog)
    {
        m_serverListDialog = new Konversation::ServerListDialog(i18n("Server List"), this);
        Application* konvApp = Application::instance();

        connect(m_serverListDialog, SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)),
                konvApp, SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)));
        connect(konvApp, SIGNAL(serverGroupsChanged(Konversation::ServerGroupSettingsPtr)),
                m_serverListDialog, SLOT(updateServerList()));
        connect(m_serverListDialog, SIGNAL(connectTo(Konversation::ConnectionFlag,int)),
                konvApp->getConnectionManager(), SLOT(connectTo(Konversation::ConnectionFlag,int)));
        connect(m_serverListDialog, SIGNAL(connectTo(Konversation::ConnectionFlag,ConnectionSettings)),
                konvApp->getConnectionManager(), SLOT(connectTo(Konversation::ConnectionFlag,ConnectionSettings)));
        connect(konvApp->getConnectionManager(), SIGNAL(closeServerList()), m_serverListDialog, SLOT(reject()));
    }

    m_serverListDialog->show();
}

void MainWindow::openQuickConnectDialog()
{
    emit showQuickConnectDialog();
}


void MainWindow::openIdentitiesDialog()
{
    QPointer<Konversation::IdentityDialog> dlg = new Konversation::IdentityDialog(this);
    if (dlg->exec() == QDialog::Accepted)
    {
        if (m_serverListDialog)
            m_serverListDialog->updateServerList();
        m_viewContainer->updateViewEncoding(m_viewContainer->getFrontView());
    }
    delete dlg;
}

IdentityPtr MainWindow::editIdentity(IdentityPtr identity)
{
    IdentityPtr newIdentity;

    QPointer<Konversation::IdentityDialog> dlg = new Konversation::IdentityDialog(this);
    newIdentity = dlg->setCurrentIdentity(identity);

    if ((dlg->exec() == QDialog::Accepted) && m_serverListDialog)
    {
        m_serverListDialog->updateServerList();
        delete dlg;
        return newIdentity;
    }
    else
    {
        delete dlg;
        return IdentityPtr();
    }
}

void MainWindow::openNotifications()
{
    (void) KNotifyConfigWidget::configure(this);
}

void MainWindow::notifyAction(int connectionId, const QString& nick)
{
    Application* konvApp = Application::instance();
    Server* server = konvApp->getConnectionManager()->getServerByConnectionId(connectionId);
    if (server) server->notifyAction(nick);
}

// TODO: Let an own class handle notify things
void MainWindow::setOnlineList(Server* notifyServer,const QStringList& /*list*/, bool /*changed*/)
{
    emit nicksNowOnline(notifyServer);
    // FIXME  if (changed && nicksOnlinePanel) newText(nicksOnlinePanel, QString::null, true);
}

void MainWindow::toggleVisibility()
{
    if (isActiveWindow())
    {
        if (Preferences::self()->showTrayIcon())
            hide();
        else
            KWindowSystem::minimizeWindow(winId());
    }
    else
    {
        activateAndRaiseWindow();
    }
}


