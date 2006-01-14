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
#include "ignore_preferences.h"
#include "watchednicknames_preferences.h"

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
  m_confChatWindowAppearanceWdg = new ChatWindowAppearance_Config( 0, "ChatWindowAppearance" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Chat Window");
  addPage ( m_confChatWindowAppearanceWdg, pagePath, "view_text", i18n("Chat Window") );

  //Appearance/Fonts
  m_confFontAppearanceWdg = new FontAppearance_Config( this, "FontAppearance" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Fonts");
  addPage ( m_confFontAppearanceWdg, pagePath, "fonts", i18n("Fonts") );

  //Appearance/Themes
  m_confThemeWdg = new Theme_Config_Ext( this, "Theme" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Themes");
  addPage ( m_confThemeWdg, pagePath, "iconthemes", i18n("Themes") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confThemeWdg);

  //Appearance/Colors
  m_confColorsAppearanceWdg = new ColorsAppearance_Config( this, "ColorsAppearance" );
  pagePath.clear();
  pagePath << i18n("Appearance") << i18n("Colors");
  addPage ( m_confColorsAppearanceWdg, pagePath, "colorize", i18n("Colors") );

  //Behavior/General
  m_confGeneralBehaviorWdg = new GeneralBehavior_Config( this, "GeneralBehavior" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("General");
  addPage ( m_confGeneralBehaviorWdg, pagePath, "exec", i18n("General") );

  //Behavior/Connection
  m_confConnectionBehaviorWdg = new ConnectionBehavior_Config( this, "ConnectionBehavior" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Connection");
  addPage ( m_confConnectionBehaviorWdg, pagePath, "connect_creating", i18n("Connection") );

  //Behaviour/Chat Window
  m_confChatwindowBehaviourWdg = new ChatwindowBehaviour_Config( this, "ChatwindowBehaviour" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Chat Window");
  addPage ( m_confChatwindowBehaviourWdg, pagePath, "view_text", i18n("Chat Window") );

  //Behaviour/Nickname List
  m_confNicklistBehaviorWdg = new NicklistBehavior_Config( this, "NicklistBehavior" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Nickname List");
  addPage ( m_confNicklistBehaviorWdg, pagePath, "player_playlist", i18n("Nickname List") );
  connect(m_confNicklistBehaviorWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confNicklistBehaviorWdg);


  //Behaviour/Tab Bar
  m_confTabBarWdg = new TabBar_Config( this, "TabBar" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Tab Bar");
  addPage ( m_confTabBarWdg, pagePath, "tab_new", i18n("Tab Bar") );

  //Behaviour/Command Aliases
  m_confAliasWdg = new Alias_Config_Ext( this, "Alias" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Command Aliases");
  addPage ( m_confAliasWdg, pagePath, "editcopy", i18n(" Command Aliases") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confAliasWdg);
  connect(m_confAliasWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Behaviour/Quick Buttons
  m_confQuickButtonsWdg = new QuickButtons_Config( this, "QuickButtons" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Quick Buttons");
  addPage ( m_confQuickButtonsWdg, pagePath, "keyboard", i18n("Quick Buttons") );
  m_indexToPageMapping.insert(lastAddedIndex(), m_confQuickButtonsWdg);
  connect(m_confQuickButtonsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));

  //Behaviour/Ignore
  m_confIgnoreWdg = new Ignore_Config(this, "Ignore");
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Ignore");
  addPage ( m_confIgnoreWdg, pagePath, "ignore", i18n("Ignore") );
  connect(m_confIgnoreWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confIgnoreWdg);

  //Behaviour/Logging
  m_confLogWdg = new Log_Config( this, "Log" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("Logging");
  addPage ( m_confLogWdg, pagePath, "log", i18n("Logging") );

  m_confDCCWdg = new DCC_Config_Ext( this, "DCC" );
  pagePath.clear();
  pagePath << i18n("Behavior") << i18n("DCC");
  addPage ( m_confDCCWdg, pagePath, "2rightarrow", i18n("DCC") );

  //Notification/Watched Nicknames
  m_confWatchedNicknamesWdg = new WatchedNicknames_Config( this, "WatchedNicknames" );
  pagePath.clear();
  pagePath << i18n("Notifications") << i18n("Watched Nicknames");
  addPage ( m_confWatchedNicknamesWdg, pagePath, "kfind", i18n("Watched Nicknames") );
  // remember index so we can open this page later from outside
  m_watchedNicknamesIndex=lastAddedIndex();
  connect(m_confWatchedNicknamesWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confWatchedNicknamesWdg);


  //Notification/Highlighting
  m_confHighlightWdg = new Highlight_Config( this, "Highlight" );
  pagePath.clear();
  pagePath << i18n("Notifications") << i18n("Highlight");
  addPage ( m_confHighlightWdg, pagePath, "paintbrush", i18n("Highlight") );
  connect(m_confHighlightWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confHighlightWdg);


  //Notification/On Screen Display
  m_confOSDWdg = new OSD_Config_Ext( this, "OSD" );
  pagePath.clear();
  pagePath << i18n("Notifications") << i18n("On Screen Display");
  addPage ( m_confOSDWdg, pagePath, "tv", i18n("On Screen Display") );
  //no modified connection needed - it's all kcfg widgets
  m_indexToPageMapping.insert(lastAddedIndex(), m_confOSDWdg);


  //Warning Dialogs
  m_confWarningsWdg = new Warnings_Config( this, "Warnings" );
  pagePath.clear();
  pagePath << i18n("Warning Dialogs");
  addPage ( m_confWarningsWdg, i18n("Warning Dialogs"), "messagebox_warning", i18n("Warning Dialogs") );
  connect(m_confWarningsWdg, SIGNAL(modified()), this, SLOT(modifiedSlot()));
  m_indexToPageMapping.insert(lastAddedIndex(), m_confWarningsWdg);

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
  QIntDictIterator<KonviSettingsPage> it( m_indexToPageMapping );
  for ( ; it.current(); ++it )
  {
    (*it).saveSettings();
  }
  m_modified = false;
  // this is for the non KConfigXT parts to update the UI (like quick buttons)
  emit settingsChanged();
}

void KonviSettingsDialog::updateWidgets()
{
  QIntDictIterator<KonviSettingsPage> it( m_indexToPageMapping );
  for ( ; it.current(); ++it )
  {
    (*it).loadSettings();
  }
}

void KonviSettingsDialog::updateWidgetsDefault()
{
  KonviSettingsPage *page = m_indexToPageMapping.find(activePageIndex());
  if(page) {
    kdDebug() << "Setting defaults" << endl;
    page->restorePageToDefaults();
  } else {
    kdDebug() << "THIS PAGE HAS NO FUNCTION TO SET THE DEFAULTS" << endl;
  }
}

void KonviSettingsDialog::openWatchedNicknamesPage()
{
  // page index has been calculated in the constructor
  showPage(m_watchedNicknamesIndex);
}

#include "konvisettingsdialog.moc"

