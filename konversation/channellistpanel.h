/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  channellistpanel.h  -  Shows the list of channels
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef _CHANNELLISTPANEL_H_
#define _CHANNELLISTPANEL_H_

#include "chatwindow.h"

/*
  Dario Abatianni
*/

class QCheckBox;
class QStringList;
class QTimer;

class KListView;

class ChannelListPanel : public ChatWindow
{
  Q_OBJECT

  public:
    ChannelListPanel(QWidget* parent);
    ~ChannelListPanel();

    virtual void closeYourself();

  signals:
    void refreshChannelList();
    void joinChannel(const QString& channelName);
    void adjustMinValue(int num);
    void adjustMaxValue(int num);
    void updateNumUsers(const QString& num);
    void updateNumChannels(const QString& num);

  public slots:
    void adjustFocus();
    void addToChannelList(const QString& channel,int users,const QString& topic);

  protected slots:
    void applyFilterClicked();
    void refreshList();
    void updateDisplay();      // will be called by a timer to update regularly
    void saveList();
    void joinChannelClicked();

    void setMinUsers(int num);
    void setMaxUsers(int num);

    void filterTextChanged(const QString& newText);
    void channelTargetClicked();
    void topicTargetClicked();
    void regExpClicked();

  protected:
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

    KListView* channelListView;

    QString filterText;
};

#endif
