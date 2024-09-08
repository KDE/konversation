#include <QTest>
#include <QObject>
#include <QString>
#include "testircurl.h"

QTEST_GUILESS_MAIN(TestIrcUrl);

#include "../../src/connectionsettings.h"

#include <QRegularExpression>
#include <QLoggingCategory>

void TestIrcUrl::testBareCon_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("port");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("nick");
    QTest::addColumn<QString>("channel");
    QTest::addColumn<bool>("useSSL");

    QTest::addColumn<QString>("result_host");
    QTest::addColumn<int>("result_port");
    QTest::addColumn<bool>("result_useSSL");

    QTest::newRow("none")
        << QString() << QString() << QString() << QString() << QString() << false
        << QString() << 6667 << false;
    QTest::newRow("servername")
        << QStringLiteral("libera.chat") << QString() << QString() << QString() << QString() << false
        << QStringLiteral("libera.chat") << 6667 << false;
    QTest::newRow("servername_and_port")
        << QStringLiteral("libera.chat:6668") << QString() << QString() << QString() << QString() << false
        << QStringLiteral("libera.chat") << 6668 << false;
    QTest::newRow("servername_and_port_twice")
        << QStringLiteral("libera.chat:6668") << QStringLiteral("6669") << QString() << QString() << QString() << false
        << QStringLiteral("libera.chat") << 6669 << false;
    QTest::newRow("ssl-server")
        << QStringLiteral("libera.chat") << QString() << QString() << QString() << QString() << true
        << QStringLiteral("libera.chat") << 6697 << true;
    QTest::newRow("ssl-server-port-notssl")
        << QStringLiteral("libera.chat") << QStringLiteral("6697") << QString() << QString() << QString() << false
        << QStringLiteral("libera.chat") << 6697 << false;
    QTest::newRow("ssl-server-port-inaddr")
        << QStringLiteral("libera.chat:6697") << QStringLiteral("") << QString() << QString() << QString() << false
        << QStringLiteral("libera.chat") << 6697 << false;
    QTest::newRow("ssl-server-other-port-inaddr")
        << QStringLiteral("libera.chat:6600") << QStringLiteral("") << QString() << QString() << QString() << true
        << QStringLiteral("libera.chat") << 6600 << true;
    QTest::newRow("ssl-server-port-twice")
        << QStringLiteral("libera.chat:6600") << QStringLiteral("7000") << QString() << QString() << QString() << true
        << QStringLiteral("libera.chat") << 7000 << true;

}

void TestIrcUrl::testBareCon()
{
    QFETCH(QString, target);
    QFETCH(QString, port);
    QFETCH(QString, password);
    QFETCH(QString, nick);
    QFETCH(QString, channel);
    QFETCH(bool, useSSL);
    QFETCH(QString, result_host);
    QFETCH(int, result_port);
    QFETCH(bool, result_useSSL);

    ConnectionSettings con(target, port, password, nick, channel, useSSL);

    QCOMPARE(result_host, con.server().host());
    QCOMPARE(result_port, con.server().port());
    QCOMPARE(result_useSSL, con.server().SSLEnabled());
}


void TestIrcUrl::testIRCUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");

    QTest::newRow("nodata") << QStringLiteral("") << QString() << 6667;
    QTest::newRow("plain-twoslashes-nodata") << QStringLiteral("irc://") << QString() << 6667;
    QTest::newRow("plain-twoslashes-onlyserver") << QStringLiteral("irc://libera.chat") << QStringLiteral("libera.chat") << 6667;
    QTest::newRow("plain-twoslashes-server-slash") << QStringLiteral("irc://libera.chat/") << QStringLiteral("libera.chat") << 6667;
    QTest::newRow("irc-server-other-port") << QStringLiteral("irc://libera.chat:7000") << QStringLiteral("libera.chat") << 7000;
    QTest::newRow("irc-server-other-port-slash") << QStringLiteral("irc://libera.chat:7001/") << QStringLiteral("libera.chat") << 7001;

    QTest::newRow("ircs-nodata") << QStringLiteral("ircs://") << QStringLiteral("") << 6667;
    QTest::newRow("ircs-onlyserver") << QStringLiteral("ircs://libera.chat") << QStringLiteral("libera.chat") << 6697;
    QTest::newRow("ircs-onlyserver-slash") << QStringLiteral("ircs://libera.chat/") << QStringLiteral("libera.chat") << 6697;
    QTest::newRow("ircs-server-other-port") << QStringLiteral("ircs://libera.chat:7000") << QStringLiteral("libera.chat") << 7000;
    QTest::newRow("ircs-server-other-port-slash") << QStringLiteral("ircs://libera.chat:7000/") << QStringLiteral("libera.chat") << 7000;
}

void TestIrcUrl::testIRCUrl()
{
    QFETCH(QString, url);
    QFETCH(QString, host);
    QFETCH(int, port);

    ConnectionSettings con(url);
    // qWarning() << con.server().host() << con.server().port();

    QCOMPARE(con.server().host(), host);
    QCOMPARE(con.server().port(), port);
}

void TestIrcUrl::testIRCUrl_regex_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("result");
    QTest::addColumn<bool>("TLS");

    QTest::newRow("plain-twoslashes-nodata") << QStringLiteral("irc://") << QStringLiteral("irc://") << false;
    QTest::newRow("plain-twoslashes-onlyserver") << QStringLiteral("irc://libera.chat") << QStringLiteral("irc://libera.chat") << false;
    QTest::newRow("plain-twoslashes-server-and-port") << QStringLiteral("irc://libera.chat:6668") << QStringLiteral("irc://libera.chat:6668") << false;

    QTest::newRow("TLS-twoslashes-nodata") << QStringLiteral("ircs://") << QStringLiteral("ircs://") << true;
    QTest::newRow("TLS-twoslashes-onlyserver") << QStringLiteral("ircs://libera.chat") << QStringLiteral("ircs://libera.chat") << true;


}

void TestIrcUrl::testIRCUrl_regex()
{
    QFETCH(QString, url);
    QFETCH(QString, result);
    QFETCH(bool, TLS);

    QCOMPARE(url, result);
    QRegularExpression s(QStringLiteral("^irc(?<s>s)?:/+(?<u>.*)"));
    auto m = s.match(url);
    bool ssl = m.capturedLength(QStringLiteral("s"));
    // qWarning() << m << m.capturedTexts() << m.capturedLength(2) << ssl;
    QCOMPARE(TLS, ssl);


}
//#include "testircurl.moc"
