/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
*/

#include "testcommon.h"

#include "common.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestCommon);

void TestCommon::testExtractColorCodes_data()
{
    QTest::addColumn<QString>("ircText");
    QTest::addColumn<QString>("expectedColorCodes");

    QTest::newRow("nocode")          << QStringLiteral("hello")                << QString();
    QTest::newRow("colorfg1")        << QStringLiteral("\x03""1hello")         << QStringLiteral("\x03""1");
    QTest::newRow("colorfg2")        << QStringLiteral("\x03""15hello")        << QStringLiteral("\x03""15");
    QTest::newRow("colorfgcomma")    << QStringLiteral("\x03""15,hello")       << QStringLiteral("\x03""15");
    QTest::newRow("colorfgbg")       << QStringLiteral("\x03""15,12hello")     << QStringLiteral("\x03""15,12");
    QTest::newRow("colorreset")      << QStringLiteral("\x03hello")            << QStringLiteral("\x03");
    QTest::newRow("colorresetcomma") << QStringLiteral("\x03,hello")           << QStringLiteral("\x03");
    QTest::newRow("colorfgbgreset")  << QStringLiteral("\x03""15,12hello\x0f") << QStringLiteral("\x03""15,12\x0f");
    QTest::newRow("biu")             << QStringLiteral("\x02he\x1dllo\x1f")    << QStringLiteral("\x02\x1d\x1f");
}

void TestCommon::testExtractColorCodes()
{
    QFETCH(QString, ircText);
    QFETCH(QString, expectedColorCodes);

    const QString colorCodes = Konversation::extractColorCodes(ircText);
    QCOMPARE(colorCodes, expectedColorCodes);
}

void TestCommon::testRemoveIrcMarkup_data()
{
    QTest::addColumn<QString>("ircText");
    QTest::addColumn<QString>("expectedText");

    QTest::newRow("nocode")          << QStringLiteral("hello")                << QStringLiteral("hello");
    QTest::newRow("colorfg1")        << QStringLiteral("\x03""1hello")         << QStringLiteral("hello");
    QTest::newRow("colorfg2")        << QStringLiteral("\x03""15hello")        << QStringLiteral("hello");
    QTest::newRow("colorfgcomma")    << QStringLiteral("\x03""15,hello")       << QStringLiteral(",hello");
    QTest::newRow("colorfgbg")       << QStringLiteral("\x03""15,12hello")     << QStringLiteral("hello");
    QTest::newRow("colorreset")      << QStringLiteral("\x03hello")            << QStringLiteral("hello");
    QTest::newRow("colorresetcomma") << QStringLiteral("\x03,hello")           << QStringLiteral(",hello");
    QTest::newRow("colorfgbgreset")  << QStringLiteral("\x03""15,12hello\x0f") << QStringLiteral("hello");
    QTest::newRow("bbmiddle")        << QStringLiteral("he\x02ll\x02o")        << QStringLiteral("hello");
    QTest::newRow("biu")             << QStringLiteral("\x02he\x1dllo\x1f")    << QStringLiteral("hello");
}

void TestCommon::testRemoveIrcMarkup()
{
    QFETCH(QString, ircText);
    QFETCH(QString, expectedText);

    const QString text = Konversation::removeIrcMarkup(ircText);
    QCOMPARE(text, expectedText);
}

void TestCommon::testMatchLength_data()
{
    QTest::addColumn<QString>("ircText");
    QTest::addColumn<int>("expectedLength");

    QTest::newRow("nocode")          << QStringLiteral("hello")                << 0;
    QTest::newRow("colorfg")         << QStringLiteral("\x03""15hello")        << 3;
    QTest::newRow("colorfgcomma")    << QStringLiteral("\x03""15,hello")       << 3;
    QTest::newRow("colorfgbg")       << QStringLiteral("\x03""15,12hello")     << 6;
    QTest::newRow("colorreset")      << QStringLiteral("\x03hello")            << 1;
    QTest::newRow("colorresetcomma") << QStringLiteral("\x03,hello")           << 1;
    QTest::newRow("b")               << QStringLiteral("\x02hello")            << 1;
    QTest::newRow("biu")             << QStringLiteral("\x02\x1d\x1fhello")    << 1;
}

/// Simulates usage of Konversation::colorRegExp in IRCView::adjustUrlRanges()
void TestCommon::testMatchLength()
{
    QFETCH(QString, ircText);
    QFETCH(int, expectedLength);

    int length = 0;
    QRegularExpressionMatch match = Konversation::colorRegExp.match(ircText, 0, QRegularExpression::NormalMatch, QRegularExpression::AnchorAtOffsetMatchOption);
    if (match.hasMatch()) {
        length = match.capturedLength();
    }
    QCOMPARE(length, expectedLength);
}

#include "moc_testcommon.cpp"
