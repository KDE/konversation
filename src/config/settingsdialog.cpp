/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include "settingsdialog.h"
#include "preferences.h"
#include "ui_chatwindowappearance_config.h"
#include "connectionbehavior_config.h"
#include "highlight_config.h"
#include "warnings_config.h"
#include "ui_log_config.h"
#include "quickbuttons_config.h"
#include "autoreplace_config.h"
#include "ui_chatwindowbehaviour_config.h"
#include "ui_fontappearance_config.h"
#include "nicklistbehavior_config.h"
#include "tabs_config.h"
#include "ui_colorsappearance_config.h"
#include "ui_generalbehavior_configui.h"
// #include "dcc_config.h" FIXME KF5 port
#include "osd_config.h"
#include "theme_config.h"
#include "alias_config.h"
#include "ignore_config.h"
#include "ui_watchednicknames_configui.h"
#include "ui_tabnotifications_config.h"

#include <config-konversation.h>

#include <KConfigDialog>
#include <KIcon>
#include <KLocalizedString>


KonviSettingsDialog::KonviSettingsDialog( QWidget *parent) :
    KConfigDialog( parent, QLatin1String("settings"), Preferences::self())
{
  m_modified = false;
  QWidget *w = 0;

  /* FIXME KF5 port
  KPageWidgetItem *interfaceGroup = new KPageWidgetItem(new QWidget(this), i18n("Interface"));
  interfaceGroup->setIcon(KIcon("preferences-desktop-theme"));
  KPageDialog::addPage(interfaceGroup);

  KPageWidgetItem *behaviorGroup = new KPageWidgetItem(new QWidget(this), i18n("Behavior"));
  behaviorGroup->setIcon(KIcon("configure"));
  KPageDialog::addPage(behaviorGroup);

  KPageWidgetItem *notificationGroup = new KPageWidgetItem(new QWidget(this), i18n("Notifications"));
  notificationGroup->setIcon(KIcon("preferences-desktop-notification"));
  KPageDialog::addPage(notificationGroup);
  */

  //Interface/Chat Window
  Ui::ChatWindowAppearance_Config confChatWindowAppearance;
  w = new QWidget();
  confChatWindowAppearance.setupUi(w);
  confChatWindowAppearance.kcfg_TimestampFormat->addItem("hh:mm");
  confChatWindowAppearance.kcfg_TimestampFormat->addItem("hh:mm:ss");
  confChatWindowAppearance.kcfg_TimestampFormat->addItem("h:m ap");
  addPage(w, i18n("Chat Window (Interface)"), QLatin1String("view-list-text"));

  //Interface/Themes
  m_confThemeWdg = new Theme_Config( this, "Theme" );
  addPage(m_confThemeWdg, i18n("Nicklist Themes"), QLatin1String("preferences-desktop-icons"));
  m_pages.append(m_confThemeWdg);
  connect(m_confThemeWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Interface/Colors
  Ui::ColorsAppearance_Config confColorsAppearance;
  w = new QWidget();
  confColorsAppearance.setupUi(w);
  addPage(w, i18n("Colors"), QLatin1String("preferences-desktop-color"));

  //Interface/Fonts
  Ui::FontAppearance_Config confFontAppearance;
  w = new QWidget();
  confFontAppearance.setupUi(w);
  addPage(w, i18n("Fonts"), QLatin1String("preferences-desktop-font"));

  //Interface/Quick Buttons
  m_confQuickButtonsWdg = new QuickButtons_Config( this, "QuickButtons" );
  addPage(m_confQuickButtonsWdg, i18n("Quick Buttons"), QLatin1String("preferences-desktop-keyboard"));
  m_pages.append(m_confQuickButtonsWdg);
  connect(m_confQuickButtonsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Interface/Tabs
  m_confTabBarWdg = new Tabs_Config( this, "TabBar" );
  addPage ( m_confTabBarWdg, i18n("Tabs (Interface)"), QLatin1String("tab-new"));

  //Behavior/General
  Ui::GeneralBehavior_ConfigUI confGeneralBehavior;
  w = new QWidget();
  confGeneralBehavior.setupUi(w);
  addPage(w, i18n("General Behavior"), QLatin1String("configure"));

  //Behavior/Connection
  ConnectionBehavior_Config* confConnectionBehavior = new ConnectionBehavior_Config(this);
  confConnectionBehavior->setObjectName("ConnectionBehavior");
  addPage(confConnectionBehavior, i18n("Connection"), QLatin1String("network-connect"));
  m_pages.append(confConnectionBehavior);

  //Behaviour/Chat Window
  Ui::ChatwindowBehaviour_Config confChatwindowBehaviour;
  w = new QWidget();
  confChatwindowBehaviour.setupUi(w);
  confChatwindowBehaviour.kcfg_ScrollbackMax->setSuffix(ki18np(" line", " lines"));
  confChatwindowBehaviour.kcfg_AutoWhoNicksLimit->setSuffix(ki18np(" nick", " nicks"));
  confChatwindowBehaviour.kcfg_AutoWhoContinuousInterval->setSuffix(ki18np(" second", " seconds"));
  addPage(w, i18n("Chat Window (Behavior)"), QLatin1String("view-list-text"));

  //Behaviour/Nickname List
  m_confNicklistBehaviorWdg = new NicklistBehavior_Config( this, "NicklistBehavior" );
  addPage ( m_confNicklistBehaviorWdg, i18n("Nickname List"), QLatin1String("preferences-contact-list"));
  connect(m_confNicklistBehaviorWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_pages.append(m_confNicklistBehaviorWdg);

  //Behaviour/Command Aliases
  m_confAliasWdg = new Alias_Config( this, "Alias" );
  addPage(m_confAliasWdg, i18n("Command Aliases"), QLatin1String("edit-rename"));
  m_pages.append(m_confAliasWdg);
  connect(m_confAliasWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Behaviour/Auto Replace
  m_confAutoreplaceWdg = new Autoreplace_Config( this, "Autoreplace" );
  addPage (m_confAutoreplaceWdg, i18n("Auto Replace"), QLatin1String("edit-rename"));
  m_pages.append(m_confAutoreplaceWdg);
  connect(m_confAutoreplaceWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Behaviour/Ignore
  m_confIgnoreWdg = new Ignore_Config(this, "Ignore");
  addPage ( m_confIgnoreWdg, i18nc("@title:tab", "Ignore"), QLatin1String("process-stop"));
  connect(m_confIgnoreWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_pages.append(m_confIgnoreWdg);

  //Behaviour/Logging
  Ui::Log_Config confLog;
  w = new QWidget();
  confLog.setupUi(w);
  confLog.kcfg_LogfilePath->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
  addPage(w, i18n("Logging"), QLatin1String("text-plain"));

  /* FIXME KF5 port
  m_confDCCWdg = new DCC_Config( this, "DCC" );
  addPage ( m_confDCCWdg, behaviorGroup, QLatin1String("arrow-right-double"), i18n("DCC") );
  */

  //Notifications/Tab Bar
  Ui::TabNotifications_Config confTabNotifications;
  w = new QWidget();
  confTabNotifications.setupUi(w);
  addPage(w, i18n("Tabs (Notifications)"), QLatin1String("tab-new"));

  //Notification/Highlighting
  m_confHighlightWdg = new Highlight_Config( this, "Highlight" );
  addPage ( m_confHighlightWdg, i18n("Highlight"), QLatin1String("flag-red"));
  connect(m_confHighlightWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_pages.append(m_confHighlightWdg);

  //Notification/Watched Nicknames
  Ui::WatchedNicknames_ConfigUI confWatchedNicks;
  w = new QWidget();
  confWatchedNicks.setupUi(w);
  addPage(w, i18n("Watched Nicknames"), QLatin1String("edit-find-user"));

  //Notification/On Screen Display
  m_confOSDWdg = new OSD_Config( this, "OSD" );
  addPage(m_confOSDWdg, i18n("On Screen Display"), QLatin1String("video-display"));
  //no modified connection needed - it's all kcfg widgets
  m_pages.append(m_confOSDWdg);

  //Notification/Warning Dialogs
  m_confWarningsWdg = new Warnings_Config( this, "Warnings" );
  addPage(m_confWarningsWdg, i18n("Warning Dialogs"), QLatin1String("dialog-warning"));
  m_pages.append(m_confWarningsWdg);
  connect(m_confWarningsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
}

void KonviSettingsDialog::modifiedSlot()
{
  // this is for the non KConfigXT parts to tell us, if the user actually changed
  // something or went back to the old settings
// qDebug();
  m_modified = false;
  foreach (KonviSettingsPage *page, m_pages)
  {
    if (page->hasChanged())
    {
      m_modified = true;
//      qDebug() << "modified!";
      break;
    }
  }
  updateButtons();
}

KonviSettingsDialog::~KonviSettingsDialog()
{
}

void KonviSettingsDialog::updateSettings()
{
  foreach (KonviSettingsPage *page, m_pages)
  {
    // this is for the non KConfigXT parts to update the UI (like quick buttons)
    page->saveSettings();
  }
  m_modified = false;
  emit settingsChanged(QLatin1String("settings"));
}

void KonviSettingsDialog::updateWidgets()
{
  foreach (KonviSettingsPage *page, m_pages)
  {
    page->loadSettings();
  }
  m_modified = false;
}

void KonviSettingsDialog::updateWidgetsDefault()
{
  foreach (KonviSettingsPage *page, m_pages)
  {
    page->restorePageToDefaults();
  }
  m_modified = true;
}

// accessor method - will be used by KonviConfigDialog::updateButtons()
bool KonviSettingsDialog::hasChanged()
{
  return m_modified;
}

// accessor method - will be used by KonviConfigDialog::updateButtons()
bool KonviSettingsDialog::isDefault()
{
  return true;
}

#include "settingsdialog.moc"

