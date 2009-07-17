/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Shows the list of channels

  Copyright (C) 2003 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2009 Travis McHenry <wordsizzle@gmail.com>
*/

#ifndef CHANNELLISTPANEL_H
#define CHANNELLISTPANEL_H

#include "chatwindow.h"
#include "ui_channellistpanelui.h"

#include <QTimer>

class ChannelListItem : public QTreeWidgetItem
{
    public:
        ChannelListItem(QTreeWidget* tree, QStringList & strings);
        ChannelListItem(QTreeWidgetItem* parent, QStringList & strings);
        bool operator<(const QTreeWidgetItem &other) const;
};

class ChannelListPanel : public ChatWindow, private Ui::ChannelListWidgetUI
{
    Q_OBJECT

    public:
        explicit ChannelListPanel(QWidget* parent);
        ~ChannelListPanel();

        using ChatWindow::closeYourself;
        virtual bool closeYourself();
        virtual void emitUpdateInfo();

    signals:
        void refreshChannelList();
        void joinChannel(const QString& channelName);

    public slots:
        void addToChannelList(const QString& channel,int users,const QString& topic);

        virtual void appendInputText(const QString&, bool fromCursor);
        void setFilter(const QString& filter);

        void applyFilterClicked();

    protected slots:
        void refreshList();
        void updateDisplay();                     // will be called by a timer to update regularly
        void saveList();
        void joinChannelClicked();
        void contextMenu(const QPoint& pos);
        void openURL();

        //Used to disable functions when not connected
        virtual void serverOnline(bool online);

    protected:

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus(){};

        virtual bool isInsertCharacterSupported() { return true; }

        void  applyFilterToItem(QTreeWidgetItem* item);

        void updateUsersChannels();

        int m_numChannels;
        int m_numUsers;
        int m_visibleChannels;
        int m_visibleUsers;

        // store channels to be inserted in ListView here first
        QList<QStringList> m_pendingChannels;
        QTimer m_updateTimer;

        int m_oldSortColumn;
        Qt::SortOrder m_oldSortOrder;
};

#endif
