/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#ifndef GENERAL_CONFIG_H
#define GENERAL_CONFIG_H

#include "ui_generalbehavior_configui.h"
#include "settingspage.h"

class GeneralBehavior_Config : public QWidget, public KonviSettingsPage, private Ui::GeneralBehavior_ConfigUI
{
    Q_OBJECT
    
    public:
        explicit GeneralBehavior_Config(QWidget* parent, const char* name = 0);
        ~GeneralBehavior_Config();
        
        virtual void saveSettings();
        virtual void loadSettings();
        virtual void restorePageToDefaults();
        
        virtual bool hasChanged();
};

#endif