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
 *  This makes a konversation settings dialog.  Just create a single instance and call 'show' when you want
 *  to show it.
 *
 *  begin:     Dec 8 2005
 *  copyright: (C) 2005 by John Tapsell, Konversation Developers
 *  email:     sho@eikehein.com
 */

#ifndef KONVISETTINGSDIALOG_H
#define KONVISETTINGSDIALOG_H

#include <kdialogbase.h>
#include <qasciidict.h>
#include <qintdict.h>

#include "konviconfigdialog.h"
#include "konvisettingspage.h"

class Warnings_Config;
class ChatWindowAppearance_Config;
class FontAppearance_Config;
class Theme_Config;
class ColorsAppearance_Config;
class GeneralBehavior_Config;
class ConnectionBehavior_Config;
class ChatwindowBehaviour_Config;
class NicklistBehavior_Config;
class Tabs_Config;
class Alias_Config;
class QuickButtons_Config;
class Autoreplace_Config;
class Log_Config;
class DCC_Config;
class WatchedNicknames_Config;
class Highlight_Config;
class OSD_Config;
class Ignore_Config;
class TabNotifications_Config;

class KDEUI_EXPORT KonviSettingsDialog : public KonviConfigDialog
{
    Q_OBJECT

    protected:
	Warnings_Config* m_confWarningsWdg;
	ChatWindowAppearance_Config* m_confChatWindowAppearanceWdg;
	FontAppearance_Config* m_confFontAppearanceWdg;
	Theme_Config* m_confThemeWdg;
	ColorsAppearance_Config* m_confColorsAppearanceWdg;
	GeneralBehavior_Config* m_confGeneralBehaviorWdg;
	ConnectionBehavior_Config* m_confConnectionBehaviorWdg;
	ChatwindowBehaviour_Config* m_confChatwindowBehaviourWdg;
	NicklistBehavior_Config* m_confNicklistBehaviorWdg;
	Tabs_Config* m_confTabBarWdg;
	Alias_Config* m_confAliasWdg;
	QuickButtons_Config* m_confQuickButtonsWdg;
	Autoreplace_Config* m_confAutoreplaceWdg;
	Log_Config* m_confLogWdg;
	DCC_Config* m_confDCCWdg;
	WatchedNicknames_Config* m_confWatchedNicknamesWdg;
	Highlight_Config* m_confHighlightWdg;
	OSD_Config* m_confOSDWdg;
	Ignore_Config* m_confIgnoreWdg;
	TabNotifications_Config* m_confTabNotificationsWdg;

	bool m_modified;

    public:
        KonviSettingsDialog( QWidget *parent);
        ~KonviSettingsDialog();

        void openWatchedNicknamesPage();

    protected slots:
        virtual void updateSettings();
        virtual void updateWidgets();
        virtual void updateWidgetsDefault();
        void modifiedSlot();

    protected:
        virtual bool hasChanged();
        virtual bool isDefault();

        // remember page index
        unsigned int m_watchedNicknamesIndex;
        QIntDict<KonviSettingsPage> m_indexToPageMapping;
};

#endif //KONVISETTINGSDIALOG_H
