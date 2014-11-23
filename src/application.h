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
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QUrl>
#include <QNetworkConfigurationManager>

#include "preferences.h"
#include "mainwindow.h"
#include "server.h"
#include "osd.h"
#include "identity.h"
#include "ircqueue.h"

#include <QApplication>

class ConnectionManager;
class AwayManager;
class ScriptLauncher;
class Server;
class QuickConnectDialog;
class Images;
class ServerGroupSettings;
class QStandardItemModel;
class QCommandLineParser;

class KTextEdit;

namespace Konversation
{
    class DBus;
    class IdentDBus;
    class Sound;
    class NotificationHandler;

    namespace DCC
    {
        class TransferManager;
    }
}

namespace KWallet
{
    class Wallet;
}


class Application : public QApplication
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
        MainWindow* getMainWindow() { return mainWindow; }

        ConnectionManager* getConnectionManager() { return m_connectionManager; }
        AwayManager* getAwayManager() { return m_awayManager; }
        ScriptLauncher* getScriptLauncher() { return m_scriptLauncher; }
        Konversation::DCC::TransferManager* getDccTransferManager() { return m_dccTransferManager; }

        // HACK
        void showQueueTuner(bool);

        // URL-Catcher
        QStandardItemModel* getUrlModel() { return m_urlModel; }

        Application(int &argc, char **argv);
        ~Application();

        static Application* instance();

        /** For D-Bus, a user can be specified as user@irc.server.net
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

        OSDWidget* osd;

        Konversation::Sound* sound();

        IRCQueue::EmptyingRate staticrates[Server::_QueueListSize];

        Images* images() { return m_images; }

        Konversation::NotificationHandler* notificationHandler() const { return m_notificationHandler; }

        // auto replacement for input or output lines
        QPair<QString, int> doAutoreplace(const QString& text, bool output, int cursorPos = -1);

        // inline auto replacement for input lines
        void doInlineAutoreplace(KTextEdit* textEdit);

        void newInstance(QCommandLineParser *args);

        static void openUrl(const QString& url);

        /// The wallet used to store passwords. Opens the wallet if it's closed.
        KWallet::Wallet* wallet();

        void abortScheduledRestart() { m_restartScheduled = false; }

    Q_SIGNALS:
        void serverGroupsChanged(const Konversation::ServerGroupSettingsPtr serverGroup);
        void appearanceChanged(); // FIXME TODO: Rather than relying on this catch-all, consumers should be rewritten to catch appropriate QEvents.

    public Q_SLOTS:
        void restart();

        void readOptions();
        void saveOptions(bool updateGUI=true);

        void fetchQueueRates(); ///< on Application::readOptions()
        void stashQueueRates(); ///< on application exit
        void resetQueueRates(); ///< when QueueTuner says to
        int countOfQueues() { return Server::_QueueListSize-1; }

        void prepareShutdown();

        void storeUrl(const QString& origin, const QString& newUrl, const QDateTime& dateTime);

    protected Q_SLOTS:
        void openQuickConnectDialog();

        void dbusMultiServerRaw(const QString &command);
        void dbusRaw(const QString& connection, const QString &command);
        void dbusSay(const QString& connection, const QString& target, const QString& command);
        void dbusInfo(const QString& string);
        void sendMultiServerCommand(const QString& command, const QString& parameter);

        void updateProxySettings();

        void closeWallet();

    protected:
        bool event(QEvent* event);

    private:
        void implementRestart();

        ConnectionManager* m_connectionManager;
        AwayManager* m_awayManager;
        Konversation::DCC::TransferManager* m_dccTransferManager;
        ScriptLauncher* m_scriptLauncher;
        QStandardItemModel* m_urlModel;
        Konversation::DBus* dbusObject;
        Konversation::IdentDBus* identDBus;
        QPointer<MainWindow> mainWindow;
        Konversation::Sound* m_sound;
        QuickConnectDialog* quickConnectDialog;
        Images* m_images;
        bool m_restartScheduled;

        Konversation::NotificationHandler* m_notificationHandler;

        KWallet::Wallet* m_wallet;

        QNetworkConfigurationManager* m_networkConfigurationManager;
};

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
