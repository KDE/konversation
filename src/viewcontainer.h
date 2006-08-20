/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include <qobject.h>
#include <qguardedptr.h>

#include "konversationmainwindow.h"
#include "common.h"
#include "server.h"

class QSplitter;

class KTabWidget;
class KActionCollection;

class KonversationMainWindow;
class ViewTree;
class ChatWindow;
class Server;
class Images;
class UrlCatcher;
class DccPanel;
class NicksOnline;

namespace Konversation
{
    class InsertCharDialog;
}

class ViewContainer : public QObject
{
    Q_OBJECT

    public:
        ViewContainer(KonversationMainWindow* window);
        ~ViewContainer();

        QSplitter* getWidget() { return m_viewTreeSplitter; }
        KonversationMainWindow* getWindow() { return m_window; }
        KActionCollection* actionCollection() { return m_window->actionCollection(); }

        QGuardedPtr<ChatWindow> getFrontView() { return m_frontView; }
        Server* getFrontServer() { return m_frontServer; }

        void silenceViews();

        void serverQuit(Server* server);

        QString currentViewTitle();
        QString currentViewURL(bool passNetwork);

        void appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView);

    public slots:
        void updateAppearance();
        void saveSplitterSizes();
        void setViewTreeShown(bool show);
        void syncTabBarToTree();

        void updateViews();
        void updateViewIcons();
        void setViewNotification(ChatWindow* widget, const Konversation::TabNotifyType& type);
        void unsetViewNotification(ChatWindow* view);
        void toggleViewNotifications();

        void switchView(QWidget* newView);
        void showView(ChatWindow* view);

        void goToView(int page);
        void showNextView();
        void showPreviousView();
        void moveViewLeft();
        void moveViewRight();

        void closeView(QWidget* view);
        void closeView(ChatWindow* view);
        void closeCurrentView();

        void changeViewCharset(int index);
        void updateViewEncoding(ChatWindow* view);

        void showViewContextMenu(QWidget* tab, const QPoint& pos);

        void clearView();
        void clearAllViews();

        void findText();
        void findNextText();

        void insertCharacter();
        void insertChar(const QChar& chr);
        void insertIRCColor();
        void insertRememberLine();
        void insertRememberLine(Server* server);

        void openLogFile();
        void openLogFile(const QString& caption, const QString& file);

        void addKonsolePanel();
        void closeKonsolePanel(ChatWindow* konsolePanel);

        void addUrlCatcher();
        void closeUrlCatcher();

        void toggleDccPanel();
        void addDccPanel();
        void closeDccPanel();
        void deleteDccPanel();
        DccPanel* getDccPanel();

        void addDccChat(const QString& myNick,const QString& nick,const QString& numericalIp,const QStringList& arguments,bool listen);

        StatusPanel* addStatusView(Server* server);
        RawLog* addRawLog(Server* server);
        void disconnectFrontServer();
        void reconnectFrontServer();
        void serverStateChanged(Server* server, Server::State state);

        Channel* addChannel(Server* server, const QString& name);
        void openChannelSettings();
        void toggleChannelNicklists();

        Query* addQuery(Server* server,const NickInfoPtr & name, bool weinitiated=true);
        void updateQueryChrome(ChatWindow* view, const QString& name);
        void closeQueries();

        ChannelListPanel* addChannelListPanel(Server* server);
        void openChannelList(const QString& filter = QString::null, bool getList = false);

        void openNicksOnlinePanel();
        void closeNicksOnlinePanel();

    signals:
        void viewChanged(ChatWindow* view);
        void removeView(ChatWindow* view);
        void setWindowCaption(const QString& caption);
        void updateChannelAppearance();
        void contextMenuClosed();
        void resetStatusBar();
        void setStatusBarTempText(const QString& text);
        void clearStatusBarTempText();
        void setStatusBarInfoLabel(const QString& text);
        void clearStatusBarInfoLabel();
        void setStatusBarLagLabelShown(bool shown);
        void updateStatusBarLagLabel(Server* server, int msec);
        void resetStatusBarLagLabel();
        void setStatusBarLagLabelTooLongLag(Server* server, int msec);
        void updateStatusBarSSLLabel(Server* server);
        void removeStatusBarSSLLabel();

    private:
        void setupTabWidget();
        void setupViewTree();
        void removeViewTree();
        void updateTabWidgetAppearance();

        void addView(ChatWindow* view, const QString& label, bool weinitiated=true);

        void updateViewActions(int index);
        void updateSwitchViewAction();
        void updateFrontView();

        void initializeSplitterSizes();
        bool m_saveSplitterSizesLock;

        KonversationMainWindow* m_window;

        QSplitter* m_viewTreeSplitter;
        KTabWidget* m_tabWidget;
        ViewTree* m_viewTree;

        Images* images;

        Server* m_frontServer;
        QGuardedPtr<ChatWindow> m_frontView;
        QGuardedPtr<ChatWindow> m_previousFrontView;
        ChatWindow* m_searchView;

        UrlCatcher* m_urlCatcherPanel;
        NicksOnline* m_nicksOnlinePanel;

        DccPanel* m_dccPanel;
        bool m_dccPanelOpen;

        Konversation::InsertCharDialog* m_insertCharDialog;

        int m_popupViewIndex;
        int m_queryViewCount;
};

#endif
