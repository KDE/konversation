/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  statuspanel.h  -  The panel where the server status messages go
  begin:     Sam Jan 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef STATUSPANEL_H
#define STATUSPANEL_H

#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

class QPushButton;
class QCheckBox;
class QLabel;

class IRCInput;

class StatusPanel : public ChatWindow
{
  Q_OBJECT

  public: 
    StatusPanel(QWidget* parent);
    ~StatusPanel();

    virtual QString getTextInLine();
    virtual void closeYourself();
    virtual bool frontView();
    virtual bool searchView();

  signals:
    void newText(QWidget* widget,const QString& highlightColor);
    void sendFile();

  public slots:
    void adjustFocus();
    void setNickname(const QString& newNickname);
    void newTextInView(const QString& highlightColor);
    void updateFonts();
    virtual void indicateAway(bool show);

  protected slots:
    void sendFileMenu();
    void statusTextEntered();
    // connected to IRCInput::textPasted() - used for large/multiline pastes
    void textPasted(QString text);

  protected:
    bool awayChanged;
    bool awayState;

    void showEvent(QShowEvent* event);
    void sendStatusText(QString line);

    QPushButton* nicknameButton;
    QLabel* awayLabel;
    IRCInput* statusInput;
    QCheckBox* logCheckBox;
};

#endif
