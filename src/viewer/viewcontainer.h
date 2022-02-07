/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include "mainwindow.h"
#include "common.h"
#include "server.h"

#include <QAbstractItemModel>
#include <QMimeData>
#include <QTabWidget>

class QSplitter;
class QToolButton;

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

class ViewMimeData : public QMimeData
{
    Q_OBJECT

    public:
        explicit ViewMimeData(ChatWindow *view);
        ~ViewMimeData() override;

        ChatWindow* view() const;

    private:
        ChatWindow *m_view;

        Q_DISABLE_COPY(ViewMimeData)
};

class TabWidget : public QTabWidget
{
    Q_OBJECT

    public:
        explicit TabWidget(QWidget* parent = nullptr);
        ~TabWidget() override;

    Q_SIGNALS:
        void contextMenu(QWidget* widget, const QPoint& pos);
        void tabBarMiddleClicked(int index);

    protected:
        void contextMenuEvent(QContextMenuEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        Q_DISABLE_COPY(TabWidget)
};

class ViewContainer : public QAbstractItemModel
{
    Q_OBJECT

    public:
        enum DataRoles {
            ColorRole = Qt::UserRole + 1,
            DisabledRole,
            HighlightRole
        };

        explicit ViewContainer(MainWindow* window);
        ~ViewContainer() override;

        QSplitter* getWidget() const { return m_viewTreeSplitter; }
        MainWindow* getWindow() const { return m_window; }
        KActionCollection* actionCollection() const { return m_window->actionCollection(); }

        QPointer<ChatWindow> getFrontView() const { return m_frontView; }
        Server* getFrontServer() const { return m_frontServer; }

        void prepareShutdown();

        int rowCount(const QModelIndex & parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex indexForView(ChatWindow* view) const;
        QModelIndex parent(const QModelIndex& index) const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        Qt::DropActions supportedDragActions() const override;
        Qt::DropActions supportedDropActions() const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList &indexes) const override;
        bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
        bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

        QString currentViewTitle() const;
        QString currentViewURL(bool passNetwork = true);

        void appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView,
                               const QHash<QString, QString> &messageTags = QHash<QString, QString>(), bool parseURL = true);

        void showQueueTuner(bool);

        int getViewIndex(QWidget* widget) const;
        ChatWindow* getViewAt(int index) const;

        QList<QPair<QString,QString> > getChannelsURI() const;

    public Q_SLOTS:
        void updateAppearance();
        void saveSplitterSizes();
        void setViewTreeShown(bool show = false);

        void updateViews(const Konversation::ServerGroupSettingsPtr &serverGroup = Konversation::ServerGroupSettingsPtr());
        void setViewNotification(ChatWindow* widget, const Konversation::TabNotifyType& type);
        void unsetViewNotification(ChatWindow* view);
        void setUnseenEventsNotification();
        void toggleViewNotifications();
        void toggleAutoJoin();
        void toggleConnectOnStartup();

        void showView(ChatWindow* view);
        void goToView(int page);
        void showNextView();
        void showPreviousView();
        void showNextActiveView();
        void showLastFocusedView();

        bool canMoveViewLeft() const;
        bool canMoveViewRight() const;
        void moveViewLeft();
        void moveViewRight();

        void closeView(int view);
        void closeView(ChatWindow* view);
        void closeViewMiddleClick(int view);
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
        void insertChar(uint chr);
        void insertIRCColor();
        void doAutoReplace();

        void focusInputBox();

        void clearViewLines();
        void insertRememberLine();
        void cancelRememberLine();
        void insertMarkerLine();
        void insertRememberLines(Server* server);

        void resetFrontViewUnseenEventsCount();

        void openLogFile();
        void openLogFile(const QString& caption, const QString& file);

        void addKonsolePanel();

        void zoomIn();
        void zoomOut();
        void resetFont();

        void addUrlCatcher();
        void closeUrlCatcher();

        void toggleDccPanel();
        void addDccPanel();
        void closeDccPanel();
        void deleteDccPanel();
        ChatWindow* getDccPanel() const;

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
        void openChannelList(Server* server = nullptr, const QString& filter = QString(), bool getList = false);

        void openNicksOnlinePanel();
        void closeNicksOnlinePanel();

    Q_SIGNALS:
        void viewChanged(const QModelIndex& idx);
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

    private Q_SLOTS:
        void viewSwitched(int newIndex);
        void onViewTreeDestroyed(QObject *object);

    private:
        void setupTabWidget();
        void setupViewTree();
        void removeViewTree();
        void updateTabWidgetAppearance();

        void addView(ChatWindow* view, const QString& label, bool weinitiated=true);
        int insertIndex(ChatWindow* view);
        void unclutterTabs();

        void updateViewActions(int index);
        void updateFrontView();

        void setFrontServer(Server *);

        void initializeSplitterSizes();

    private:
        bool m_saveSplitterSizesLock;

        MainWindow* m_window;

        QSplitter* m_viewTreeSplitter;
        TabWidget* m_tabWidget;
        QToolButton* m_tabCloseButton;
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

        Q_DISABLE_COPY(ViewContainer)
};

#endif
