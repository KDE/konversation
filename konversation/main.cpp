/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  main.cpp  -  description
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

static const char* description=I18N_NOOP("Konversation");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE

int main(int argc, char* argv[])
{
  KAboutData aboutData("konversation",
                       I18N_NOOP("Konversation"),
                       VERSION,
                       description,
                       KAboutData::License_GPL,
                       "(C)2002, Dario Abatianni",
                       0,
                       0,
                       "eisfuchs@tigress.com");

  aboutData.addAuthor("Dario Abatianni",0,"eisfuchs@tigress.com");
  aboutData.addAuthor("Matthias Gierlings",0,"gismore@users.sourceforge.net");

  KCmdLineArgs::init(argc,argv,&aboutData);

  KonversationApplication app;
 
  return app.exec();
}  
