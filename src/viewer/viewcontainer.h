/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include "mainwindow.h"
#include "common.h"
#include "server.h"

#include <QAbstractItemModel>

#include <QTabWidget>

class QSplitter;

class KActionCollection;

class MainWindow;
class ViewTree;
class ChatWindow;
class Server;
class Images;
class UrlCatcher;
class NicksOnline;
class QueueTuner;
class ViewSpringLoader;

namespace Konversation
{
    class InsertCharDialog;
    class ServerGroupSettings;

    namespace DCC
    {
        class Chat;
    }
}

class TabWidget : public QTabWidget
{
    Q_OBJECT

    public:
        explicit TabWidget(QWidget* parent = 0);
        ~TabWidget();

    Q_SIGNALS:
        void removedTab(int index) const;
        void contextMenu(QWidget* widget, const QPoint& pos);

    protected:
        void tabRemoved(int index);

        virtual void contextMenuEvent(QContextMenuEvent* event);
};

class ViewContainer : public QAbstractItemModel
{
    Q_OBJECT

    public:
        enum DataRoles {
            ColorRole = Qt::UserRole + 1
        };

        explicit ViewContainer(MainWindow* window);
        ~ViewContainer();

        QSplitter* getWidget() { return m_viewTreeSplitter; }
        MainWindow* getWindow() { return m_window; }
        KActionCollection* actionCollection() { return m_window->actionCollection(); }

        QPointer<ChatWindow> getFrontView() { return m_frontView; }
        Server* getFrontServer() { return m_frontServer; }

        void prepareShutdown();

        int rowCount(const QModelIndex & parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex& index) const;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

        QString currentViewTitle();
        QString currentViewURL(bool passNetwork = true);

        void appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView,
                               bool parseURL = true);

        void showQueueTuner(bool);

        int getViewIndex(QWidget* widget);
        ChatWindow* getViewAt(int index);

        QList<QPair<QString,QString> > getChannelsURI();

    public slots:
        void updateAppearance();
        void saveSplitterSizes();
        void setViewTreeShown(bool show = false);

        void updateViews(const Konversation::ServerGroupSettingsPtr serverGroup = Konversation::ServerGroupSettingsPtr());
        void setViewNotification(ChatWindow* widget, const Konversation::TabNotifyType& type);
        void unsetViewNotification(ChatWindow* view);
        void toggleViewNotifications();
        void toggleAutoJoin();
        void toggleConnectOnStartup();

        void showView(ChatWindow* view);
        void goToView(int page);
        void showNextView();
        void showPreviousView();
        void showNextActiveView();
        void showLastFocusedView();

        void moveViewLeft();
        void moveViewRight();

        void closeView(QWidget* view);
        void closeView(ChatWindow* view);
        void closeViewMiddleClick(QWidget* view);
        void closeCurrentView();
        void renameKonsole();
        void cleanupAfterClose(ChatWindow* view);

        void changeViewCharset(int index);
        void updateViewEncoding(ChatWindow* view);

        void showViewContextMenu(QWidget* tab, const QPoint& pos);

        void clearView();
        void clearAllViews();

        void findText();
        void findNextText();
        void findPrevText();

        void insertCharacter();
        void insertChar(const QChar& chr);
        void insertIRCColor();
        void doAutoReplace();

        void focusInputBox();

        void clearViewLines();
        void insertRememberLine();
        void cancelRememberLine();
        void insertMarkerLine();
        void insertRememberLines(Server* server);

        void openLogFile();
        void openLogFile(const QString& caption, const QString& file);

        void addKonsolePanel();

        void addUrlCatcher();
        void closeUrlCatcher();

        void toggleDccPanel();
        void addDccPanel();
        void closeDccPanel();
        void deleteDccPanel();
        ChatWindow* getDccPanel();

        void addDccChat(Konversation::DCC::Chat* myNick);

        StatusPanel* addStatusView(Server* server);
        RawLog* addRawLog(Server* server);
        void disconnectFrontServer();
        void reconnectFrontServer();
        void showJoinChannelDialog();
        void connectionStateChanged(Server* server, Konversation::ConnectionState state);
        void channelJoined(Channel* channel);

        Channel* addChannel(Server* server, const QString& name);
        void rejoinChannel();
        void openChannelSettings();
        void toggleChannelNicklists();

        Query* addQuery(Server* server,const NickInfoPtr & name, bool weinitiated=true);
        void updateQueryChrome(ChatWindow* view, const QString& name);
        void closeQueries();

        ChannelListPanel* addChannelListPanel(Server* server);
        void openChannelList(Server* server = 0, const QString& filter = QString(), bool getList = false);

        void openNicksOnlinePanel();
        void closeNicksOnlinePanel();

    signals:
        void viewChanged(const QModelIndex& idx);
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
        void resetStatusBarLagLabel(Server* server);
        void setStatusBarLagLabelTooLongLag(Server* server, int msec);
        void updateStatusBarSSLLabel(Server* server);
        void removeStatusBarSSLLabel();
        void autoJoinToggled(const Konversation::ServerGroupSettingsPtr);
        void autoConnectOnStartupToggled(const Konversation::ServerGroupSettingsPtr);

        void frontServerChanging(Server*);

    private slots:
        void setupIrcContextMenus();
        void viewSwitched(int newIndex);
        void removedTab(int index);
        void movedTab(int from, int to);

    private:
        void setupTabWidget();
        void setupViewTree();
        void removeViewTree();
        void updateTabWidgetAppearance();

        void addView(ChatWindow* view, const QString& label, bool weinitiated=true);

        void updateViewActions(int index);
        void updateFrontView();

        void setFrontServer(Server *);

        void initializeSplitterSizes();
        bool m_saveSplitterSizesLock;

        MainWindow* m_window;

        QSplitter* m_viewTreeSplitter;
        TabWidget* m_tabWidget;
        ViewTree* m_viewTree;
        QWidget* m_vbox;
        QueueTuner* m_queueTuner;

        Images* images;

        QPointer<Server> m_frontServer;
        QPointer<Server> m_contextServer;
        QPointer<ChatWindow> m_frontView;
        QPointer<ChatWindow> m_searchView;

        QPointer<ChatWindow> m_currentView;
        QPointer<ChatWindow> m_lastFocusedView;

        UrlCatcher* m_urlCatcherPanel;
        NicksOnline* m_nicksOnlinePanel;

        ChatWindow* m_dccPanel;
        bool m_dccPanelOpen;

        Konversation::InsertCharDialog* m_insertCharDialog;

        int m_popupViewIndex;
        int m_queryViewCount;

        QList<ChatWindow*> m_activeViewOrderList;

        ViewSpringLoader* m_viewSpringLoader;
};

#endif
