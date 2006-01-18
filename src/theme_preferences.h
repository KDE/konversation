/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  (C) 2004,2005 by İsmail Dönmez
*/

#ifndef PREFSPAGETHEMES_H
#define PREFSPAGETHEMES_H

#include "theme_preferencesui.h"
#include "konvisettingspage.h"

class QStringList;

class Theme_Config : public Theme_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

    public:
        Theme_Config(QWidget* parent, const char* name=NULL);
        ~Theme_Config();

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();

    protected slots:
        void updatePreview(int id);
        void updateButtons();
        void installTheme();
        void removeTheme();

    private:
        QStringList m_dirs;
        QString m_oldTheme;
        QString m_currentTheme;
};
#endif
