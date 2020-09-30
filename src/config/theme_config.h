/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/

#ifndef THEME_CONFIG_H
#define THEME_CONFIG_H

#include "ui_theme_configui.h"
#include "settingspage.h"

#include <KIO/Job>


class Theme_Config : public QWidget, public KonviSettingsPage, private Ui::Theme_ConfigUI
{
    Q_OBJECT

    public:
        explicit Theme_Config(QWidget* parent, const char* name=NULL);
        ~Theme_Config() override;

        void restorePageToDefaults() override;
        void saveSettings() override;
        void loadSettings() override;

        bool hasChanged() override;

    Q_SIGNALS:
        void modified();

    protected Q_SLOTS:
        void updatePreview(int id);
        void updateButtons();
        void installTheme();
        void removeTheme();
        void postRemoveTheme(KJob* delete_job);

    private:
        QStringList m_dirs;
        QString m_oldTheme;
        QString m_currentTheme;
        int m_defaultThemeIndex;
};

#endif
