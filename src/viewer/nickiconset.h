/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#ifndef NICKICONSET_H
#define NICKICONSET_H

// app
#include "images.h"
// Qt
#include <QIcon>

class NickIconSet
{
public:
    enum UserPresence {
        UserPresent = 0,
        UserAway = 1,
    };

    bool load(const QString &baseDir);

    bool isNull() const;
    QIcon nickIcon(Images::NickPrivilege privilege, NickIconSet::UserPresence presence = UserPresent) const;
    // special need for tooltip
    QIcon nickIconAwayOverlay() const;
    int defaultIconSize() const;

private:
    void clear();

private:
    QIcon m_nickIcons[Images::_NickPrivilege_COUNT][2];
    QIcon m_nickIconAwayOverlay;
    int m_defaultIconSize = 0;
};

#endif
