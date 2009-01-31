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

#include "settingsdialog.h" ////// header renamed
#include "configdialog.h" ////// header renamed
#include "config/preferences.h"
#include "chatwindowappearance_preferences.h"
#include "connectionbehavior_preferences.h"
#include "highlight_preferences.h"
#include "warnings_preferences.h"
#include "log_preferences.h"
#include "quickbuttons_preferences.h"
#include "autoreplace_preferences.h"
#include "chatwindowbehaviour_preferences.h"
#include "fontappearance_preferences.h"
#include "nicklistbehavior_preferences.h"
#include "tabs_preferences.h"
#include "colorsappearance_preferences.h"
#include "generalbehavior_preferences.h"
#include "dcc_preferences.h"
#include "osd_preferences.h"
#include "theme_preferences.h"
#include "alias_preferences.h"
#include "ignore_preferences.h"
#include "watchednicknames_preferences.h"
#include "tabnotifications_preferences.h"

#include <qsplitter.h>
#include <qcombobox.h>

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

KonviSettingsDialog::KonviSettingsDialog( QWidget *parent) :
    KonviConfigDialog( parent, "settings", Preferences::self(), KPageDialog::Tree)
{
  m_modified = false;

  KPageWidgetItem *interfaceGroup = new KPageWidgetItem(new QWidget(this), i18n("Interface"));
  interfaceGroup->setIcon(KIcon("preferences-desktop-theme"));
  KPageDialog::addPage(interfaceGroup);

  KPageWidgetItem *behaviorGroup = new KPageWidgetItem(new QWidget(this), i18n("Behavior"));
  behaviorGroup->setIcon(KIcon("configure"));
  KPageDialog::addPage(behaviorGroup);

  KPageWidgetItem *notificationGroup = new KPageWidgetItem(new QWidget(this), i18n("Notifications"));
  notificationGroup->setIcon(KIcon("preferences-desktop-notification"));
  KPageDialog::addPage(notificationGroup);

  //Interface/Chat Window
  m_confChatWindowAppearanceWdg = new ChatWindowAppearance_Config( this, "ChatWindowAppearance" );
  m_confChatWindowAppearanceWdg->kcfg_TimestampFormat->insertItem("hh:mm");
  m_confChatWindowAppearanceWdg->kcfg_TimestampFormat->insertItem("hh:mm:ss");
  m_confChatWindowAppearanceWdg->kcfg_TimestampFormat->insertItem("h:m ap");
  addPage ( m_confChatWindowAppearanceWdg, interfaceGroup, "view-list-text", i18n("Chat Window") );

  //Interface/Themes
  m_confThemeWdg = new Theme_Config( this, "Theme" );
  addPage ( m_confThemeWdg, interfaceGroup, "preferences-desktop-icons", i18n("Nicklist Themes") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confThemeWdg);
  connect(m_confThemeWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Interface/Colors
  m_confColorsAppearanceWdg = new ColorsAppearance_Config( this, "ColorsAppearance" );
  addPage ( m_confColorsAppearanceWdg, interfaceGroup, "preferences-desktop-color", i18n("Colors") );

  //Interface/Fonts
  m_confFontAppearanceWdg = new FontAppearance_Config( this, "FontAppearance" );
  addPage ( m_confFontAppearanceWdg, interfaceGroup, "preferences-desktop-font", i18n("Fonts") );

  //Interface/Quick Buttons
  m_confQuickButtonsWdg = new QuickButtons_Config( this, "QuickButtons" );
  addPage ( m_confQuickButtonsWdg, interfaceGroup, "preferences-desktop-keyboard", i18n("Quick Buttons") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confQuickButtonsWdg);
  connect(m_confQuickButtonsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Interface/Tabs
  m_confTabBarWdg = new Tabs_Config( this, "TabBar" );
  addPage ( m_confTabBarWdg, interfaceGroup, "tab-new", i18n("Tabs") );

  //Behavior/General
  m_confGeneralBehaviorWdg = new GeneralBehavior_Config( this, "GeneralBehavior" );
  addPage ( m_confGeneralBehaviorWdg, interfaceGroup, "configure", i18n("General") );

  //Behavior/Connection
  m_confConnectionBehaviorWdg = new ConnectionBehavior_Config( this, "ConnectionBehavior" );
  addPage ( m_confConnectionBehaviorWdg, interfaceGroup, "network-connect", i18n("Connection") );

  //Behaviour/Chat Window
  m_confChatwindowBehaviourWdg = new ChatwindowBehaviour_Config( this, "ChatwindowBehaviour" );
  addPage ( m_confChatwindowBehaviourWdg, behaviorGroup, "view-list-text", i18n("Chat Window") );

  //Behaviour/Nickname List
  m_confNicklistBehaviorWdg = new NicklistBehavior_Config( this, "NicklistBehavior" );
  addPage ( m_confNicklistBehaviorWdg, behaviorGroup, "preferences-contact-list", i18n("Nickname List") );
  connect(m_confNicklistBehaviorWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confNicklistBehaviorWdg);

  //Behaviour/Command Aliases
  m_confAliasWdg = new Alias_Config( this, "Alias" );
  addPage ( m_confAliasWdg, behaviorGroup, "edit-rename", i18n("Command Aliases") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confAliasWdg);
  connect(m_confAliasWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Behaviour/Auto Replace
  m_confAutoreplaceWdg = new Autoreplace_Config( this, "Autoreplace" );
  addPage ( m_confAutoreplaceWdg, behaviorGroup, "kview", i18n("Auto Replace") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confAutoreplaceWdg);
  connect(m_confAutoreplaceWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Behaviour/Ignore
  m_confIgnoreWdg = new Ignore_Config(this, "Ignore");
  addPage ( m_confIgnoreWdg, behaviorGroup, "process-stop", i18n("Ignore") );
  connect(m_confIgnoreWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confIgnoreWdg);

  //Behaviour/Logging
  m_confLogWdg = new Log_Config( this, "Log" );
  addPage ( m_confLogWdg, behaviorGroup, "log", i18n("Logging") );

  m_confDCCWdg = new DCC_Config( this, "DCC" );
  addPage ( m_confDCCWdg, behaviorGroup, "arrow-right-double", i18n("DCC") );

  //Notifications/Tab Bar
  m_confTabNotificationsWdg = new TabNotifications_Config( this, "TabBar" );
  addPage ( m_confTabNotificationsWdg, notificationGroup, "tab-new", i18n("Tabs") );

  //Notification/Highlighting
  m_confHighlightWdg = new Highlight_Config( this, "Highlight" );
  addPage ( m_confHighlightWdg, notificationGroup, "paintbrush", i18n("Highlight") );
  connect(m_confHighlightWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confHighlightWdg);

  //Notification/Watched Nicknames
  m_confWatchedNicknamesWdg = new WatchedNicknames_Config( this, "WatchedNicknames" );
  // remember index so we can open this page later from outside
  m_watchedNicknamesPage = addPage ( m_confWatchedNicknamesWdg, notificationGroup, "edit-find-user", i18n("Watched Nicknames") );
  connect(m_confWatchedNicknamesWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confWatchedNicknamesWdg);

  //Notification/On Screen Display
  m_confOSDWdg = new OSD_Config( this, "OSD" );
  addPage ( m_confOSDWdg, notificationGroup, "video-display", i18n("On Screen Display") );
  //no modified connection needed - it's all kcfg widgets
  m_indexToPageMapping.insert(lastAddedIndex(), m_confOSDWdg);

  //Notification/Warning Dialogs
  m_confWarningsWdg = new Warnings_Config( this, "Warnings" );
  addPage ( m_confWarningsWdg, notificationGroup, "dialog-warning", i18n("Warning Dialogs") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confWarningsWdg);
  connect(m_confWarningsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
}

void KonviSettingsDialog::modifiedSlot()
{
  // this is for the non KConfigXT parts to tell us, if the user actually changed
  // something or went back to the old settings
// kDebug() << "KonviSettingsDialog::modifiedSlot()" << endl;
  m_modified = false;
  Q3IntDictIterator<KonviSettingsPage> it( m_indexToPageMapping );
  for ( ; it.current(); ++it )
  {
    if ( (*it).hasChanged() )
    {
      m_modified = true;
//      kDebug() << "KonviSettingsDialog::modifiedSlot(): modified!" << endl;
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
  Q3IntDictIterator<KonviSettingsPage> it( m_indexToPageMapping );
  for ( ; it.current(); ++it )
  {
    // this is for the non KConfigXT parts to update the UI (like quick buttons)
    (*it).saveSettings();
  }
  m_modified = false;
  emit settingsChanged();
}

void KonviSettingsDialog::updateWidgets()
{
  Q3IntDictIterator<KonviSettingsPage> it( m_indexToPageMapping );
  for ( ; it.current(); ++it )
  {
    (*it).loadSettings();
  }
  m_modified = false;
}

void KonviSettingsDialog::updateWidgetsDefault()
{
  Q3IntDictIterator<KonviSettingsPage> it( m_indexToPageMapping );
  for ( ; it.current(); ++it )
  {
    (*it).restorePageToDefaults();
  }
  m_modified = true;
}

void KonviSettingsDialog::openWatchedNicknamesPage()
{
  // page index has been calculated in the constructor
  setCurrentPage(m_watchedNicknamesPage);
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

// #include "./settingsdialog.moc"

