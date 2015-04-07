/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Where it all began ...
  begin:     Die Jan 15 05:59:05 CET 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/


#include "application.h"
#include "version.h"
#include "commit.h"

#include <QMutex>
#include <QWaitCondition>
#include <QCommandLineParser>

#include <KAboutData>
#include <Kdelibs4ConfigMigrator>
#include <KDBusAddons/KDBusService>

int main(int argc, char* argv[])
{

    // Migrate pre-existing (4.x) configuration

    QStringList configFiles;

    configFiles.append(QLatin1String("konversationrc"));
    configFiles.append(QLatin1String("konversation.notifyrc"));

    Kdelibs4ConfigMigrator migrate(QLatin1String("konversation"));
    migrate.setConfigFiles(configFiles);
    migrate.setUiFiles(QStringList() << QLatin1String("konversationui.rc"));
    migrate.migrate();

    Application app(argc, argv);

    KLocalizedString::setApplicationDomain("konversation");

    KAboutData aboutData("konversation",
        i18n("Konversation"),
        KONVI_VERSION,
        i18n("A user-friendly IRC client"),
        KAboutLicense::GPL,
        i18n("(C) 2002-2014 by the Konversation team"),
        i18n("Konversation is a client for the Internet Relay Chat (IRC) protocol.\n\n"
        "Meet friends on the net, make new acquaintances and lose yourself in talk about your favorite subject."),
        QStringLiteral("http://konversation.kde.org/"));

    aboutData.addAuthor(i18n("Dario Abatianni"),i18n("Original Author, Project Founder"),"eisfuchs@tigress.com");
    aboutData.addAuthor(i18n("Peter Simonsson"),i18n("Maintainer"),"peter.simonsson@gmail.com");
    aboutData.addAuthor(i18n("Eike Hein"),i18n("Maintainer, Release Manager, User interface, Connection management, Protocol handling, Auto-away"),"hein@kde.org");
    aboutData.addAuthor(i18n("Shintaro Matsuoka"),i18n("DCC, Encoding handling, OSD positioning"),"shin@shoegazed.org");
    aboutData.addAuthor(i18n("Eli MacKenzie"),i18n("Protocol handling, Input line"),"argonel@gmail.com");
    aboutData.addAuthor(i18n("İsmail Dönmez"),i18n("Blowfish, SSL support, KNetwork port, Colored nicks, Nicklist themes"),"ismail@kde.org");
    aboutData.addAuthor(i18n("John Tapsell"),i18n("Refactoring, KAddressBook/Kontact integration"), "john@geola.co.uk");
    aboutData.addAuthor(i18n("Bernd Buschinski"),i18n("DCC port to KDE 4, various DCC improvements"), "b.buschinski@web.de");

    aboutData.addCredit(i18n("Olivier Bédard"),i18n("Website hosting"));
    aboutData.addCredit(i18n("Jędrzej Lisowski"),i18n("Website maintenance"),"yesoos@gmail.com");
    aboutData.addCredit(i18n("Christian Muehlhaeuser"),i18n("Multiple modes extension, Close widget placement, OSD functionality"),"chris@chris.de");
    aboutData.addCredit(i18n("Gary Cramblitt"),i18n("Documentation, Watched Nicks improvements, Custom web browser extension"),"garycramblitt@comcast.net");
    aboutData.addCredit(i18n("Matthias Gierlings"),i18n("Color configurator, Highlight dialog"),"gismore@users.sourceforge.net");
    aboutData.addCredit(i18n("Alex Zepeda"),i18n("DCOP interface"),"garbanzo@hooked.net");
    aboutData.addCredit(i18n("Stanislav Karchebny"),i18n("Non-Latin1-Encodings"),"berkus@users.sourceforge.net");
    aboutData.addCredit(i18n("Mickael Marchand"),i18n("Konsole part view"),"marchand@kde.org");
    aboutData.addCredit(i18n("Michael Goettsche"),i18n("Quick connect, Ported new OSD, other features and bugfixes"),"michael.goettsche@kdemail.net");
    aboutData.addCredit(i18n("Benjamin Meyer"),i18n("A Handful of fixes and code cleanup"),"ben+konversation@meyerhome.net");
    aboutData.addCredit(i18n("Jakub Stachowski"),i18n("Drag&Drop improvements"),"qbast@go2.pl");
    aboutData.addCredit(i18n("Sebastian Sariego"),i18n("Artwork"),"segfault@kde.cl");
    aboutData.addCredit(i18n("Renchi Raju"),i18n("Firefox style searchbar"));
    aboutData.addCredit(i18n("Michael Kreitzer"),i18n("Raw modes, Tab grouping per server, Ban list"),"mrgrim@gr1m.org");
    aboutData.addCredit(i18n("Frauke Oster"),i18n("System tray patch"),"frauke@frsv.de");
    aboutData.addCredit(i18n("Lucijan Busch"),i18n("Bug fixes"),"lucijan@kde.org");
    aboutData.addCredit(i18n("Sascha Cunz"),i18n("Extended user modes patch"),"mail@sacu.de");
    aboutData.addCredit(i18n("Steve Wollkind"),i18n("Close visible tab with shortcut patch"),"steve@njord.org");
    aboutData.addCredit(i18n("Thomas Nagy"),i18n("Cycle tabs with mouse scroll wheel"),"thomas.nagy@eleve.emn.fr");
    aboutData.addCredit(i18n("Tobias Olry"),i18n("Channel ownership mode patch"),"tobias.olry@web.de");
    aboutData.addCredit(i18n("Ruud Nabben"),i18n("Option to enable IRC color filtering"),"r.nabben@gawab.com");
    aboutData.addCredit(i18n("Lothar Braun"),i18n("Bug fixes"),"mail@lobraun.de");
    aboutData.addCredit(i18n("Ivor Hewitt"),i18n("Bug fixes, OSD work, clearing topics"),"ivor@ivor.org");
    aboutData.addCredit(i18n("Emil Obermayr"),i18n("Sysinfo script"),"nobs@tigress.com");
    aboutData.addCredit(i18n("Stanislav Nikolov"),i18n("Bug fixes"),"valsinats@gmail.com");
    aboutData.addCredit(i18n("Juan Carlos Torres"),i18n("Auto-join context menu"),"carlosdgtorres@gmail.com");
    aboutData.addCredit(i18n("Travis McHenry"),i18n("Various fixes, ported encryption to QCA2, added DH1080 key exchange support."),"tmchenryaz@cox.net");
    aboutData.addCredit(i18n("Modestas Vainius"),i18n("Bug fixes and enhancements."),"modestas@vainius.eu");
    aboutData.addCredit(i18n("Abdurrahman AVCI"),i18n("Various bug fixes and enhancements."),"abdurrahmanavci@gmail.com");
    aboutData.addCredit(i18n("Martin Blumenstingl"),i18n("KStatusNotifierItem support, KIdleTime support, other enhancements"),"darklight.xdarklight@googlemail.com");

    KAboutData::setApplicationData(aboutData);

    app.setApplicationName(aboutData.componentName());
    app.setOrganizationDomain(aboutData.organizationDomain());
    app.setApplicationVersion(aboutData.version());
    app.setApplicationDisplayName(aboutData.displayName());
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();

    cmdLineParser.addPositionalArgument(QStringLiteral("url"), i18n("irc:// URL or server hostname"));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("server"), i18n("Server to connect"), i18n("server")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("port"), i18n("Port to use"), i18n("port"), "6667"));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("channel"), i18n("Channel to join after connection"), i18n("channel")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("nick"), i18n("Nickname to use"), i18n("nickname")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("password"), i18n("Password for connection"), i18n("password")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("ssl"), i18n("Use SSL for connection")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("noautoconnect"), i18n("Disable auto-connecting to any IRC networks")));
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("startupdelay"), i18n("Delay D-Bus activity and UI creation by the specified amount of miliseconds"), i18n("msec"), "2000"));
    // cmdLineParser.addOption(QCommandLineOption(QStringLiteral("restart"), i18n("Quits and restarts Konversation (if running, otherwise has no effect)"))); FIXME KF5 Port: --restart
