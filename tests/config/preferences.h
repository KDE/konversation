/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QUrl>

// mock class
class Preferences
{
private:
    Preferences() = default;

public:

    static Preferences *self() { return &s_instance; }

    bool disableExpansion() const { return false; }

private:
    static Preferences s_instance;
};

#endif
