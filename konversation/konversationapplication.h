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

  $Id$
*/

#ifndef KONVERSATIONAPPLICATION_H
#define KONVERSATIONAPPLICATION_H

#include <kapp.h>
#include <ksimpleconfig.h>

#include <qcstring.h>

#include "preferences.h"
#include "prefsdialog.h"
#include "server.h"
#include "event.h"

class KonvDCOP;

/*
  @author Dario Abatianni
*/

class KonversationApplication : public KApplication
{
  Q_OBJECT
  public:

    static Preferences preferences;

    // URL-Catcher
    // TODO: Provide a list of seen URLs
    static QStringList urlList;
    static void storeURL(const QString &url);

    // Returns a list of signals we should emit
    void processHooks (EVENT_TYPE type, const QString &criteria, const QString &sender, const QString &target, const QString &data);

    KonversationApplication();
    ~KonversationApplication();

    void syncPrefs();

  public slots:
    void connectToServer(int number);
    void connectToAnotherServer(int number);
    void readOptions();
    void saveOptions(bool updateGUI=true);
    void quitKonversation();

    void openPrefsDialog();
    void closePrefsDialog();

    void emitDCOPSig(const QCString &signal,QByteArray &data);

  protected slots:
    void removeServer(Server* server);
    void dcopSay(const QString& server,const QString& target,const QString& command);
    void dcopError(const QString& string);

  protected:
    QPtrList<Server> serverList;
    PrefsDialog* prefsDialog;
    KonvDCOP* dcopObject;
};

#endif
