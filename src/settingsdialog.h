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

#include <QList>


class Warnings_Config;
class Theme_Config;
class NicklistBehavior_Config;
class Tabs_Config;
class Alias_Config;
class QuickButtons_Config;
class Autoreplace_Config;
class DCC_Config;
class WatchedNicknames_Config;
class Highlight_Config;
class OSD_Config;
class Ignore_Config;

class KonviSettingsDialog : public KonviConfigDialog
{
    Q_OBJECT

    protected:
	Warnings_Config* m_confWarningsWdg;
	Theme_Config* m_confThemeWdg;
	NicklistBehavior_Config* m_confNicklistBehaviorWdg;
	Tabs_Config* m_confTabBarWdg;
	Alias_Config* m_confAliasWdg;
	QuickButtons_Config* m_confQuickButtonsWdg;
	Autoreplace_Config* m_confAutoreplaceWdg;
	DCC_Config* m_confDCCWdg;
	WatchedNicknames_Config* m_confWatchedNicknamesWdg;
	Highlight_Config* m_confHighlightWdg;
	OSD_Config* m_confOSDWdg;
	Ignore_Config* m_confIgnoreWdg;

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
        QList<KonviSettingsPage*> m_pages;
};

#endif //KONVISETTINGSDIALOG_H
