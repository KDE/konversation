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

#ifndef IGNORE_CONFIG_H
#define IGNORE_CONFIG_H

#include "ui_ignore_configui.h"
#include "settingspage.h"

class Ignore;
class Ignore_Config : public QWidget, public KonviSettingsPage, private Ui::Ignore_ConfigUI
{
    Q_OBJECT

    public:
        explicit Ignore_Config( QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0 );
        ~Ignore_Config();
        QString flagNames;

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();

    private:
        QStringList m_oldIgnoreList;

        QStringList currentIgnoreList();  // in hasChanged() format
        QList<Ignore*> getIgnoreList(); // in prefs format
        void updateEnabledness();

    public Q_SLOTS:
        virtual void languageChange();

    protected Q_SLOTS:
        void newIgnore();
        void removeIgnore();
        void flagCheckboxChanged();
        void select(QTreeWidgetItem* item);
        void removeAllIgnore();
    Q_SIGNALS:
        void modified();
};

#endif // IGNORE_CONFIG_H
