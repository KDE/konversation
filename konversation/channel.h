/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    channel.h  -  The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
    email:     eisfuchs@tigress.com

    $Id$
*/

#ifndef CHANNEL_H
#define CHANNEL_H

#include <qpushbutton.h>
#include <qwidget.h>
#include <qcheckbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qtimer.h>

#include "nick.h"
#include "nicklistview.h"
#include "chatwindow.h"
#include "server.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "ircinput.h"
#include "nickchangedialog.h"
#include "topiccombobox.h"

/*
  @author Dario Abatianni
*/

class Channel : public ChatWindow
{
  Q_OBJECT

  public:
    Channel(QWidget* parent);
    ~Channel();

    void setName(const QString& newName);
    void setKey(const QString& newKey);
    QString getKey();

    void joinNickname(const QString &nickname,const QString &hostname);
    void renameNick(const QString &nickname,const QString &newName);
    void addNickname(const QString &nickname,const QString &hostmask, bool op, bool voice);
    void removeNick(const QString &nickname, const QString &reason, bool quit);
    void kickNick(const QString &nickname, const QString &kicker, const QString &reason);
    Nick *getNickByName(const QString &lookname);
    QList<Nick> getNickList();

    void adjustNicks(int value);
    void adjustOps(int value);
    void updateNicksOps();

    void setTopic(const QString &topic);
    void setTopic(const QString &nickname, const QString &topic); // Overloaded
    void updateMode(const QString &nick,char mode,bool plus, const QString &parameter);
    void updateModeWidgets(char mode, bool plus, const QString &parameter);
    void updateQuickButtons(QStringList newButtonList);
    void updateFonts();

    void closeYourself();

  signals:
    void newText(QWidget* channel);
    void prefsChanged();
    void sendFile();

  public slots:
    void setNickname(const QString& newNickname);
    void channelTextEntered();
    void sendChannelText(const QString& line);
    void newTextInView();
    void urlCatcher(const QString &url);
    void adjustFocus();
    void showQuickButtons(bool show);
    void showModeButtons(bool show);

  protected slots:
    void completeNick();
    void quickButtonClicked(const QString& definition);
    void modeButtonClicked(int id,bool on);

    void popupCommand(int id);                // Will be connected to NickListView::popupCommand()
    void doubleClickCommand(QListViewItem*);  // Will be connected to NickListView::doubleClicked()
    // Dialogs
    void openNickChangeDialog();
    void changeNickname(const QString& newNickname);
    void closeNickChangeDialog(QSize newSize);
    // will be called when the user types a new topic in the topic line
    void requestNewTopic(const QString& newTopic);
    // connected to IRCInput::textPasted() - used to handle large/multiline pastings
    void textPasted(QString text);
    // connected to IRCInput::sendFile()
    void sendFileMenu();
    void autoUserhost();

  protected:
    QStringList getSelectedNicksList();
    void showEvent(QShowEvent* event);

    int nicks;
    int ops;

    bool quickButtonsChanged;  // to take care of redraw problem if hidden
    bool quickButtonsState;
    bool modeButtonsChanged;  // to take care of redraw problem if hidden
    bool modeButtonsState;
    bool splitterChanged;

    unsigned int completionPosition;

    QSplitter* splitter;
    QString topic; // Caches current topic
    TopicComboBox* topicLine;
    QStringList topicHistory;
    QHBox* modeBox;

    QString key;

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
    QGrid* buttonsGrid;
    IRCInput* channelInput;
    QCheckBox* logCheckBox;

    NickChangeDialog* nickChangeDialog;
    QList<Nick> nicknameList;
    QStringList selectedNicksList;
    QList<QuickButton> buttonList;
    QTimer userhostTimer;
};

#endif
