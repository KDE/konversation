/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationapplication.h  -  description
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kapp.h>
#include <ksimpleconfig.h>

#ifndef KONVERSATIONAPPLICATION_H
#define KONVERSATIONAPPLICATION_H

#include "preferences.h"
#include "prefsdialog.h"
#include "server.h"

/*
  @author Dario Abatianni
*/

class KonversationApplication : public KApplication
{
  Q_OBJECT
  public:
    static Preferences preferences;

    /* URL-Catcher */
    static QStringList urlList;
    static void storeURL(QString& url);

    KonversationApplication();
    ~KonversationApplication();

  public slots:
    void connectToServer(int number);
    void connectToAnotherServer(int number);
    void readOptions();
    void saveOptions();
    void quitKonversation();

  protected:
    QList<Server> serverList;
    KSimpleConfig* config;
    PrefsDialog* prefsDialog;
};

#endif
