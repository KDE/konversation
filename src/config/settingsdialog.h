/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#ifndef KONVISETTINGSDIALOG_H
#define KONVISETTINGSDIALOG_H

#include "settingspage.h"

#include <QList>

#include <configdialog.h>

class Warnings_Config;
class Theme_Config;
class NicklistBehavior_Config;
class Tabs_Config;
class Alias_Config;
class QuickButtons_Config;
class Autoreplace_Config;
class DCC_Config;
class Highlight_Config;
class OSD_Config;
class LauncherEntry_Config;
class Ignore_Config;


class KonviSettingsDialog : public ConfigDialog
{
    Q_OBJECT

    public:
        explicit KonviSettingsDialog(QWidget *parent);
        ~KonviSettingsDialog() override;

    protected Q_SLOTS:
        void updateSettings() override;
        void updateWidgets() override;
        void updateWidgetsDefault() override;
        void modifiedSlot();

    protected:
        bool hasChanged() override;
        bool isDefault() override;

    private:
        Warnings_Config* m_confWarningsWdg;
        Theme_Config* m_confThemeWdg;
        NicklistBehavior_Config* m_confNicklistBehaviorWdg;
        Tabs_Config* m_confTabBarWdg;
        Alias_Config* m_confAliasWdg;
        QuickButtons_Config* m_confQuickButtonsWdg;
        Autoreplace_Config* m_confAutoreplaceWdg;
        DCC_Config* m_confDCCWdg;
        Highlight_Config* m_confHighlightWdg;
        OSD_Config* m_confOSDWdg;
        LauncherEntry_Config* m_confLauncherEntryWdg;
        Ignore_Config* m_confIgnoreWdg;

        bool m_modified;

        // remember page index
        QList<KonviSettingsPage*> m_pages;

        Q_DISABLE_COPY(KonviSettingsDialog)
};

#endif //KONVISETTINGSDIALOG_H
