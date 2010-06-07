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

#include <KCmdLineArgs>
#include <KAboutData>

#define HACKSTR(x) #x
#define STRHACK(x) HACKSTR(x)

/*
  Don't use i18n() here, use ki18n() instead!
  i18n() will only work as soon as a kapplication object was made.
*/

int main(int argc, char* argv[])
{
    KAboutData aboutData("konversation",
        "",
        ki18n("Konversation"),
        KONVI_VERSION " #" STRHACK(COMMIT),
        ki18n("A user-friendly IRC client"),
        KAboutData::License_GPL,
        ki18n("(C) 2002-2010 by the Konversation team"),
        ki18n("Konversation is a client for the Internet Relay Chat (IRC) protocol.\n"
        "Meet friends on the net, make new acquaintances and lose yourself in\n"
        "talk about your favorite subject."),
        "http://konversation.kde.org/");

    aboutData.addAuthor(ki18n("Dario Abatianni"),ki18n("Original Author, Project Founder"),"eisfuchs@tigress.com");
    aboutData.addAuthor(ki18n("Peter Simonsson"),ki18n("Maintainer"),"psn@linux.se");
    aboutData.addAuthor(ki18n("Eike Hein"),ki18n("Maintainer, Release Manager, User interface, Connection management, Protocol handling, Auto-away"),"hein@kde.org");
    aboutData.addAuthor(ki18n("Shintaro Matsuoka"),ki18n("DCC, Encoding handling, OSD positioning"),"shin@shoegazed.org");
    aboutData.addAuthor(ki18n("Eli MacKenzie"),ki18n("Protocol handling, Input line"),"argonel@gmail.com");
    aboutData.addAuthor(ki18n("İsmail Dönmez"),ki18n("Blowfish, SSL support, KNetwork port, Colored nicks, Nicklist themes"),"ismail@kde.org");
    aboutData.addAuthor(ki18n("John Tapsell"),ki18n("Refactoring, KAddressBook/Kontact integration"), "john@geola.co.uk");
    aboutData.addAuthor(ki18n("Bernd Buschinski"),ki18n("DCC port to KDE 4, various DCC improvements"), "b.buschinski@web.de");

    aboutData.addCredit(ki18n("Olivier Bédard"),ki18n("Website hosting"));
    aboutData.addCredit(ki18n("Jędrzej Lisowski"),ki18n("Website maintenance"),"yesoos@gmail.com");
    aboutData.addCredit(ki18n("Christian Muehlhaeuser"),ki18n("Multiple modes extension, Close widget placement, OSD functionality"),"chris@chris.de");
    aboutData.addCredit(ki18n("Gary Cramblitt"),ki18n("Documentation, Watched nicks online improvements, Custom web browser extension"),"garycramblitt@comcast.net");
    aboutData.addCredit(ki18n("Matthias Gierlings"),ki18n("Color configurator, Highlight dialog"),"gismore@users.sourceforge.net");
    aboutData.addCredit(ki18n("Alex Zepeda"),ki18n("DCOP interface"),"garbanzo@hooked.net");
    aboutData.addCredit(ki18n("Stanislav Karchebny"),ki18n("Non-Latin1-Encodings"),"berkus@users.sourceforge.net");
    aboutData.addCredit(ki18n("Mickael Marchand"),ki18n("Konsole part view"),"marchand@kde.org");
    aboutData.addCredit(ki18n("Michael Goettsche"),ki18n("Quick connect, Ported new OSD, other features and bugfixes"),"michael.goettsche@kdemail.net");
    aboutData.addCredit(ki18n("Benjamin Meyer"),ki18n("A Handful of fixes and code cleanup"),"ben+konversation@meyerhome.net");
    aboutData.addCredit(ki18n("Jakub Stachowski"),ki18n("Drag&Drop improvements"),"qbast@go2.pl");
    aboutData.addCredit(ki18n("Sebastian Sariego"),ki18n("Artwork"),"segfault@kde.cl");
    aboutData.addCredit(ki18n("Renchi Raju"),ki18n("Firefox style searchbar"));
    aboutData.addCredit(ki18n("Michael Kreitzer"),ki18n("Raw modes, Tab grouping per server, Ban list"),"mrgrim@gr1m.org");
    aboutData.addCredit(ki18n("Frauke Oster"),ki18n("System tray patch"),"frauke@frsv.de");
    aboutData.addCredit(ki18n("Lucijan Busch"),ki18n("Bug fixes"),"lucijan@kde.org");
    aboutData.addCredit(ki18n("Sascha Cunz"),ki18n("Extended user modes patch"),"mail@sacu.de");
    aboutData.addCredit(ki18n("Steve Wollkind"),ki18n("Close visible tab with shortcut patch"),"steve@njord.org");
    aboutData.addCredit(ki18n("Thomas Nagy"),ki18n("Cycle tabs with mouse scroll wheel"),"thomas.nagy@eleve.emn.fr");
    aboutData.addCredit(ki18n("Tobias Olry"),ki18n("Channel ownership mode patch"),"tobias.olry@web.de");
    aboutData.addCredit(ki18n("Ruud Nabben"),ki18n("Option to enable IRC color filtering"),"r.nabben@gawab.com");
    aboutData.addCredit(ki18n("Lothar Braun"),ki18n("Bug fixes"),"mail@lobraun.de");
    aboutData.addCredit(ki18n("Ivor Hewitt"),ki18n("Bug fixes, OSD work, clearing topics"),"ivor@ivor.org");
    aboutData.addCredit(ki18n("Emil Obermayr"),ki18n("Sysinfo script"),"nobs@tigress.com");
    aboutData.addCredit(ki18n("Stanislav Nikolov"),ki18n("Bug fixes"),"valsinats@gmail.com");
    aboutData.addCredit(ki18n("Juan Carlos Torres"),ki18n("Auto-join context menu"),"carlosdgtorres@gmail.com");
    aboutData.addCredit(ki18n("Travis McHenry"),ki18n("Various fixes, ported encryption to QCA2, added DH1080 key exchange support."),"tmchenryaz@cox.net");
    aboutData.addCredit(ki18n("Modestas Vainius"),ki18n("Bug fixes and enhancements."),"modestas@vainius.eu");
    aboutData.addCredit(ki18n("Abdurrahman AVCI"),ki18n("Various bug fixes and enhancements."),"abdurrahmanavci@gmail.com");
    aboutData.addCredit(ki18n("Martin Blumenstingl"),ki18n("KStatusNotifierItem support, KIdleTime support, other enhancements"),"darklight.xdarklight@googlemail.com");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineOptions options;
    options.add( "+[url]", ki18n("irc:// URL or server hostname"), 0);
    options.add( "server <server>", ki18n("Server to connect"), 0 );
    options.add( "port <port>", ki18n("Port to use"), "6667");
    options.add( "channel <channel>", ki18n("Channel to join after connection"), "");
    options.add( "nick <nickname>", ki18n("Nickname to use"),"");
    options.add( "password <password>", ki18n("Password for connection"),"");
    options.add( "ssl", ki18n("Use SSL for connection"),"false");
    options.add( "noautoconnect", ki18n("Disable auto-connecting to any IRC networks"));


    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs::addStdCmdLineOptions();

    if (!KUniqueApplication::start()) return 0;

    Application app;

    return app.exec();
}
