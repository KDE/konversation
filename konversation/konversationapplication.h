/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationapplication.h  -  The main application
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef KONVERSATIONAPPLICATION_H
#define KONVERSATIONAPPLICATION_H

#include <kapp.h>

#include "preferences.h"
#include "ircevent.h"
#include "osd.h"
#include "konvdcop.h"
#include "identity.h"
#include "nickinfo.h"

class QCString;

class KonversationMainWindow;
class KonvDCOP;
class Server;
class PrefsDialog;
class QuickConnectDialog;

namespace Konversation {
  class Sound;
}

/*
  @author Dario Abatianni
*/

class KonversationApplication : public KApplication
{
  Q_OBJECT

  public:
    static Preferences preferences;

    // URL-Catcher
    void storeUrl(const QString& who,const QString& url);
    const QStringList& getUrlList();

    // DCOP: Returns a list of signals we should emit
    QPtrList<IRCEvent> retrieveHooks(EVENT_TYPE type);

    KonversationApplication();
    ~KonversationApplication();

    void syncPrefs();
    Server* getServerByName(const QString& name);
    /** Tries to find a nickinfo for a given ircnick on a given ircserver.
     * @param ircnick The case-insensitive ircnick of the person you want to find.  e.g. "johnflux"
     * @param serverOrGroup The case-insensitive server name (e.g. "irc.kde.org") or server group name (e.g. "freenode"), or null to search all servers
     * @return A nickinfo for this user and server if one is found.
     */
    NickInfoPtr getNickInfo(const QString &ircnick, const QString &serverOrGroup);
    OSDWidget* osd;
    
    Konversation::Sound* sound();
    
    // Returns list of pointers to Servers.
    const QPtrList<Server> getServerList();

  signals:
    void catchUrl(const QString& who,const QString& url);

  public slots:
    void connectToServer(int number);
    bool connectToAnotherServer(int number);
    void quickConnectToServer(const QString& hostName, const QString& port = "6667", 
    						const QString& nick = KonversationApplication::preferences.getNickname(0), const QString& password="");
    void readOptions();
    void saveOptions(bool updateGUI=true);
    void quitKonversation();

    void closePrefsDialog();

    void deleteUrl(const QString& who,const QString& url);
    void clearUrlList();

    bool emitDCOPSig(const QString& appId, const QString& objId, const QString& signal, QByteArray& data);

  protected slots:
    void openPrefsDialog();
    void openPrefsDialog(Preferences::Pages page);
    void openQuickConnectDialog();
    void removeServer(Server* server);
    void dcopSay(const QString& server,const QString& target,const QString& command);
    void dcopInfo(const QString& string);
    void insertRememberLine();
    void appearanceChanged();
    void sendMultiServerCommand(const QString& command, const QString& parameter);
    void dcopConnectToServer(const QString& url, int port);
    
  protected:
    QPtrList<Server> serverList;
    QStringList urlList;
    PrefsDialog* prefsDialog;
    KonvDCOP* dcopObject;
    KonvPrefsDCOP* prefsDCOP;
    KonvIdentDCOP* identDCOP;
    KonversationMainWindow* mainWindow;
    Konversation::Sound* m_sound;
    QuickConnectDialog* quickConnectDialog;

};

#endif
