/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2005 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2005 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2005-2008 Eike Hein <hein@kde.org>
*/

#ifndef APPLICATION_H
#define APPLICATION_H


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
    class LauncherEntryHandler;

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
    friend class ConnectionManager;

    public:
        /** This function in general shouldn't be called, because in the future there
         *  may be multiple windows.
         *  However, in some situations we have messageboxes that aren't targeted for
         *  any particular main window, such as general errors from dcop calls.
         *
         *  Note to any MDI developer - get this to return any of the windows, or some
         *  'main' one.
         */
        MainWindow* getMainWindow() const { return mainWindow; }

        ConnectionManager* getConnectionManager() const { return m_connectionManager; }
        AwayManager* getAwayManager() const { return m_awayManager; }
        ScriptLauncher* getScriptLauncher() const { return m_scriptLauncher; }
        Konversation::DCC::TransferManager* getDccTransferManager() const { return m_dccTransferManager; }

        // HACK
        void showQueueTuner(bool);

        // URL-Catcher
        QStandardItemModel* getUrlModel() const { return m_urlModel; }

        Application(int &argc, char **argv);
        ~Application() override;

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
         *  @param serverOrGroup The case-insensitive server name (e.g. "irc.kde.org") or server group name (e.g. "libera"), or null to search all servers
         *  @return A nickinfo for this user and server if one is found.
         */
        NickInfoPtr getNickInfo(const QString &ircnick, const QString &serverOrGroup);

        Konversation::Sound* sound() const;

        OSDWidget* osd() const;

        IRCQueue::EmptyingRate staticrates[Server::_QueueListSize];

        Images* images() const { return m_images; }

        Konversation::NotificationHandler* notificationHandler() const { return m_notificationHandler; }
        Konversation::LauncherEntryHandler* launcherEntryHandler() const { return m_launcherEntryHandler; }

        // auto replacement for input or output lines
        QPair<QString, int> doAutoreplace(const QString& text, bool output, int cursorPos = -1) const;

        // inline auto replacement for input lines
        void doInlineAutoreplace(KTextEdit* textEdit) const;

        void newInstance(QCommandLineParser *args);
        void restoreInstance();

        static void openUrl(const QString& url);

        /// The wallet used to store passwords. Opens the wallet if it's closed.
        KWallet::Wallet* wallet();

        void abortScheduledRestart() { m_restartScheduled = false; }

        /// The command line parser is needed for handling parsing arguments on new activations.
        void setCommandLineParser(QCommandLineParser *parser) { m_commandLineParser = parser; }
        QCommandLineParser *commandLineParser() const { return m_commandLineParser; }

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

        void handleActivate(const QStringList& arguments);
        void handleOpen(const QList<QUrl>& urls);

    protected:
        bool event(QEvent* event) override;

    private Q_SLOTS:
        void openQuickConnectDialog();

        void dbusMultiServerRaw(const QString &command);
        void dbusRaw(const QString& connection, const QString &command);
        void dbusSay(const QString& connection, const QString& target, const QString& command);
        void dbusInfo(const QString& string);
        void sendMultiServerCommand(const QString& command, const QString& parameter);

        void updateProxySettings();

        void closeWallet();

    private:
        void implementRestart();
        void activateForStartLikeCall();

        enum AutoConnectMode { NoAutoConnect, AutoConnect };
        enum WindowRestoreMode { NoWindowRestore, WindowRestore };
        void createMainWindow(AutoConnectMode autoConnectMode, WindowRestoreMode restoreMode);

    private:
        ConnectionManager* m_connectionManager;
        AwayManager* m_awayManager;
        Konversation::DCC::TransferManager* m_dccTransferManager;
        ScriptLauncher* m_scriptLauncher;
        QStandardItemModel* m_urlModel;
        Konversation::DBus* dbusObject;
        Konversation::IdentDBus* identDBus;
        QPointer<MainWindow> mainWindow;
        OSDWidget* m_osd;
        mutable Konversation::Sound* m_sound;
        QuickConnectDialog* quickConnectDialog;
        Images* m_images;
        bool m_restartScheduled;

        Konversation::NotificationHandler* m_notificationHandler;
        Konversation::LauncherEntryHandler* m_launcherEntryHandler;

        KWallet::Wallet* m_wallet;
        QCommandLineParser *m_commandLineParser;
        QStringList m_restartArguments;

        Q_DISABLE_COPY(Application)
};

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
