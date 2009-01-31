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

#ifndef KONVISETTINGSDIALOG_H
#define KONVISETTINGSDIALOG_H

#include "configdialog.h" ////// header renamed
#include "settingspage.h" ////// header renamed

#include <q3intdict.h>


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

class KonviSettingsDialog : public KonviConfigDialog
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
        explicit KonviSettingsDialog(QWidget *parent);
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
        KPageWidgetItem *m_watchedNicknamesPage;
        Q3IntDict<KonviSettingsPage> m_indexToPageMapping;
};

#endif //KONVISETTINGSDIALOG_H
