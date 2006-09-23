/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#ifndef KONVERSATIONAPPLICATION_H
#define KONVERSATIONAPPLICATION_H

#include <kuniqueapplication.h>

#include "preferences.h"
#include "osd.h"
#include "konvdcop.h"
#include "identity.h"
#include "nickinfo.h"

class QCString;

class KonversationMainWindow;
class KonvDCOP;
class Server;
class QuickConnectDialog;
class Images;

namespace Konversation
{
    class Sound;
    class NotificationHandler;

    // Shared between NickListView and IRCView
    enum PopupIDs
    {
        ModesSub,GiveOp,TakeOp,GiveHalfOp,TakeHalfOp,GiveVoice,TakeVoice,
        KickBanSub,IgnoreNick,UnignoreNick,
        Kick,KickBan,BanNick,BanHost,BanDomain,BanUserHost,BanUserDomain,
        KickBanHost,KickBanDomain,KickBanUserHost,KickBanUserDomain,
        Whois,Version,Ping,OpenQuery,DccSend,Join,Names,Topic,
        CustomID, AddressbookSub, AddressbookChange, AddressbookNew, AddressbookDelete,
        AddressbookEdit, SendEmail, StartDccChat, AddNotify
    };

}

class KonversationApplication : public KUniqueApplication
{
    Q_OBJECT

    public:
        /** This function in general shouldn't be called, because in the future there
         *  may be multiple windows.
         *  However, in some situations we have messageboxes that aren't targeted for
         *  any particular main window, such as general errors from dcop calls.
         *
         *  Note to any MDI developer - get this to return any of the windows, or some
         *  'main' one.
         */
        KonversationMainWindow *getMainWindow();
        // URL-Catcher
        void storeUrl(const QString& who,const QString& url);
        const QStringList& getUrlList();

        KonversationApplication();
        ~KonversationApplication();

        static KonversationApplication* instance();

        Server* getServerByName(const QString& name);

        /** For dcop and addressbook, a user can be specified as user@irc.server.net
         *  or user\@servergroup or using the unicode separator symbol 0xE120 instead
         *  of the "@".  This function takes a string like the above examples, and
         *  modifies ircnick and serverOrGroup to contain the split up string.  If
         *  the string doesn't have an @ or 0xE120, ircnick is set to the
         *  nick_server, and serverOrGroup is set to empty.
         *  Behaviour is undefined for serverOrGroup if multiple @ or 0xE120 are found.
         *  @param nick_server A string containting ircnick and possibly servername or server group
         *  @param ircnick This is modified to contain the ircnick
         *  @param serverOrGroup This is modified to contain the servername, servergroup or an empty string.
         */
        static void splitNick_Server(const QString& nick_server, QString &ircnick, QString &serverOrGroup);

        /** Tries to find a nickinfo for a given ircnick on a given ircserver.
         *  @param ircnick The case-insensitive ircnick of the person you want to find.  e.g. "johnflux"
         *  @param serverOrGroup The case-insensitive server name (e.g. "irc.kde.org") or server group name (e.g. "freenode"), or null to search all servers
         *  @return A nickinfo for this user and server if one is found.
         */
        NickInfoPtr getNickInfo(const QString &ircnick, const QString &serverOrGroup);

        bool validateIdentity(IdentityPtr identity, bool interactive=true);

        OSDWidget* osd;

        Konversation::Sound* sound();

        Images* images();

        /* Updates all nick icons according to new theme */
        void updateNickIcons();

        // Returns list of pointers to Servers.
        const QPtrList<Server> getServerList();

        Konversation::NotificationHandler* notificationHandler() const { return m_notificationHandler; }

        // auto replacement for input or output lines
        QString doAutoreplace(const QString& text,bool output);

        int newInstance();

        void delayedConnectToServer(const QString& hostName,
            const QString& port = "6667",
            const QString& channel="",
            const QString& nick = Preferences::nickname(0),
            const QString& password="",
            const bool& useSSL=false
            );

    signals:
        void catchUrl(const QString& who,const QString& url);
        void prefsChanged();
        void closeServerList();

    public slots:
        Server* connectToServerGroup(const QString& serverGroup);
        Server* connectToServer(int serverGroupId, Konversation::ServerSettings quickServer = Konversation::ServerSettings());
        void quickConnectToServer(const QString& hostName,
            const QString& port = "6667",
            const QString& channel="",
            const QString& nick = Preferences::nickname(0),
            const QString& password="",
            const bool& useSSL=false
            );
        void readOptions();
        void saveOptions(bool updateGUI=true);

        void deleteUrl(const QString& who,const QString& url);
        void clearUrlList();
        /** Send a message to all servers that you are away. */
        void toggleAway();
        bool emitDCOPSig(const QString& appId, const QString& objId, const QString& signal, QByteArray& data);

        void dcopConnectToServer(const QString& url, int port, const QString& channel, const QString& password);

    protected slots:
        void openQuickConnectDialog();
        void removeServer(Server* server);

        void dcopMultiServerRaw(const QString &command);
        void dcopRaw(const QString& server, const QString &command);
        void dcopSay(const QString& server,const QString& target,const QString& command);
        void dcopInfo(const QString& string);
        void sendMultiServerCommand(const QString& command, const QString& parameter);

    private:
        QPtrList<Server> serverList;
        QStringList urlList;
        KonvDCOP* dcopObject;
        KonvIdentDCOP* identDCOP;
        KonversationMainWindow* mainWindow;
        Konversation::Sound* m_sound;
        QuickConnectDialog* quickConnectDialog;
        Images* m_images;

        Konversation::NotificationHandler* m_notificationHandler;

        QStringList colorList;

        // For command line arguments
        QString m_hostName;
        QString m_port;
        QString m_channel;
        QString m_nick;
        QString m_password;
        bool m_useSSL;
        bool m_connectDelayed;
};

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
