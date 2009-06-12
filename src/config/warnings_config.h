/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef WARNINGS_CONFIG_H
#define WARNINGS_CONFIG_H

#include "ui_warnings_configui.h"
#include "settingspage.h"


class Warnings_Config : public QWidget, public KonviSettingsPage, private Ui::Warnings_ConfigUI
{
    Q_OBJECT

    public:
        explicit Warnings_Config( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
        ~Warnings_Config();

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();

    public slots:
        virtual void languageChange();

    protected:
        QString currentWarningsChecked(); // for hasChanged()

        QString m_oldWarningsChecked;     // for hasChanged()

    signals:
        void modified();
};

#endif // WARNINGS_CONFIG_H
