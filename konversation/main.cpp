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

  $Id$
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "konversationapplication.h"

#if QT_VERSION < 0x030100
#include <time.h>
#endif

/*
  Don't use i18n() here, use I18N_NOOP() instead!
  i18n() will only work as soon as a kapplication object was made.
*/

static const char* shortDescription=I18N_NOOP("A user friendly IRC client");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE

int main(int argc, char* argv[])
{
  KAboutData aboutData("konversation",
                       I18N_NOOP("Konversation"),
                       VERSION,
                       shortDescription,
                       KAboutData::License_GPL,
                       "(C)2002, 2003 Dario Abatianni",
                       I18N_NOOP("Konversation is a client for the Internet Relay Chat (IRC) protocol.\n"
                                 "Meet friends on the net, make new acquaintances and lose yourself in\n"
                                 "talk about your favourite subject."),
                       "http://konversation.sourceforge.net/",
                       "eisfuchs@tigress.com");

  aboutData.addAuthor("Dario Abatianni",0,"eisfuchs@tigress.com");
  aboutData.addAuthor("Matthias Gierlings",0,"gismore@users.sourceforge.net");

  KCmdLineArgs::init(argc,argv,&aboutData);

  KonversationApplication app;
 
  return app.exec();
}  

#if QT_VERSION < 0x030100
// copied from Trolltech QT 3.1
unsigned int toTime_t(QDateTime dt)
{
    tm brokenDown;

    QDate d=dt.date();
    QTime t=dt.time();

    brokenDown.tm_sec = t.second();
    brokenDown.tm_min = t.minute();
    brokenDown.tm_hour = t.hour();
    brokenDown.tm_mday = d.day();
    brokenDown.tm_mon = d.month() - 1;
    brokenDown.tm_year = d.year() - 1900;
    brokenDown.tm_isdst = -1;
    int secsSince1Jan1970UTC = (int) mktime( &brokenDown );
    if ( secsSince1Jan1970UTC < -1 )
        secsSince1Jan1970UTC = -1;

    return (unsigned int) secsSince1Jan1970UTC;
}
#endif
