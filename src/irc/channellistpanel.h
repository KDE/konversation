/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Shows the list of channels
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef CHANNELLISTPANEL_H
#define CHANNELLISTPANEL_H

#include "chatwindow.h"

#include <qtimer.h>


class QCheckBox;
class QStringList;
class QTimer;
class Q3ListView;
class Q3ListViewItem;
class QPushButton;

class K3ListView;
class KLineEdit;

class ChannelListPanel : public ChatWindow
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
        void adjustMinValue(int num);
        void adjustMaxValue(int num);
        void updateNumUsers(const QString& num);
        void updateNumChannels(const QString& num);

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

        void setMinUsers(int num);
        void setMaxUsers(int num);

        void filterTextChanged(const QString& newText);
        void channelTargetClicked();
        void topicTargetClicked();
        void regExpClicked();

        void contextMenu (K3ListView* l, Q3ListViewItem* i, const QPoint& p);
        void openURL();

        //Used to disable functions when not connected
        virtual void serverOnline(bool online);

    protected:

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        virtual bool isInsertCharacterSupported() { return true; }

        int getNumChannels();
        int getNumUsers();
        int getVisibleChannels();
        int getVisibleUsers();

        void setNumChannels(int num);
        void setNumUsers(int num);
        void setVisibleChannels(int num);
        void setVisibleUsers(int num);

        void setChannelTarget(bool state);
        bool getChannelTarget();

        void setTopicTarget(bool state);
        bool getTopicTarget();

        void setRegExp(bool state);
        bool getRegExp();

        int getMinUsers();
        int getMaxUsers();

        const QString& getFilterText();
        void  applyFilterToItem(Q3ListViewItem* item);

        void updateUsersChannels();

        int numChannels;
        int numUsers;
        int visibleChannels;
        int visibleUsers;

        int minUsers;
        int maxUsers;

        bool channelTarget;
        bool topicTarget;

        bool regExp;

        // store channels to be inserted in ListView here first
        QStringList pendingChannels;
        QTimer updateTimer;

        QCheckBox* channelFilter;
        QCheckBox* topicFilter;
        QCheckBox* regexpCheck;

        QPushButton* applyFilter;
        QPushButton* refreshListButton;
        QPushButton* joinChannelButton;

        K3ListView* channelListView;

        KLineEdit* filterInput;

        QString filterText;

        int m_oldSortColumn;
};
#endif
