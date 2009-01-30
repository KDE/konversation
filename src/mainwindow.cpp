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

#include "mainwindow.h" ////// header renamed
#include "application.h" ////// header renamed
//#include "linkaddressbook/addressbook.h"
//#include "settingsdialog.h" ////// header renamed
#include "viewcontainer.h"
#include "statusbar.h" ////// header renamed
#include "bookmarkhandler.h" ////// header renamed
#include "trayicon.h"
#include "serverlistdialog.h"
#include "identitydialog.h"
#include "notificationhandler.h"
#include "irccharsets.h"
#include "connectionmanager.h"
#include "awaymanager.h"
#include "transfermanager.h" ////// header renamed

#include <qnamespace.h>
#include <q3whatsthis.h>
#include <qsignalmapper.h>
#include <qobject.h>
//Added by qt3to4:
#include <QHideEvent>
#include <QEvent>
#include <QShowEvent>
#include <QSplitter>
#include <QKeySequence>

//#include <kaccel.h>
#include <KActionCollection>
#include <KAction>
#include <KToggleAction>
#include <KSelectAction>
#include <KAuthorized>
#include <kacceleratormanager.h>
#include <kstandardaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenubar.h>
//#include <kkeydialog.h>
#include <kdeversion.h>
#include <kedittoolbar.h>
#include <kmenu.h>
#include <kwindowsystem.h>
#include <kglobal.h>
#include <kstandarddirs.h>
//#include <dcopclient.h>
//#include <scriptmanager.h>
#include <kabc/addressbook.h>
#include <kabc/errorhandler.h>
#include <KShortcutsDialog>

#include <config-konvi.h>
#ifdef  USE_KNOTIFY
#include <knotifyconfigwidget.h>
#include <kauthorized.h>
#include <KShortcutsDialog>
#endif


KonversationMainWindow::KonversationMainWindow() : KXmlGuiWindow(0, Qt::WStyle_ContextHelp | Qt::WType_TopLevel | Qt::WDestructiveClose)
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
    connect(KGlobalSettings::self(), SIGNAL(iconChanged(int)), m_viewContainer, SLOT(updateViewIcons()));
    connect(KonversationApplication::instance(), SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettingsPtr)),
            m_viewContainer, SLOT(updateViews(const Konversation::ServerGroupSettings*)));
    connect(m_viewContainer, SIGNAL(autoJoinToggled(const Konversation::ServerGroupSettingsPtr)),
            KonversationApplication::instance(), SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettingsPtr)));
    connect(m_viewContainer, SIGNAL(setWindowCaption(const QString&)), this, SLOT(setCaption(const QString&)));
    connect(KonversationApplication::instance()->getConnectionManager(),
            SIGNAL(connectionChangedState(Server*, Konversation::ConnectionState)),
            m_viewContainer, SLOT(connectionStateChanged(Server*, Konversation::ConnectionState)));
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
    KStandardAction::quit(this,SLOT(quitProgram()),actionCollection());

    hideMenuBarAction = KStandardAction::showMenubar(this, SLOT(toggleMenubar()), actionCollection());

    setStandardToolBarMenuEnabled(true);
    KStandardAction::configureToolbars(this, SLOT(configureToolbars()), actionCollection());

    KStandardAction::keyBindings(this, SLOT(openKeyBindings()), actionCollection());
    KAction *preferencesAction = KStandardAction::preferences(this, SLOT(openPrefsDialog()), actionCollection());

#ifdef USE_KNOTIFY // options_configure_notifications
    KAction *configureNotificationsAction = KStandardAction::configureNotifications(this,SLOT(openNotifications()), actionCollection());
