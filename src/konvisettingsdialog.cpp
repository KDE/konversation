/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Benjamin C Meyer (ben+kdelibs at meyerhome dot net)
 *  Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2004 Michael Brade <brade@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

/*
 *  KConfigDialog derivative allowing for a multi-level hierarchical TreeList.
 *  Differences from KConfigDialog:
 *  - Use QStringList instead of QString for the item name(s) in addPage and
 *    addPageInternal, thus calling the respective KDialogBase methods which
 *    allow specifying a path from which the TreeList hierarchy is constructed.
 *  - Use 16x16 icons in the TreeList.
 *  See the KConfigDialog reference for detailed documentation.
 *
 *  begin:     Nov 22 2005
 *  copyright: (C) 2005 by Eike Hein, KConfigDialog developers
 *  email:     sho@eikehein.com
 */

#include "konvisettingsdialog.h"
#include "konviconfigdialog.h"
#include "config/preferences.h"

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

#include "chatwindowappearance_preferences.h"
#include "connectionbehavior_preferences.h"
#include "highlight_preferences.h"
#include "warnings_preferences.h"
#include "chatwindowappearance_preferences.h"
#include "log_preferences.h"
#include "quickbuttons_preferences.h"
#include "watchednicknames_preferences.h"
#include "chatwindowbehaviour_preferences.h"
#include "fontappearance_preferences.h"
#include "nicklistbehavior_preferences.h"
#include "tabbar_preferences.h"
#include "colorsappearance_preferences.h"
#include "generalbehavior_preferences.h"
#include "ex_dcc_preferences.h"
#include "ex_osd_preferences.h"
#include "ex_theme_preferences.h"
#include "ex_alias_preferences.h"

