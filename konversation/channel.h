/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    channel.h  -  description
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
    email:     eisfuchs@tigress.com
*/

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qwidget.h>
#include <qcheckbox.h>
#include <qvbox.h>
#include <qlabel.h>

#include <kdialog.h>

#ifndef CHANNEL_H
#define CHANNEL_H

#include "nick.h"
#include "nicklistview.h"
#include "chatwindow.h"
#include "server.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "ircinput.h"

/*
  @author Dario Abatianni
*/

class ChatWindow;

class Channel : public ChatWindow
{
  Q_OBJECT

  public:
//    Channel(Server* server,const QString& channelName,QWidget* parent);
    Channel(QWidget* parent);
    ~Channel();

    QWidget* getChannelPane() { return channelPane; };
    QString& getChannelName() { return channelName; };

    void setChannelName(const QString& newName);

 // Will be inherited from ChatWindow()
//    void setServer(Server* newServer) { server=newServer; };

    void joinNickname(QString& nickname,QString& hostname);
    void renameNick(QString& nickname,QString& newName);
    void addNickname(QString& nickname,QString& hostmask,bool op,bool voice);
    void removeNick(QString& nickname,QString& reason,bool quit);
    void kickNick(QString& nickname,QString& kicker,QString& reason);
    Nick* getNickByName(QString& lookname);

    void adjustNicks(int value);
    void adjustOps(int value);
    void updateNicksOps();

    void setTopic(QString& topic);
    void setTopic(QString& nickname,QString& topic); // Overloaded
    void updateMode(QString& nick,char mode,bool plus,QString& parameter);
    void updateModeWidgets(char mode,bool plus,QString& parameter);
    void updateQuickButtons(QStringList newButtonList);

  signals:
    void newText(QWidget* channel);

  public slots:
    void setNickname(const QString&);
    void channelTextEntered();
    void sendChannelText(const QString& line);
    void newTextInView();
    void urlCatcher(QString url);

  protected slots:
    void completeNick();
    void quickButtonClicked(int id);
    void modeButtonClicked(int id,bool on);
    /* Will be connected to NickListView::popupCommand(int) */
    void popupCommand(int id);

  protected:
    QStringList* getSelectedNicksList();

    int spacing() {  return KDialog::spacingHint(); };
    int margin() {  return KDialog::marginHint(); };

    QString channelName;

    int nicks;
    int ops;

    unsigned int completionPosition;

    QVBox* channelPane;
    QComboBox* topicLine;
    /* TODO: Somehow we need the nickname to the corresponding topic displayed */
    QStringList topicHistory;

    ModeButton* modeT;
    ModeButton* modeN;
    ModeButton* modeS;
    ModeButton* modeI;
    ModeButton* modeP;
    ModeButton* modeM;
    ModeButton* modeK;
    ModeButton* modeL;

    QLineEdit* limit;

    QLabel* nicksOps;
    NickListView* nicknameListView;
    QPushButton* nicknameButton;
    IRCInput* channelInput;
    QCheckBox* logCheckBox;

    QList<Nick> nicknameList;
    QList<QuickButton> buttonList;
};

#endif
