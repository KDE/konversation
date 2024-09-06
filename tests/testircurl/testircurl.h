
#ifndef TEST_IRCURL
#define TEST_IRCURL

#include <QObject>

class TestIrcUrl : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testIRCUrl_data();
    void testIRCUrl();

    void testIRCUrl_regex_data();
    void testIRCUrl_regex();

    void testBareCon_data();
    void testBareCon();
};

#endif
