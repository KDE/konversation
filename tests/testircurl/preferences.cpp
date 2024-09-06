/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#include "preferences.h"
#include "servergroupsettings.h"

Preferences Preferences::s_instance = Preferences();

const Konversation::ServerGroupSettingsPtr Preferences::serverGroupById(int)
{
    Konversation::ServerGroupSettingsPtr p(new Konversation::ServerGroupSettings);
    return p;
}

const QList<Konversation::ServerGroupSettingsPtr> Preferences::serverGroupsByServer(const QString&)
{
    QList<Konversation::ServerGroupSettingsPtr> r;
    return r;
}