#ifndef QT_NO_DEBUG
    cmdLineParser.addOption(QCommandLineOption(QStringLiteral("nui"), i18n("Sets KUniqueApplication::NonUniqueInstance (debug only, use with caution)")));
#endif
    aboutData.setupCommandLine(&cmdLineParser);

    cmdLineParser.process(app);

    if (cmdLineParser.isSet(QStringLiteral("startupdelay")))
    {
        bool ok;
        ulong delay = cmdLineParser.value(QStringLiteral("startupdelay")).toULong(&ok);

        if (ok)
        {
            QMutex dummy;
            dummy.lock();
            QWaitCondition waitCondition;
            waitCondition.wait(&dummy, delay);
        }
    }

    KDBusService::StartupOptions startOptions = KDBusService::Unique;

#ifndef QT_NO_DEBUG
    if (cmdLineParser.isSet(QStringLiteral("nui")))
        startOptions = KDBusService::Multiple;
#endif

    KDBusService dbusService(startOptions);

    aboutData.processCommandLine(&cmdLineParser);
    app.newInstance(&cmdLineParser);

    QObject::connect(&dbusService, &KDBusService::activateRequested,
        app.instance()->getMainWindow(), &MainWindow::activateAndRaiseWindow,
        Qt::DirectConnection);

    return app.exec();
}
