/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2022 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#include "launcherentry_config.h"

#include "preferences.h"
#include "osd.h"
#include "application.h"
#include "mainwindow.h"
#include "viewcontainer.h"
#include "launcherentryhandler.h"

LauncherEntry_Config::LauncherEntry_Config( QWidget* parent, const char* name)
    : QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);
}

LauncherEntry_Config::~LauncherEntry_Config() = default;

void LauncherEntry_Config::loadSettings()
{
}

void LauncherEntry_Config::restorePageToDefaults()
{
}

void LauncherEntry_Config::saveSettings()
{
}

bool LauncherEntry_Config::hasChanged()
{
    // follow the interface, no Non-KConfigXT settings here, so none have changed
    return false;
}

#include "moc_launcherentry_config.cpp"
