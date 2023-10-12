/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/


#include "application.h"
#include "version.h"

#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>

#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
    Application app(argc, argv);

    KLocalizedString::setApplicationDomain("konversation");

    KAboutData aboutData(QStringLiteral("konversation"),
        i18n("Konversation"),
        QStringLiteral(KONVI_VERSION_STRING),
        i18n("A user-friendly IRC client"),
        KAboutLicense::GPL,
        i18n("Copyright 2002-%1 by the Konversation team", QStringLiteral("2022")),
        i18n("Konversation is a client for the Internet Relay Chat (IRC) protocol.\n\n"
        "Meet friends on the net, make new acquaintances and lose yourself in talk about your favorite subject."),
        QStringLiteral("https://konversation.kde.org/"));

    aboutData.addAuthor(i18n("Dario Abatianni"),i18n("Original Author, Project Founder"),QStringLiteral("eisfuchs@tigress.com"));
    aboutData.addAuthor(i18n("Peter Simonsson"),i18n("Maintainer"),QStringLiteral("peter.simonsson@gmail.com"));
    aboutData.addAuthor(i18n("Eike Hein"),i18n("Maintainer, Release Manager, User interface, Connection management, Protocol handling, Auto-away"),QStringLiteral("hein@kde.org"));
    aboutData.addAuthor(i18n("Shintaro Matsuoka"),i18n("DCC, Encoding handling, OSD positioning"),QStringLiteral("shin@shoegazed.org"));
    aboutData.addAuthor(i18n("Eli MacKenzie"),i18n("Protocol handling, Input line"),QStringLiteral("argonel@gmail.com"));
    aboutData.addAuthor(i18n("İsmail Dönmez"),i18n("Blowfish, SSL support, KNetwork port, Colored nicks, Nicklist themes"),QStringLiteral("ismail@kde.org"));
    aboutData.addAuthor(i18n("John Tapsell"),i18n("Refactoring, KAddressBook/Kontact integration"), QStringLiteral("john@geola.co.uk"));
    aboutData.addAuthor(i18n("Bernd Buschinski"),i18n("DCC port to KDE 4, various DCC improvements"), QStringLiteral("b.buschinski@web.de"));

    aboutData.addCredit(i18n("Olivier Bédard"),i18n("Website hosting"));
    aboutData.addCredit(i18n("Jędrzej Lisowski"),i18n("Website maintenance"),QStringLiteral("yesoos@gmail.com"));
    aboutData.addCredit(i18n("Christian Muehlhaeuser"),i18n("Multiple modes extension, Close widget placement, OSD functionality"),QStringLiteral("chris@chris.de"));
    aboutData.addCredit(i18n("Gary Cramblitt"),i18n("Documentation, Watched Nicks improvements, Custom web browser extension"),QStringLiteral("garycramblitt@comcast.net"));
    aboutData.addCredit(i18n("Matthias Gierlings"),i18n("Color configurator, Highlight dialog"),QStringLiteral("gismore@users.sourceforge.net"));
    aboutData.addCredit(i18n("Alex Zepeda"),i18n("DCOP interface"),QStringLiteral("garbanzo@hooked.net"));
    aboutData.addCredit(i18n("Stanislav Karchebny"),i18n("Non-Latin1-Encodings"),QStringLiteral("berkus@users.sourceforge.net"));
    aboutData.addCredit(i18n("Mickael Marchand"),i18n("Konsole part view"),QStringLiteral("marchand@kde.org"));
    aboutData.addCredit(i18n("Michael Goettsche"),i18n("Quick connect, Ported new OSD, other features and bugfixes"),QStringLiteral("michael.goettsche@kdemail.net"));
    aboutData.addCredit(i18n("Benjamin Meyer"),i18n("A Handful of fixes and code cleanup"),QStringLiteral("ben+konversation@meyerhome.net"));
    aboutData.addCredit(i18n("Jakub Stachowski"),i18n("Drag&Drop improvements"),QStringLiteral("qbast@go2.pl"));
    aboutData.addCredit(i18n("Sebastian Sariego"),i18n("Artwork"),QStringLiteral("segfault@kde.cl"));
    aboutData.addCredit(i18n("Renchi Raju"),i18n("Firefox style searchbar"));
    aboutData.addCredit(i18n("Michael Kreitzer"),i18n("Raw modes, Tab grouping per server, Ban list"),QStringLiteral("mrgrim@gr1m.org"));
    aboutData.addCredit(i18n("Frauke Oster"),i18n("System tray patch"),QStringLiteral("frauke@frsv.de"));
    aboutData.addCredit(i18n("Lucijan Busch"),i18n("Bug fixes"),QStringLiteral("lucijan@kde.org"));
    aboutData.addCredit(i18n("Sascha Cunz"),i18n("Extended user modes patch"),QStringLiteral("mail@sacu.de"));
    aboutData.addCredit(i18n("Steve Wollkind"),i18n("Close visible tab with shortcut patch"),QStringLiteral("steve@njord.org"));
    aboutData.addCredit(i18n("Thomas Nagy"),i18n("Cycle tabs with mouse scroll wheel"),QStringLiteral("thomas.nagy@eleve.emn.fr"));
    aboutData.addCredit(i18n("Tobias Olry"),i18n("Channel ownership mode patch"),QStringLiteral("tobias.olry@web.de"));
    aboutData.addCredit(i18n("Ruud Nabben"),i18n("Option to enable IRC color filtering"),QStringLiteral("r.nabben@gawab.com"));
    aboutData.addCredit(i18n("Lothar Braun"),i18n("Bug fixes"),QStringLiteral("mail@lobraun.de"));
    aboutData.addCredit(i18n("Ivor Hewitt"),i18n("Bug fixes, OSD work, clearing topics"),QStringLiteral("ivor@ivor.org"));
    aboutData.addCredit(i18n("Emil Obermayr"),i18n("Sysinfo script"),QStringLiteral("nobs@tigress.com"));
    aboutData.addCredit(i18n("Stanislav Nikolov"),i18n("Bug fixes"),QStringLiteral("valsinats@gmail.com"));
    aboutData.addCredit(i18n("Juan Carlos Torres"),i18n("Auto-join context menu"),QStringLiteral("carlosdgtorres@gmail.com"));
    aboutData.addCredit(i18n("Travis McHenry"),i18n("Various fixes, ported encryption to QCA2, added DH1080 key exchange support."),QStringLiteral("tmchenryaz@cox.net"));
    aboutData.addCredit(i18n("Modestas Vainius"),i18n("Bug fixes and enhancements."),QStringLiteral("modestas@vainius.eu"));
    aboutData.addCredit(i18n("Abdurrahman AVCI"),i18n("Various bug fixes and enhancements."),QStringLiteral("abdurrahmanavci@gmail.com"));
    aboutData.addCredit(i18n("Martin Blumenstingl"),i18n("KStatusNotifierItem support, KIdleTime support, other enhancements"),QStringLiteral("darklight.xdarklight@googlemail.com"));

    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("konversation"), QApplication::windowIcon()));

    QCommandLineParser cmdLineParser;

    cmdLineParser.addPositionalArgument(QStringLiteral("url"), i18n("irc:// URL or server hostname"));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("server"), i18n("Server to connect"), i18n("server")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("port"), i18n("Port to use"), i18n("port"), QStringLiteral("6667")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("channel"), i18n("Channel to join after connection"), i18n("channel")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("nick"), i18n("Nickname to use"), i18n("nickname")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("password"), i18n("Password for connection"), i18n("password")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("ssl"), i18n("Use SSL for connection")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("noautoconnect"), i18n("Disable auto-connecting to any IRC networks")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("startupdelay"), i18n("Delay D-Bus activity and UI creation by the specified amount of milliseconds"), i18n("msec"), QStringLiteral("2000")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("restart"), i18n("Quits and restarts Konversation (if running, otherwise has no effect)")));