#endif

    KAction* action;

    //(new KAction(i18n("&Server List..."), "server", KShortcut("F2"), this, SLOT(openServerList()),actionCollection(), "open_server_list"))->setToolTip(i18n("Manage networks and servers"));
    action=new KAction(this);
    action->setText(i18n("&Server List..."));
    action->setIcon(KIcon("server"));
    action->setShortcut(KShortcut("F2"));
    action->setToolTip(i18n("Manage networks and servers"));
    connect(action, SIGNAL(triggered()), SLOT(openServerList()));
    actionCollection()->addAction("open_server_list", action);

    //(new KAction(i18n("Quick &Connect..."), "connect_creating", KShortcut("F7"), this, SLOT(openQuickConnectDialog()), actionCollection(), "quick_connect_dialog"))->setToolTip(i18n("Type in the address of a new IRC server to connect to"));
    action=new KAction(this);
    action->setText(i18n("Quick &Connect..."));
    action->setIcon(KIcon("connect_creating"));
    action->setShortcut(KShortcut("F7"));
    action->setToolTip(i18n("Type in the address of a new IRC server to connect to"));
    connect(action, SIGNAL(triggered()), SLOT(openQuickConnectDialog()));
    actionCollection()->addAction("quick_connect_dialog", action);

    //action = new KAction(i18n("&Reconnect"), "connect_creating", 0, m_viewContainer, SLOT(reconnectFrontServer()), actionCollection(), "reconnect_server");
    action=new KAction(this);
    action->setText(i18n("&Reconnect"));
    action->setIcon(KIcon("view-refresh"));
    action->setEnabled(false);
    action->setToolTip(i18n("Reconnect to the current server."));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(reconnectFrontServer()));
    actionCollection()->addAction("reconnect_server", action);


    //action = new KAction(i18n("&Disconnect"), "connect_no", 0, m_viewContainer, SLOT(disconnectFrontServer()), actionCollection(), "disconnect_server");
    action=new KAction(this);
    action->setText(i18n("&Disconnect"));
    action->setIcon(KIcon("network-disconnect"));
    action->setEnabled(false);
    action->setToolTip(i18n("Disconnect from the current server."));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(disconnectFrontServer()));
    actionCollection()->addAction("disconnect_server", action);

    //(new KAction(i18n("&Identities..."), "identity", KShortcut("F8"), this, SLOT(openIdentitiesDialog()), actionCollection(), "identities_dialog"))->setToolTip(i18n("Manage your nick, away and other identity settings"));
    action=new KAction(this);
    action->setText(i18n("&Identities..."));
    action->setIcon(KIcon("user-identity"));
    action->setShortcut(KShortcut("F8"));
    action->setToolTip(i18n("Manage your nick, away and other identity settings"));
    connect(action, SIGNAL(triggered()), SLOT(openIdentitiesDialog()));
    actionCollection()->addAction("identities_dialog", action);

    //new KToggleAction(i18n("&Watched Nicks Online"), "kontact_contacts", KShortcut("F4"), m_viewContainer, SLOT(openNicksOnlinePanel()), actionCollection(), "open_nicksonline_window");
    action=new KToggleAction(this);
    action->setText(i18n("&Watched Nicks Online"));
    action->setIcon(KIcon("kontact_contacts"));
    action->setShortcut(KShortcut("F4"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openNicksOnlinePanel()));
    actionCollection()->addAction("open_nicksonline_window", action);


    //new KToggleAction(i18n("&DCC Status"), "2rightarrow", KShortcut("F9"), m_viewContainer, SLOT(toggleDccPanel()), actionCollection(), "open_dccstatus_window");
    action=new KToggleAction(this);
    action->setText(i18n("&DCC Status"));
    action->setIcon(KIcon("arrow-right-double"));
    action->setShortcut(KShortcut("F9"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleDccPanel()));
    actionCollection()->addAction("open_dccstatus_window", action);



    //action = new KAction(i18n("&Open Logfile"), "history", KShortcut("Ctrl+O"), m_viewContainer, SLOT(openLogFile()), actionCollection(), "open_logfile");
    action=new KAction(this);
    action->setText(i18n("&Open Logfile"));
    action->setIcon(KIcon("view-history"));
    action->setShortcut(KShortcut("Ctrl+O"));
    action->setEnabled(false);
    action->setToolTip(i18n("Open the known history for this channel in a new tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openLogFile()));
    actionCollection()->addAction("open_logfile", action);

    //action = new KAction(i18n("&Channel Settings..."), "edit", m_viewContainer, SLOT(openChannelSettings()), actionCollection(), "channel_settings");
    action=new KAction(this);
    action->setText(i18n("&Channel Settings..."));
    action->setIcon(KIcon("configure"));
    action->setEnabled(false);
    action->setToolTip(i18n("Open the channel settings dialog for this tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openChannelSettings()));
    actionCollection()->addAction("channel_settings", action);

    //KToggleAction* channelListAction = new KToggleAction(i18n("Channel &List"), "view_text", KShortcut("F5"), m_viewContainer, SLOT(openChannelList()), actionCollection(), "open_channel_list");
    action=new KToggleAction(this);
    action->setText(i18n("Channel &List"));
    action->setIcon(KIcon("view-list-text"));
    action->setShortcut(KShortcut("F5"));
    action->setEnabled(false);
    action->setToolTip(i18n("Show a list of all the known channels on this server"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(openChannelList()));
    actionCollection()->addAction("open_channel_list", action);

    //action = new KToggleAction(i18n("&URL Catcher"), "enhanced_browsing", KShortcut("F6"), m_viewContainer, SLOT(addUrlCatcher()), actionCollection(), "open_url_catcher");
    action=new KToggleAction(this);
    action->setText(i18n("&URL Catcher"));
    action->setIcon(KIcon("enhanced_browsing"));
    action->setShortcut(KShortcut("F6"));
    action->setToolTip(i18n("List all URLs that have been mentioned recently in a new tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(addUrlCatcher()));
    actionCollection()->addAction("open_url_catcher", action);

    if (KAuthorized::authorizeKAction("shell_access"))
    {
        //action = new KAction(i18n("New &Konsole"), "openterm", 0, m_viewContainer, SLOT(addKonsolePanel()), actionCollection(), "open_konsole");
        action=new KAction(this);
        action->setText(i18n("New &Konsole"));
        action->setIcon(KIcon("utilities-terminal"));
        action->setToolTip(i18n("Open a terminal in a new tab"));
        connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(addKonsolePanel()));
        actionCollection()->addAction("open_konsole", action);
    }

    // Actions to navigate through the different pages
    KShortcut nextShortcut = KStandardShortcut::tabNext();
    KShortcut prevShortcut = KStandardShortcut::tabPrev();

    const char *nextIcon, *prevIcon;
    if (QApplication::isRightToLeft())
    {
        prevShortcut.setAlternate(QKeySequence("Alt+Right"));
        nextShortcut.setAlternate(QKeySequence("Alt+Left"));
        nextIcon="go-previous-view";
        prevIcon="go-next-view";
    }
    else
    {
        nextShortcut.setAlternate(QKeySequence("Alt+Right"));
        prevShortcut.setAlternate(QKeySequence("Alt+Left"));
        nextIcon="go-next-view";
        prevIcon="go-previous-view";
    }

    //action = new KAction(i18n("&Next Tab"), QApplication::isRightToLeft() ? "previous" : "next", QApplication::isRightToLeft() ? prevShortcut : nextShortcut, m_viewContainer, SLOT(showNextView()), actionCollection(), "next_tab");
    action=new KAction(this);
    action->setText(i18n("&Next Tab"));
    action->setIcon(KIcon(nextIcon));
    action->setShortcut(nextShortcut);
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showNextView()));
    actionCollection()->addAction("next_tab", action);

    //action = new KAction(i18n("&Previous Tab"), QApplication::isRightToLeft() ? "next" : "previous", QApplication::isRightToLeft() ? nextShortcut : prevShortcut, m_viewContainer, SLOT(showPreviousView()),actionCollection(),"previous_tab");
    action=new KAction(this);
    action->setText(i18n("&Previous Tab"));
    action->setIcon(KIcon(prevIcon));
    action->setShortcut(prevShortcut);
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showPreviousView()));
    actionCollection()->addAction("previous_tab", action);

    //action = new KAction(i18n("Close &Tab"),"tab_remove",KShortcut("Ctrl+w"), m_viewContainer, SLOT(closeCurrentView()),actionCollection(),"close_tab");
    action=new KAction(this);
    action->setText(i18n("Close &Tab"));
    action->setIcon(KIcon("tab-close-other"));
    action->setShortcut(KShortcut("Ctrl+w"));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(closeCurrentView()));
    actionCollection()->addAction("close_tab", action);

    //action = new KAction(i18n("Next Active Tab"), 0, KShortcut("Ctrl+Alt+Space"), m_viewContainer, SLOT(showNextActiveView()), actionCollection(), "next_active_tab");
    action=new KAction(this);
    action->setText(i18n("Next Active Tab"));
    action->setShortcut(KShortcut("Ctrl+Alt+Space"));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showNextActiveView()));
    actionCollection()->addAction("next_active_tab", action);

    if (Preferences::tabPlacement()==Preferences::Left)
    {
        //action = new KAction(i18n("Move Tab Up"), "1uparrow", KShortcut("Alt+Shift+Left"), m_viewContainer, SLOT(moveViewLeft()), actionCollection(), "move_tab_left");
        action=new KAction(this);
        action->setText(i18n("Move Tab Up"));
        action->setIcon(KIcon("arrow-up"));
        action->setShortcut(KShortcut("Alt+Shift+Left"));
        action->setEnabled(false);
        action->setToolTip("Move this tab");
        connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewLeft()));
        actionCollection()->addAction("move_tab_left", action);

        //action = new KAction(i18n("Move Tab Down"), "1downarrow", KShortcut("Alt+Shift+Right"), m_viewContainer, SLOT(moveViewRight()), actionCollection(), "move_tab_right");
        action->setEnabled(false);
        action->setToolTip("Move this tab");
        action=new KAction(this);
        action->setText(i18n("Move Tab Down"));
        action->setIcon(KIcon("arrow-down"));
        action->setShortcut(KShortcut("Alt+Shift+Right"));
        connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewRight()));
        actionCollection()->addAction("move_tab_right", action);
    }
    else
    {
        if (QApplication::isRightToLeft())
        {
            //action = new KAction(i18n("Move Tab Right"), "1rightarrow", KShortcut("Alt+Shift+Right"), m_viewContainer, SLOT(moveViewLeft()), actionCollection(), "move_tab_left");
            action=new KAction(this);
            action->setText(i18n("Move Tab Right"));
            action->setIcon(KIcon("arrow-right"));
            action->setShortcut(KShortcut("Alt+Shift+Right"));
            action->setEnabled(false);
            action->setToolTip("Move this tab");
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewLeft()));
            actionCollection()->addAction("move_tab_left", action);

            //action  = new KAction(i18n("Move Tab Left"), "1leftarrow", KShortcut("Alt+Shift+Left"), m_viewContainer, SLOT(moveViewRight()), actionCollection(), "move_tab_right");
            action=new KAction(this);
            action->setText(i18n("Move Tab Left"));
            action->setIcon(KIcon("arrow-left"));
            action->setShortcut(KShortcut("Alt+Shift+Left"));
            action->setEnabled(false);
            action->setToolTip("Move this tab");
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewRight()));
            actionCollection()->addAction("move_tab_right", action);

        }
        else
        {
            //action  = new KAction(i18n("Move Tab Left"), "1leftarrow", KShortcut("Alt+Shift+Left"), m_viewContainer, SLOT(moveViewLeft()), actionCollection(), "move_tab_left");
            action=new KAction(this);
            action->setText(i18n("Move Tab Left"));
            action->setIcon(KIcon("arrow-left"));
            action->setShortcut(KShortcut("Alt+Shift+Left"));
            action->setEnabled(false);
            action->setToolTip("Move this tab");
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewLeft()));
            actionCollection()->addAction("move_tab_left", action);

            //action = new KAction(i18n("Move Tab Right"), "1rightarrow", KShortcut("Alt+Shift+Right"), m_viewContainer, SLOT(moveViewRight()), actionCollection(), "move_tab_right");
            action=new KAction(this);
            action->setText(i18n("Move Tab Right"));
            action->setIcon(KIcon("arrow-right"));
            action->setShortcut(KShortcut("Alt+Shift+Right"));
            action->setEnabled(false);
            action->setToolTip("Move this tab");
            connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(moveViewRight()));
            actionCollection()->addAction("move_tab_right", action);

        }

    }

    //action = new KAction(i18n("Rejoin Channel"), 0, m_viewContainer, SLOT(rejoinChannel()), actionCollection(), "rejoin_channel");
    action->setEnabled(false);
    action=new KAction(this);
    action->setText(i18n("Rejoin Channel"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(rejoinChannel()));
    actionCollection()->addAction("rejoin_channel", action);

    //action = new KToggleAction(i18n("Enable Notifications"), 0, 0, m_viewContainer, SLOT(toggleViewNotifications()), actionCollection(), "tab_notifications");
    action->setEnabled(false);
    action=new KToggleAction(this);
    action->setText(i18n("Enable Notifications"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleViewNotifications()));
    actionCollection()->addAction("tab_notifications", action);

    //action = new KToggleAction(i18n("Join on Connect"), 0, 0, m_viewContainer, SLOT(toggleAutoJoin()), actionCollection(), "tab_autojoin");
    action->setEnabled(false);
    action=new KToggleAction(this);
    action->setText(i18n("Join on Connect"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(toggleAutoJoin()));
    actionCollection()->addAction("tab_autojoin", action);

    QStringList encodingDescs = Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames();
    encodingDescs.prepend(i18n("Default"));
    //KSelectAction* selectAction = new KSelectAction(i18n("Set Encoding"), "charset", 0, actionCollection(), "tab_encoding");
    KSelectAction* selectAction = new KSelectAction(this);
    selectAction->setEditable(false);
    selectAction->setItems(encodingDescs);
    selectAction->setEnabled(false);
    selectAction->setText(i18n("Set Encoding"));
    selectAction->setIcon(KIcon("character-set"));
    connect(selectAction, SIGNAL(activated(int)), m_viewContainer, SLOT(changeViewCharset(int)));
    actionCollection()->addAction("tab_encoding", selectAction);

    QSignalMapper* tabSelectionMapper = new QSignalMapper(this);
    connect(tabSelectionMapper, SIGNAL(mapped(int)), m_viewContainer, SLOT(goToView(int)));

    for (uint i = 1; i <= 10; ++i)
    {
        //KAction* tabSelectionAction = new KAction(i18n("Go to Tab %1",i), 0, KShortcut(QString("Alt+%1").arg(i%10)), tabSelectionMapper, SLOT(map()), actionCollection(), QString("go_to_tab_%1").arg(i).local8Bit());

        action=new KAction(this);
        action->setText(i18n("Go to Tab %1",i));
        action->setShortcut(KShortcut(QString("Alt+%1").arg(i%10)));
        connect(action, SIGNAL(triggered()), tabSelectionMapper, SLOT(map()));
        actionCollection()->addAction(QString("go_to_tab_%1").arg(i).local8Bit(), action);

        tabSelectionMapper->setMapping(action, i-1);
    }

    //action = new KAction(i18n("Clear &Marker Lines"), 0, KShortcut("Qt::CTRL+Qt::SHIFT+R"), m_viewContainer, SLOT(clearViewLines()),actionCollection(),"clear_lines");
    action=new KAction(this);
    action->setText(i18n("Clear &Marker Lines"));
    action->setShortcut(KShortcut("Qt::CTRL+Qt::SHIFT+R"));
    action->setEnabled(false);
    action->setToolTip(i18n("Clear marker lines in the current tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(clearViewLines()));
    actionCollection()->addAction("clear_lines", action);

    //action = new KAction(i18n("&Clear Window"), 0, KShortcut("Ctrl+L"), m_viewContainer, SLOT(clearView()),actionCollection(),"clear_window");
    action=new KAction(this);
    action->setText(i18n("&Clear Window"));
    action->setShortcut(KShortcut("Ctrl+L"));
    action->setEnabled(false);
    action->setToolTip(i18n("Clear the contents of the current tab"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(clearView()));
    actionCollection()->addAction("clear_window", action);

    //action = new KAction(i18n("Clear &All Windows"),0,KShortcut("Qt::CTRL+Qt::SHIFT+L"), m_viewContainer, SLOT(clearAllViews()),actionCollection(),"clear_tabs");
    action=new KAction(this);
    action->setText(i18n("Clear &All Windows"));
    action->setShortcut(KShortcut("Qt::CTRL+Qt::SHIFT+L"));
    action->setEnabled(false);
    action->setToolTip(i18n("Clear the contents of all open tabs"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(clearAllViews()));
    actionCollection()->addAction("clear_tabs", action);

    KToggleAction* awayAction = new KToggleAction(this);
    action->setText(i18n("Global Away"));
    action->setShortcut(KShortcut("Ctrl+Shift+A"));
    awayAction->setEnabled(false);
    connect(awayAction, SIGNAL(toggled(bool)), KonversationApplication::instance()->getAwayManager(), SLOT(toggleGlobalAway(bool)));
    actionCollection()->addAction("toggle_away", action);

    //action = new KAction(i18n("&Join Channel..."), "add", KShortcut("Ctrl+J"), m_viewContainer, SLOT(showJoinChannelDialog()), actionCollection(), "join_channel");
    action=new KAction(this);
    action->setText(i18n("&Join Channel..."));
    action->setIcon(KIcon("list-add"));
    action->setShortcut(KShortcut("Ctrl+J"));
    action->setEnabled(false);
    action->setToolTip("Join a new channel on this server");
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(showJoinChannelDialog()));
    actionCollection()->addAction("join_channel", action);

    action = KStandardAction::find(m_viewContainer, SLOT(findText()), actionCollection());
    action->setEnabled(false);
    action = KStandardAction::findNext(m_viewContainer, SLOT(findNextText()), actionCollection());
    action->setEnabled(false);
    action = KStandardAction::findPrev(m_viewContainer, SLOT(findPrevText()), actionCollection());
    action->setEnabled(false);

    //action = new KAction(i18n("&IRC Color..."), "colorize", Qt::CTRL+Qt::Key_K, m_viewContainer, SLOT(insertIRCColor()), actionCollection(), "irc_colors");
    action=new KAction(this);
    action->setText(i18n("&IRC Color..."));
    action->setIcon(KIcon("format-text-color"));
    action->setShortcut(KShortcut("Ctrl+K"));
    action->setEnabled(false);
    action->setToolTip(i18n("Set the color of your current IRC message"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(insertIRCColor()));
    actionCollection()->addAction("irc_colors", action);

    //action = new KAction(i18n("&Marker Line"), 0,  KShortcut("Ctrl+R") , m_viewContainer, SLOT(insertMarkerLine()), actionCollection(), "insert_marker_line");
    action=new KAction(this);
    action->setText(i18n("&Marker Line"));
    action->setShortcut(KShortcut("Ctrl+R"));
    action->setEnabled(false);
    action->setToolTip(i18n("Insert a horizontal line into the current tab that only you can see"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(insertMarkerLine()));
    actionCollection()->addAction("insert_marker_line", action);

    //action = new KAction(i18n("Special &Character..."), "char", KShortcut("Alt+Shift+C"), m_viewContainer, SLOT(insertCharacter()), actionCollection(), "insert_character");
    action=new KAction(this);
    action->setText(i18n("Special &Character..."));
    action->setIcon(KIcon("character-set"));
    action->setShortcut(KShortcut("Alt+Shift+C"));
    action->setEnabled(false);
    action->setToolTip(i18n("Insert any character into your current IRC message"));
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(insertCharacter()));
    actionCollection()->addAction("insert_character", action);

    //action = new KAction(i18n("Close &All Open Queries"), 0, KShortcut("F11"), m_viewContainer, SLOT(closeQueries()), actionCollection(), "close_queries");
    action=new KAction(this);
    action->setText(i18n("Close &All Open Queries"));
    action->setShortcut(KShortcut("F11"));
    action->setEnabled(false);
    connect(action, SIGNAL(triggered()), m_viewContainer, SLOT(closeQueries()));
    actionCollection()->addAction("close_queries", action);

#if 1==0
    action=new KAction(this);
    action->setText();
    action->setIcon(KIcon());
    action->setShortcut();
    action->setToolTip();
    connect(action, SIGNAL(triggered()), SLOT());
    actionCollection()->addAction(, action);
#endif

    //KToggleAction* toggleChannelNickListsAction = new KToggleAction(i18n("Hide Nicklist"), 0, KShortcut("Ctrl+H"), m_viewContainer, SLOT(toggleChannelNicklists()), actionCollection(), "hide_nicknamelist");
    KToggleAction* toggleChannelNickListsAction = new KToggleAction(this);
    if (!Preferences::showNickList())
        toggleChannelNickListsAction->setChecked(true);
    toggleChannelNickListsAction->setText(i18n("Hide Nicklist"));
    toggleChannelNickListsAction->setShortcut(KShortcut("Ctrl+H"));
    connect(toggleChannelNickListsAction, SIGNAL(triggered()), m_viewContainer, SLOT(toggleChannelNicklists()));
    actionCollection()->addAction("hide_nicknamelist", toggleChannelNickListsAction);

    // set up system tray
    m_trayIcon = new Konversation::TrayIcon(this);
    connect(this, SIGNAL(endNotification()), m_trayIcon, SLOT(endNotification()));
    connect(KGlobalSettings::self(), SIGNAL(iconChanged(int)), m_trayIcon, SLOT(updateAppearance()));
    connect(m_trayIcon, SIGNAL(quitSelected()), this, SLOT(quitProgram()));
    KMenu *trayMenu = qobject_cast<KMenu*>(m_trayIcon->contextMenu());
    #ifdef USE_KNOTIFY
    configureNotificationsAction->plug(trayMenu);
    #endif
    trayMenu->addAction(preferencesAction);
    trayMenu->addAction(awayAction);

    // decide whether to show the tray icon or not
    updateTrayIcon();

    createGUI();

    resize(700, 500);                             // Give the app a sane default size
    setAutoSaveSettings();

    // Apply menubar show/hide pref
    hideMenuBarAction->setChecked(Preferences::showMenuBar());
    toggleMenubar(true);

    // Bookmarks
    //m_bookmarkHandler = new KonviBookmarkHandler(this);

    // set up KABC with a nice gui error dialog
    KABC::GuiErrorHandler *m_guiErrorHandler = new KABC::GuiErrorHandler(this);
    //kapp->dcopClient()->setAcceptCalls( false );
    //Konversation::Addressbook::self()->getAddressBook()->setErrorHandler(m_guiErrorHandler);
    //kapp->dcopClient()->setAcceptCalls( true );

    if (Preferences::useNotify() && Preferences::openWatchedNicksAtStartup())
        m_viewContainer->openNicksOnlinePanel();

}

KonversationMainWindow::~KonversationMainWindow()
{
    delete m_viewContainer;
}

int KonversationMainWindow::confirmQuit()
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    if (konvApp->getConnectionManager()->connectionCount() == 0)
        return KMessageBox::Continue;

    int result = KMessageBox::Cancel;

    /*
    if (!KMessageBox::shouldBeShownContinue("systemtrayquitKonversation")
         && konvApp->getDccTransferManager()->hasActiveTransfers())
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("<qt>You have active DCC file transfers. Are you sure you want to quit <b>Konversation</b>?</qt>"),
            i18n("Confirm Quit"),
            KStandardGuiItem::quit(),
            KStandardGuiItem::cancel(),
            "QuitWithActiveDccTransfers");
    }
    else
    {
    */
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("<qt>Are you sure you want to quit <b>Konversation</b>?</qt>"),
            i18n("Confirm Quit"),
            KStandardGuiItem::quit(),
            KStandardGuiItem::cancel(),
            "systemtrayquitKonversation");
    //}

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
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    if (!konvApp->sessionSaving())
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
                i18n("<qt>This will hide the menu bar completely. You can show it again by typing %1.</qt>",accel),
                "Hide menu bar","HideMenuBarWarning");
        }
        menuBar()->hide();
    }

    Preferences::setShowMenuBar(hideMenuBarAction->isChecked());
}

