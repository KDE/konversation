/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  main.cpp  -  Where it all began ...
  begin:     Die Jan 15 05:59:05 CET 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "konversationapplication.h"
#include "version.h"

/*
  Don't use i18n() here, use I18N_NOOP() instead!
  i18n() will only work as soon as a kapplication object was made.
*/

static const char* shortDescription=I18N_NOOP("A user friendly IRC client");

int main(int argc, char* argv[])
{
  KAboutData aboutData("konversation",
                       I18N_NOOP("Konversation"),
                       VERSION,
                       shortDescription,
                       KAboutData::License_GPL,
                       I18N_NOOP("(C)2002-2004 by the Konversation team"),
                       I18N_NOOP("Konversation is a client for the Internet Relay Chat (IRC) protocol.\n"
                                 "Meet friends on the net, make new acquaintances and lose yourself in\n"
                                 "talk about your favorite subject."),
                       "http://www.konversation.org/");

  aboutData.addAuthor("Dario Abatianni",I18N_NOOP("Project founder, main programmer, release coordinator"),"eisfuchs@tigress.com");
  aboutData.addAuthor("Matthias Gierlings",I18N_NOOP("Color configurator, Highlight dialog"),"gismore@users.sourceforge.net");
  aboutData.addAuthor("Alex Zepeda",I18N_NOOP("DCOP interface"),"garbanzo@hooked.net");
  aboutData.addAuthor("Stanislav Karchebny",I18N_NOOP("Non-Latin1-Encodings"),"berkus@users.sourceforge.net");
  aboutData.addAuthor("Mickael Marchand",I18N_NOOP("Konsole part view"),"marchand@kde.org");
  aboutData.addAuthor("Peter Simonsson",I18N_NOOP("Color picker, IRC color preferences, KNotify events, Systray notification, Shell style completion, Sound support for highlights"),"psn@linux.se");
  aboutData.addAuthor("Christian Muehlhaeuser",I18N_NOOP("Multiple modes extension, Close widget placement, OSD functionality"),"chris@chris.de");
  aboutData.addAuthor("John Tapsell",I18N_NOOP("Refactoring, Kadddressbook/Kontact integration"), "john@geola.co.uk");

  aboutData.addCredit("Frauke Oster",I18N_NOOP("System tray patch"),"frauke@frsv.de");
  aboutData.addCredit("Lucijan Busch",I18N_NOOP("Bug fixes"),"lucijan@kde.org");
  aboutData.addCredit("Sascha Cunz",I18N_NOOP("Extended user modes patch"),"mail@sacu.de");
  aboutData.addCredit("Steve Wollkind",I18N_NOOP("Close visible tab with shortcut patch"),"steve@njord.org");
  aboutData.addCredit("Thomas Nagy",I18N_NOOP("Cycle tabs with mouse scroll wheel"),"thomas.nagy@eleve.emn.fr");
  aboutData.addCredit("Gary Cramblitt",I18N_NOOP("DCC panel fixes, custom web browser extension"),"garycramblitt@comcast.net");
  aboutData.addCredit("Tobias Olry",I18N_NOOP("Channel ownership mode patch"),"tobias.olry@web.de");
  aboutData.addCredit("Ruud Nabben",I18N_NOOP("Option to enable IRC color filtering"),"r.nabben@gawab.com");
  aboutData.addCredit("Michael Goettsche",I18N_NOOP("Quick connect, Ported new OSD, other features and bugfixes"),"michael.goettsche@kdemail.net");
  aboutData.addCredit("İsmail Dönmez",I18N_NOOP("Ported to KNetwork, many bugfixes and features"),"kde@myrealbox.com");
  aboutData.addCredit("Luciash d' being",I18N_NOOP("Application icons"),"luci@sh.ground.cz");
  aboutData.addCredit("Shintaro Matsuoka",I18N_NOOP("DCC transfer"),"shin@shoegazed.org");

  KCmdLineArgs::init(argc,argv,&aboutData);

  KonversationApplication app;

  return app.exec();
}
