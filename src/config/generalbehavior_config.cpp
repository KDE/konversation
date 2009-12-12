/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "generalbehavior_config.h"
#include "preferences.h"

#include <config-konversation.h>

GeneralBehavior_Config::GeneralBehavior_Config(QWidget* parent, const char* name)
: QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);
    
#ifdef HAVE_NOTIFICATIONITEM
    kcfg_TrayNotifyBlink->setDisabled(true);
#endif
}

GeneralBehavior_Config::~GeneralBehavior_Config()
{
}

void GeneralBehavior_Config::restorePageToDefaults()
{
}

void GeneralBehavior_Config::loadSettings()
{
}

void GeneralBehavior_Config::saveSettings()
{
}

bool GeneralBehavior_Config::hasChanged()
{
    return true; // FIXME: is this OK?
}

#include "generalbehavior_config.moc"