#ifndef QT_NO_DEBUG
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("nui"), i18n("Sets KUniqueApplication::NonUniqueInstance (debug only, use with caution)")));
#endif
    aboutData.setupCommandLine(&cmdLineParser);

    cmdLineParser.process(app);
    aboutData.processCommandLine(&cmdLineParser);

    if (cmdLineParser.isSet(QStringLiteral("startupdelay")))
    {
        bool ok;
        ulong delay = cmdLineParser.value(QStringLiteral("startupdelay")).toULong(&ok);

        if (ok)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }

    KDBusService::StartupOptions startOptions = KDBusService::Unique;

#ifndef QT_NO_DEBUG
    if (cmdLineParser.isSet(QStringLiteral("nui")))
        startOptions = KDBusService::Multiple;
#endif

    startOptions |= KDBusService::NoExitOnFailure;

    KDBusService dbusService(startOptions);

    if (app.isSessionRestored() && KMainWindow::canBeRestored(1))
        app.restoreInstance();
    else
        app.newInstance(&cmdLineParser);
    app.setCommandLineParser(&cmdLineParser);

    QObject::connect(&dbusService, &KDBusService::activateRequested,
                     &app, &Application::handleActivate,
                     Qt::DirectConnection);
    QObject::connect(&dbusService, &KDBusService::openRequested,
                     &app, &Application::handleOpen,
                     Qt::DirectConnection);

    return QApplication::exec();
}
