/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QUrl>
#include "identity.h"
#include "servergroupsettings.h"
#include <QHash>

// mock class
class Preferences
{
private:
    Preferences() = default;

public:

    static Preferences *self() { return &s_instance; }

    bool disableExpansion() const { return false; }

    static bool isServerGroup(const QString& ) { return false; }
    static const IdentityPtr identityById(int ) {
        IdentityPtr y;
        y=new Identity();
        return y;
    }

    static QList<int> serverGroupIdsByName(const QString& ) { return {-1}; }
    static const Konversation::ServerGroupSettingsPtr serverGroupById(int);
    static const QList<Konversation::ServerGroupSettingsPtr> serverGroupsByServer(const QString& server);
private:
    static Preferences s_instance;
    Konversation::ServerGroupHash mServerGroupHash;
};

#endif