void KonversationMainWindow::focusAndShowErrorMessage(const QString &errorMsg)
{
    show();
    KWindowSystem::demandAttention(winId());
    KWindowSystem::activateWindow(winId());
    KMessageBox::error(this, errorMsg);
}

void KonversationMainWindow::openPrefsDialog()
{/*
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
    m_settingsDialog->show();*/
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
    KShortcutsDialog::configure(actionCollection());

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
        KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

        connect(m_serverListDialog, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettingsPtr)),
                konvApp, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettingsPtr)));
        connect(konvApp, SIGNAL(serverGroupsChanged(const Konversation::ServerGroupSettingsPtr)),
                m_serverListDialog, SLOT(updateServerList()));
        connect(m_serverListDialog, SIGNAL(connectTo(Konversation::ConnectionFlag, int)),
                konvApp->getConnectionManager(), SLOT(connectTo(Konversation::ConnectionFlag, int)));
        connect(m_serverListDialog, SIGNAL(connectTo(Konversation::ConnectionFlag, ConnectionSettings&)),
                konvApp->getConnectionManager(), SLOT(connectTo(Konversation::ConnectionFlag, ConnectionSettings&)));
        connect(konvApp->getConnectionManager(), SIGNAL(closeServerList()), m_serverListDialog, SLOT(slotClose()));
    }

    m_serverListDialog->show();
}

