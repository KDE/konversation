/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#ifndef TESTCOMMON_H
#define TESTCOMMON_H

#include <QObject>

class TestCommon : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testExtractColorCodes_data();
    void testExtractColorCodes();
    void testRemoveIrcMarkup_data();
    void testRemoveIrcMarkup();
    void testMatchLength_data();
    void testMatchLength();
};

#endif
