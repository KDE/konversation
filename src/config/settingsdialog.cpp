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
#include "dcc_config.h"
#include "osd_config.h"
#include "theme_config.h"
#include "alias_config.h"
#include "ignore_config.h"
#include "ui_watchednicknames_configui.h"
#include "ui_tabnotifications_config.h"

#include <config-konversation.h>

#include <KConfigDialog>
#include <QIcon>
#include <KLocalizedString>


KonviSettingsDialog::KonviSettingsDialog( QWidget *parent) :
    ConfigDialog( parent, QLatin1String("settings"), Preferences::self())
{
  setFaceType(KPageDialog::Tree);

  m_modified = false;

  KPageWidgetItem* interfaceGroup = new KPageWidgetItem(new QWidget(this), i18n("Interface"));
  interfaceGroup->setIcon(QIcon::fromTheme("preferences-desktop-theme"));
  KPageDialog::addPage(interfaceGroup);

  KPageWidgetItem* behaviorGroup = new KPageWidgetItem(new QWidget(this), i18n("Behavior"));
  behaviorGroup->setIcon(QIcon::fromTheme("configure"));
  KPageDialog::addPage(behaviorGroup);

  KPageWidgetItem* notificationGroup = new KPageWidgetItem(new QWidget(this), i18n("Notifications"));
  notificationGroup->setIcon(QIcon::fromTheme("preferences-desktop-notification"));
  KPageDialog::addPage(notificationGroup);

  QWidget *w = 0;

  //Interface/Chat Window
  Ui::ChatWindowAppearance_Config confChatWindowAppearance;
  w = new QWidget();
  confChatWindowAppearance.setupUi(w);
  confChatWindowAppearance.kcfg_TimestampFormat->addItem("hh:mm");
  confChatWindowAppearance.kcfg_TimestampFormat->addItem("hh:mm:ss");
  confChatWindowAppearance.kcfg_TimestampFormat->addItem("h:m ap");
  konviAddSubPage(interfaceGroup, w, i18n("Chat Window"), QLatin1String("view-list-text"));

  //Interface/Themes
  m_confThemeWdg = new Theme_Config( this, "Theme" );
  konviAddSubPage(interfaceGroup, m_confThemeWdg, i18n("Nicklist Themes"), QLatin1String("preferences-desktop-icons"));
  m_pages.append(m_confThemeWdg);
  connect(m_confThemeWdg, &Theme_Config::modified, this, &KonviSettingsDialog::modifiedSlot);

  //Interface/Colors
  Ui::ColorsAppearance_Config confColorsAppearance;
  w = new QWidget();
  confColorsAppearance.setupUi(w);
  konviAddSubPage(interfaceGroup, w, i18n("Colors"), QLatin1String("preferences-desktop-color"));

  //Interface/Fonts
  Ui::FontAppearance_Config confFontAppearance;
  w = new QWidget();
  confFontAppearance.setupUi(w);
  konviAddSubPage(interfaceGroup, w, i18n("Fonts"), QLatin1String("preferences-desktop-font"));

  //Interface/Quick Buttons
  m_confQuickButtonsWdg = new QuickButtons_Config( this, "QuickButtons" );
  konviAddSubPage(interfaceGroup, m_confQuickButtonsWdg, i18n("Quick Buttons"), QLatin1String("preferences-desktop-keyboard"));
  m_pages.append(m_confQuickButtonsWdg);
  connect(m_confQuickButtonsWdg, &QuickButtons_Config::modified, this, &KonviSettingsDialog::modifiedSlot);

  //Interface/Tabs
  m_confTabBarWdg = new Tabs_Config( this, "TabBar" );
  konviAddSubPage(interfaceGroup, m_confTabBarWdg, i18n("Tabs"), QLatin1String("tab-new"));

  //Behavior/General
  Ui::GeneralBehavior_ConfigUI confGeneralBehavior;
  w = new QWidget();
  confGeneralBehavior.setupUi(w);
  konviAddSubPage(behaviorGroup, w, i18n("General Behavior"), QLatin1String("configure"));

  //Behavior/Connection
  ConnectionBehavior_Config* confConnectionBehavior = new ConnectionBehavior_Config(this);
  confConnectionBehavior->setObjectName("ConnectionBehavior");
  konviAddSubPage(behaviorGroup, confConnectionBehavior, i18n("Connection"), QLatin1String("network-connect"));
  m_pages.append(confConnectionBehavior);

  //Behaviour/Chat Window
  Ui::ChatwindowBehaviour_Config confChatwindowBehaviour;
  w = new QWidget();
  confChatwindowBehaviour.setupUi(w);
  confChatwindowBehaviour.kcfg_ScrollbackMax->setSuffix(ki18np(" line", " lines"));
  confChatwindowBehaviour.kcfg_AutoWhoNicksLimit->setSuffix(ki18np(" nick", " nicks"));
  confChatwindowBehaviour.kcfg_AutoWhoContinuousInterval->setSuffix(ki18np(" second", " seconds"));
  konviAddSubPage(behaviorGroup, w, i18n("Chat Window"), QLatin1String("view-list-text"));

  //Behaviour/Nickname List
  m_confNicklistBehaviorWdg = new NicklistBehavior_Config( this, "NicklistBehavior" );
  konviAddSubPage(behaviorGroup, m_confNicklistBehaviorWdg, i18n("Nickname List"), QLatin1String("preferences-contact-list"));
  connect(m_confNicklistBehaviorWdg, &NicklistBehavior_Config::modified, this, &KonviSettingsDialog::modifiedSlot);
  m_pages.append(m_confNicklistBehaviorWdg);

  //Behaviour/Command Aliases
  m_confAliasWdg = new Alias_Config( this, "Alias" );
  konviAddSubPage(behaviorGroup, m_confAliasWdg, i18n("Command Aliases"), QLatin1String("edit-rename"));
  m_pages.append(m_confAliasWdg);
  connect(m_confAliasWdg, &Alias_Config::modified, this, &KonviSettingsDialog::modifiedSlot);

  //Behaviour/Auto Replace
  m_confAutoreplaceWdg = new Autoreplace_Config( this, "Autoreplace" );
  konviAddSubPage(behaviorGroup, m_confAutoreplaceWdg, i18n("Auto Replace"), QLatin1String("edit-rename"));
  m_pages.append(m_confAutoreplaceWdg);
  connect(m_confAutoreplaceWdg, &Autoreplace_Config::modified, this, &KonviSettingsDialog::modifiedSlot);

  //Behaviour/Ignore
  m_confIgnoreWdg = new Ignore_Config(this, "Ignore");
  konviAddSubPage(behaviorGroup, m_confIgnoreWdg, i18nc("@title:tab", "Ignore"), QLatin1String("process-stop"));
  connect(m_confIgnoreWdg, &Ignore_Config::modified, this, &KonviSettingsDialog::modifiedSlot);
  m_pages.append(m_confIgnoreWdg);

  //Behaviour/Logging
  Ui::Log_Config confLog;
  w = new QWidget();
  confLog.setupUi(w);
  confLog.kcfg_LogfilePath->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
  konviAddSubPage(behaviorGroup, w, i18n("Logging"), QLatin1String("text-plain"));

  //DCC
  m_confDCCWdg = new DCC_Config(this, "DCC");
  konviAddSubPage(behaviorGroup, m_confDCCWdg, i18nc("@title:tab", "DCC"), QLatin1String("arrow-right-double"));

  //Notifications/Tab Bar
  Ui::TabNotifications_Config confTabNotifications;
  w = new QWidget();
  confTabNotifications.setupUi(w);
  konviAddSubPage(notificationGroup, w, i18n("Tabs"), QLatin1String("tab-new"));

  //Notification/Highlighting
  m_confHighlightWdg = new Highlight_Config( this, "Highlight" );
  konviAddSubPage(notificationGroup, m_confHighlightWdg, i18n("Highlight"), QLatin1String("flag-red"));
  connect(m_confHighlightWdg, &Highlight_Config::modified, this, &KonviSettingsDialog::modifiedSlot);
  m_pages.append(m_confHighlightWdg);

  //Notification/Watched Nicknames
  Ui::WatchedNicknames_ConfigUI confWatchedNicks;
  w = new QWidget();
  confWatchedNicks.setupUi(w);
  konviAddSubPage(notificationGroup, w, i18n("Watched Nicknames"), QLatin1String("edit-find-user"));

  //Notification/On Screen Display
  m_confOSDWdg = new OSD_Config( this, "OSD" );
  konviAddSubPage(notificationGroup, m_confOSDWdg, i18n("On Screen Display"), QLatin1String("video-display"));
  //no modified connection needed - it's all kcfg widgets
  m_pages.append(m_confOSDWdg);

  //Notification/Warning Dialogs
  m_confWarningsWdg = new Warnings_Config( this, "Warnings" );
  konviAddSubPage(notificationGroup, m_confWarningsWdg, i18n("Warning Dialogs"), QLatin1String("dialog-warning"));
  m_pages.append(m_confWarningsWdg);
  connect(m_confWarningsWdg, &Warnings_Config::modified, this, &KonviSettingsDialog::modifiedSlot);
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