void KonversationMainWindow::openQuickConnectDialog()
{
    emit showQuickConnectDialog();
}

// open the preferences dialog and show the watched nicknames page
void KonversationMainWindow::openNotify()
{/*
    openPrefsDialog();
    if (m_settingsDialog) m_settingsDialog->openWatchedNicknamesPage();*/
}

void KonversationMainWindow::openIdentitiesDialog()
{
    Konversation::IdentityDialog dlg(this);

    if (dlg.exec() == KDialog::Accepted)
    {
        if (m_serverListDialog)
            m_serverListDialog->updateServerList();
        m_viewContainer->updateViewEncoding(m_viewContainer->getFrontView());
    }
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
        return IdentityPtr();
}

void KonversationMainWindow::openNotifications()
{
    #ifdef USE_KNOTIFY
    (void) KNotifyConfigWidget::configure(this);
    #endif
}

void KonversationMainWindow::notifyAction(const QString& serverName, const QString& nick)
{
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
    Server* server = konvApp->getConnectionManager()->getServerByName(serverName);
    if (server) server->notifyAction(nick);
}

// TODO: Let an own class handle notify things
void KonversationMainWindow::setOnlineList(Server* notifyServer,const QStringList& /*list*/, bool /*changed*/)
{
    emit nicksNowOnline(notifyServer);
    // FIXME  if (changed && nicksOnlinePanel) newText(nicksOnlinePanel, QString::null, true);
}

QString KonversationMainWindow::currentURL(bool passNetwork)
{
    return m_viewContainer->currentViewURL(passNetwork);
}

QString KonversationMainWindow::currentTitle()
{
    return m_viewContainer->currentViewTitle();
}

// // #include "./mainwindow.moc"
