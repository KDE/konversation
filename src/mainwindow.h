/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2005 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2005 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2005-2008 Eike Hein <hein@kde.org>
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "common.h"
#include "preferences.h"
#include "ssllabel.h"

#include <KXmlGuiWindow>

#include <QStringList>


class KToggleAction;

class Server;
class KonviSettingsDialog;
class ViewContainer;

namespace Konversation
{
    class ServerListDialog;
    class TrayIcon;
    class StatusBar;
}

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
    friend class ViewContainer;

    public:
        MainWindow();
        ~MainWindow() override;

        ViewContainer* getViewContainer() const { return m_viewContainer; }
        Konversation::TrayIcon* systemTrayIcon() const { return m_trayIcon; }
        bool restore();

        QSize sizeHint() const override;

    Q_SIGNALS:
        void showQuickConnectDialog();
        void nicksNowOnline(Server*);
        void resetFrontViewUnseenEventsCount();
        void endNotification();
        void serverStateChanged(Server* server, Konversation::ConnectionState state);
        void triggerRememberLine();
        void triggerRememberLines(Server*);
        void cancelRememberLine();
        void insertMarkerLine();

    public Q_SLOTS:
        void activateAndRaiseWindow();

        void quitProgram();

        void updateTrayIcon();

        void openServerList();

        void openIdentitiesDialog();
        IdentityPtr editIdentity(const IdentityPtr &identity);

        void setOnlineList(Server* notifyServer,const QStringList& list, bool changed);

    protected:
        void showEvent(QShowEvent* e) override;
        void hideEvent(QHideEvent* e) override;
        void leaveEvent(QEvent* e) override;

        bool queryClose() override;
        bool event(QEvent* e) override;

        void saveProperties(KConfigGroup &config) override;

    private Q_SLOTS:
        /** This is connected to the preferences settingsChanged signal and acts to compress
        *  multiple successively settingsChanged() signals into a single output
        *  appearanceChanged() signal.
        *
        *  Do not connect to the settingsChanged signal elsewhere.  If you want to know when
        *  the settings have changed, connect to:
        *  KonversationApplication::instance(), SIGNAL(appearanceChanged())
        */
        void settingsChangedSlot();

        /** This is connected to the appearanceChanged signal.
        *  @see settingsChangedSlot()
        */
        void resetHasDirtySettings();

        void toggleMenubar(bool dontShowWarning = false);

        void openPrefsDialog();
        void openKeyBindings();
        void openQuickConnectDialog();

        // it seems that moc does not honor #ifs in compile so we create an
        // empty slot in our .cpp file rather than #if this slot out
        void openNotifications();
        void notifyAction(int connectionId,const QString& nick);

        void toggleVisibility();

    private:
        int confirmQuit();

        enum MoveToDesktopMode { NoMoveToDesktop, MoveToDesktop };
        void activateRaiseAndMoveToDesktop(MoveToDesktopMode moveToDesktop);

    private:
        ViewContainer* m_viewContainer;
        Konversation::StatusBar* m_statusBar;
        Konversation::TrayIcon* m_trayIcon;

        KToggleAction* m_showMenuBarAction;

        KonviSettingsDialog *m_settingsDialog;
        Konversation::ServerListDialog* m_serverListDialog;

        /** @see settingsChangedSlot() */
        bool m_hasDirtySettings;
        bool m_closeOnQuitAction;

        Q_DISABLE_COPY(MainWindow)
};

#endif /* MAINWINDOW_H */
