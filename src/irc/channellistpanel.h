/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2009 Travis McHenry <wordsizzle@gmail.com>
*/

#ifndef CHANNELLISTPANEL_H
#define CHANNELLISTPANEL_H

#include "chatwindow.h"
#include "ui_channellistpanelui.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class KToolBar;

struct ChannelItem
{
    QString name;
    int users;
    QString topic;
};


/**
 * Shows the list of channels
 */
class ChannelListProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
        explicit ChannelListProxyModel(QObject *parent = nullptr);

        int filterMinimumUsers() const { return m_minUsers; }
        int filterMaximumUsers() const { return m_maxUsers; }
        bool filterTopic() const { return m_filterTopic; }
        bool filterChannel() const { return m_filterChannel; }

    public Q_SLOTS:
        void setFilterMinimumUsers(int users);
        void setFilterMaximumUsers(int users);

        void setFilterTopic(bool filter);
        void setFilterChannel(bool filter);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        bool usersInRange(int users) const;
        int m_minUsers;
        int m_maxUsers;
        bool m_filterTopic;
        bool m_filterChannel;
};

class ChannelListModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        explicit ChannelListModel(QObject* parent);
        ~ChannelListModel() override = default;

        void append(const ChannelItem& item);

        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    private:
        QList<ChannelItem> m_channelList;

        Q_DISABLE_COPY(ChannelListModel)
};

class ChannelListPanel : public ChatWindow, private Ui::ChannelListWidgetUI
{
    Q_OBJECT
    friend class Server;

    public:
        explicit ChannelListPanel(QWidget* parent);
        ~ChannelListPanel() override;

        using ChatWindow::closeYourself;
        virtual bool closeYourself();
        void emitUpdateInfo() override;

        bool isInsertSupported() const override { return true; }
        QString getTextInLine() const override { return m_filterLine->text(); }

    public Q_SLOTS:
        void refreshList();
        void addToChannelList(const QString& channel,int users,const QString& topic);
        void endOfChannelList();
        void applyFilterClicked();

        void appendInputText(const QString&, bool fromCursor) override;
        void setFilter(const QString& filter);

    Q_SIGNALS:
        void refreshChannelList();
        void joinChannel(const QString& channelName);

    protected Q_SLOTS:
        //Used to disable functions when not connected
        void serverOnline(bool online) override;

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus()override {}

    private Q_SLOTS:
        void saveList();

        void filterChanged();
        void updateFilter();

        void updateUsersChannels();
        void currentChanged(const QModelIndex &current,const QModelIndex &previous);
        void setProgress();

        void joinChannelClicked();
        void contextMenu(const QPoint& pos);
        void openURL();

    private:
        void countUsers(const QModelIndex& index, int pos);

    private:
        int m_numChannels;
        int m_numUsers;
        int m_visibleChannels;
        int m_visibleUsers;
        bool m_online;
        bool m_firstRun;
        bool m_regexState;

        QTimer* m_progressTimer;
        QTimer* m_filterTimer;
        QTimer* m_tempTimer;

        ChannelListModel* m_channelListModel;
        ChannelListProxyModel* m_proxyModel;

        KToolBar *m_toolBar;
        QAction *m_saveList;
        QAction *m_refreshList;
        QAction *m_joinChannel;

        Q_DISABLE_COPY(ChannelListPanel)
};

#endif
