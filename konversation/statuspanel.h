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

  $Id$
*/

#ifndef STATUSPANEL_H
#define STATUSPANEL_H

#include <qpushbutton.h>
#include <qcheckbox.h>

#include "chatwindow.h"
#include "ircinput.h"

/*
  @author Dario Abatianni
*/

class StatusPanel : public ChatWindow
{
  Q_OBJECT

  public: 
    StatusPanel(QWidget* parent);
    ~StatusPanel();

  signals:
    void newText(QWidget* widget);
    void sendFile();
    
  public slots:
    void adjustFocus();
    void setNickname(const QString& newNickname);
    void newTextInView();
    void updateFonts();

  protected slots:
    void sendFileMenu();
    void statusTextEntered();
    // connected to IRCInput::textPasted() - used for large/multiline pastes
    void textPasted(QString text);

  protected:
    void sendStatusText(QString line);

    QPushButton* nicknameButton;
    IRCInput* statusInput;
    QCheckBox* logCheckBox;
};

#endif