KonviSettingsDialog::KonviSettingsDialog( QWidget *parent) :
	           KonviConfigDialog( parent, "settings", Preferences::self(), KDialogBase::TreeList)
{
  m_modified = false;
  setShowIconsInTreeList(true); 

  QStringList iconPath;

  iconPath << i18n("Appearance");
  setFolderIcon( iconPath, SmallIcon("looknfeel") );

  iconPath.clear();
  iconPath << i18n("Behavior");
  setFolderIcon( iconPath, SmallIcon("configure") );

  iconPath.clear();
  iconPath<< i18n("Behavior");
  setFolderIcon( iconPath, SmallIcon("configure") );

  iconPath.clear();
  iconPath<< i18n("Notifications");
  setFolderIcon( iconPath, SmallIcon("playsound") );

  QStringList pagePath;

  //Appearance/Chat Window
  ChatWindowAppearance_Config* confChatWindowAppearanceWdg = new ChatWindowAppearance_Config( 0, "ChatWindowAppearance" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Chat Window");
  addPage ( confChatWindowAppearanceWdg, pagePath, "view_text", i18n("Chat Window") );

  //Appearance/Fonts
  FontAppearance_Config* confFontAppearanceWdg = new FontAppearance_Config( this, "FontAppearance" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Fonts");
  addPage ( confFontAppearanceWdg, pagePath, "fonts", i18n("Fonts") );

  //Appearance/Themes
  Theme_Config_Ext* confThemeWdg = new Theme_Config_Ext( this, "Theme" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Themes");
  addPage ( confThemeWdg, pagePath, "iconthemes", i18n("Themes") );

  //Appearance/Colors
  ColorsAppearance_Config* confColorsAppearanceWdg = new ColorsAppearance_Config( this, "ColorsAppearance" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Colors");
  addPage ( confColorsAppearanceWdg, pagePath, "colorize", i18n("Colors") );

  //Behavior/General
  GeneralBehavior_Config* confGeneralBehaviorWdg = new GeneralBehavior_Config( this, "GeneralBehavior" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("General");
  addPage ( confGeneralBehaviorWdg, pagePath, "exec", i18n("General") );

  //Behavior/Connection
  ConnectionBehavior_Config* confConnectionBehaviorWdg = new ConnectionBehavior_Config( this, "ConnectionBehavior" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Connection");
  addPage ( confConnectionBehaviorWdg, pagePath, "connect_creating", i18n("Connection") );

  //Behaviour/Chat Window
  ChatwindowBehaviour_Config* confChatwindowBehaviourWdg = new ChatwindowBehaviour_Config( this, "ChatwindowBehaviour" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Chat Window");
  addPage ( confChatwindowBehaviourWdg, pagePath, "view_text", i18n("Chat Window") );

  //Behaviour/Nickname List
  NicklistBehavior_Config* confNicklistBehaviorWdg = new NicklistBehavior_Config( this, "NicklistBehavior" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Nickname List");
  addPage ( confNicklistBehaviorWdg, pagePath, "player_playlist", i18n("Nickname List") );

  //Behaviour/Tab Bar
  TabBar_Config* confTabBarWdg = new TabBar_Config( this, "TabBar" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Tab Bar");
  addPage ( confTabBarWdg, pagePath, "tab_new", i18n("Tab Bar") );

  //Behaviour/Command Aliases
  Alias_Config_Ext* confAliasWdg = new Alias_Config_Ext( this, "Alias" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Command Aliases");
  addPage ( confAliasWdg, pagePath, "editcopy", i18n(" Command Aliases") );

  //Behaviour/Quick Buttons
  QuickButtons_Config* confQuickButtonsWdg = new QuickButtons_Config( this, "QuickButtons" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Quick Buttons");
  addPage ( confQuickButtonsWdg, pagePath, "keyboard", i18n("Quick Buttons") );

  //Behaviour/Logging
  Log_Config* confLogWdg = new Log_Config( this, "Log" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Logging");
  addPage ( confLogWdg, pagePath, "log", i18n("Logging") );

  DCC_Config_Ext* confDCCWdg = new DCC_Config_Ext( this, "DCC" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("DCC");
  addPage ( confDCCWdg, pagePath, "2rightarrow", i18n("DCC") );

  //Notification/Watched Nicknames
  WatchedNicknames_Config* confWatchedNicknamesWdg = new WatchedNicknames_Config( this, "WatchedNicknames" );
  pagePath.clear();
  pagePath << i18n("Notifications") << i18n("Watched Nicknames");
  addPage ( confWatchedNicknamesWdg, pagePath, "kfind", i18n("Watched Nicknames") );

  //Notification/Highlighting
  Highlight_Config* confHighlightWdg = new Highlight_Config( this, "Highlight" );
  pagePath.clear();
  pagePath << i18n("Notifications") << i18n("Highlight");
  addPage ( confHighlightWdg, pagePath, "paintbrush", i18n("Highlight") );

  //Notification/On Screen Display
  OSD_Config_Ext* confOSDWdg = new OSD_Config_Ext( this, "OSD" );
  pagePath.clear();
  pagePath << i18n("Notifications") << i18n("On Screen Display");
  addPage ( confOSDWdg, pagePath, "tv", i18n("On Screen Display") );

  //Warning Dialogs
  m_confWarningsWdg = new Warnings_Config( this, "Warnings" );
  pagePath.clear();
  pagePath << i18n("Warning Dialogs");
  addPage ( m_confWarningsWdg, i18n("Warning Dialogs"), "messagebox_warning", i18n("Warning Dialogs") );
  connect(m_confWarningsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  unfoldTreeList();
}

void KonviSettingsDialog::modifiedSlot()
{
  m_modified = true;
  updateButtons();
}
KonviSettingsDialog::~KonviSettingsDialog()
{
}
void KonviSettingsDialog::updateSettings()
{
  m_confWarningsWdg->saveSettings();
  m_modified = false;
}

void KonviSettingsDialog::updateWidgets()
{
  m_confWarningsWdg->updateWidgets();
}

void KonviSettingsDialog::updateWidgetsDefault()
{
}

#include "konvisettingsdialog.moc"

