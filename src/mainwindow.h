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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "common.h"
#include "preferences.h"
#include "ssllabel.h"

#include <QStringList>

#include <kxmlguiwindow.h>

class QQmlApplicationEngine; // WIPQTQUICK
class QStackedWidget; // WIPQTQUICK

class KToggleAction;

class Server;
class KonviSettingsDialog;
class ViewContainer;
class MessageModel; // WIPQTQUICK
class FilteredMessageModel; // WIPQTQUICK
class FilteredUserModel; // WIPQTQUICK
class IdentityModel; // WIPQTQUICK
class Completer; // WIPQTQUICK

namespace Konversation
{
    class ServerListDialog;
    class TrayIcon;
    class StatusBar;
}

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

    public:
        MainWindow(bool raiseQtQuickUi, const QString& uiPackage = QStringLiteral("default")); // WIPQTQUICK
        ~MainWindow();

        ViewContainer* getViewContainer() { return m_viewContainer; }
        Konversation::TrayIcon* systemTrayIcon() const { return m_trayIcon; }

        MessageModel* getMessageModel() { return m_messageModel; } // WIPQTQUICK
        FilteredMessageModel* getFilteredMessageModel() { return m_filteredMessageModel; } // WIPQTQUICK
        FilteredUserModel* getFilteredUserModel() { return m_filteredUserModel; } // WIPQTQUICK
        bool loadUiPackage(const QString &packageName, bool raise = false);
        bool reloadUiPackage();

        /** Some errors need to be shown, even when konversation is minimized.
         */
        void focusAndShowErrorMessage(const QString &errorMsg);

    Q_SIGNALS:
        void showQuickConnectDialog();
        void nicksNowOnline(Server*);
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
        IdentityPtr editIdentity(IdentityPtr identity);

        void setOnlineList(Server* notifyServer,const QStringList& list, bool changed);

    protected Q_SLOTS:
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
        void showMenubar(bool show); // WIPQTQUICK

        void openPrefsDialog();
        void openKeyBindings();
        void openQuickConnectDialog();

        // it seems that moc does not honor #ifs in compile so we create an
        // empty slot in our .cpp file rather than #if this slot out
        void openNotifications();
        void notifyAction(int connectionId,const QString& nick);

        void toggleVisibility();

        void showEvent(QShowEvent* e) Q_DECL_OVERRIDE;
        void hideEvent(QHideEvent* e) Q_DECL_OVERRIDE;
        void leaveEvent(QEvent* e) Q_DECL_OVERRIDE;


    protected:
        QSize sizeHint() const Q_DECL_OVERRIDE;

        int confirmQuit();
        bool queryClose() Q_DECL_OVERRIDE;
        bool event(QEvent* e) Q_DECL_OVERRIDE;

        ViewContainer* m_viewContainer;
        Konversation::StatusBar* m_statusBar;
        Konversation::TrayIcon* m_trayIcon;

        KToggleAction* m_showMenuBarAction;

        KonviSettingsDialog *m_settingsDialog;
        Konversation::ServerListDialog* m_serverListDialog;

        MessageModel *m_messageModel; // WIPQTQUICK
        FilteredMessageModel *m_filteredMessageModel; // WIPQTQUICK
        FilteredUserModel *m_filteredUserModel; // WIPQTQUICK
        IdentityModel *m_identityModel;
        Completer *m_completer; // WIPQTQUICK
        QQmlApplicationEngine *m_qmlEngine; // WIPQTQUICK
        QString m_currentUiPackage; // WIPQTQUICK
        QStackedWidget *m_uiStack; // WIPQTQUICK

        /** @see settingsChangedSlot() */
        bool m_hasDirtySettings;
        bool m_closeApp;
};

#endif /* MAINWINDOW_H */
