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
    
    /** For dcop and addressbook, a user can be specified as user@irc.server.net
     *  or user@servergroup or using the unicode seperator symbol 0xE120 instead
     *  of the "@".  This function takes a string like the above examples, and
     *  modifies ircnick and serverOrGroup to contain the split up string.  If
     *  the string doesn't have an @ or 0xE120, ircnick is set to the
     *  nick_server, and serverOrGroup is set to empty.
     *  Behaviour is undefined for serverOrGroup if multiple @ or 0xE120 are found.
     *  @param nick_server A string containting ircnick and possibly servername or server group
     *  @param ircnick This is modified to contain the ircnick
     *  @param serverOrGroup This is modified to contain the servername, servergroup or an empty string.
     */
    static void splitNick_Server(QString nick_server, QString &ircnick, QString &serverOrGroup);
    
    /** Tries to find a nickinfo for a given ircnick on a given ircserver.
     *  @param ircnick The case-insensitive ircnick of the person you want to find.  e.g. "johnflux"
     *  @param serverOrGroup The case-insensitive server name (e.g. "irc.kde.org") or server group name (e.g. "freenode"), or null to search all servers
     *  @return A nickinfo for this user and server if one is found.
     */
    NickInfoPtr getNickInfo(const QString &ircnick, const QString &serverOrGroup);
    
    OSDWidget* osd;
    
    Konversation::Sound* sound();
    
    // Returns list of pointers to Servers.
    const QPtrList<Server> getServerList();
    
  signals:
    void catchUrl(const QString& who,const QString& url);
    void prefsChanged();

  public slots:
    void connectToServer(int number);
    bool connectToAnotherServer(int number);
    void quickConnectToServer(const QString& hostName, const QString& port = "6667", 
    						const QString& nick = KonversationApplication::preferences.getNickname(0),
						const QString& password="");
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
