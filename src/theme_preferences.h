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

#ifndef PREFSPAGETHEMES_H
#define PREFSPAGETHEMES_H

#include "theme_preferencesui.h"
#include "konvisettingspage.h"

#include <kio/job.h>


class QStringList;

class Theme_Config : public Theme_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

    public:
        explicit Theme_Config(QWidget* parent, const char* name=NULL);
        ~Theme_Config();

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();

    signals:
        void modified();

    protected slots:
        void updatePreview(int id);
        void updateButtons();
        void installTheme();
        void removeTheme();
        void postRemoveTheme(KIO::Job* delete_job);

    private:
        QStringList m_dirs;
        QString m_oldTheme;
        QString m_currentTheme;
        int m_defaultThemeIndex;
};
#endif
