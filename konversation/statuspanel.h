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

#include <qstring.h>

/*
  @author Dario Abatianni
*/

class QPushButton;
class QCheckBox;
class QLabel;
class QComboBox;

class IRCInput;
class NickChangeDialog;

class StatusPanel : public ChatWindow
{
  Q_OBJECT

  public:
#ifdef USE_MDI
    StatusPanel(QString caption);
#else
    StatusPanel(QWidget* parent);
#endif
    ~StatusPanel();

    virtual QString getTextInLine();
    virtual void closeYourself();
    virtual bool frontView();
    virtual bool searchView();
    
    virtual void setChannelEncoding(const QString& encoding);
    virtual QString getChannelEncoding();
    virtual QString getChannelEncodingDefaultDesc();

  signals:
    void newText(QWidget* widget,const QString& highlightColor,bool important);
    void sendFile();
    void prefsChanged();

  public slots:
    void setNickname(const QString& newNickname);
    void newTextInView(const QString& highlightColor,bool important);
    void updateFonts();
    virtual void indicateAway(bool show);

    virtual void appendInputText(const QString&);
    
  protected slots:
    void sendFileMenu();
    void statusTextEntered();
    void sendStatusText(const QString& line);
    // connected to IRCInput::textPasted() - used for large/multiline pastes
    void textPasted(QString text);
    void changeNickname(const QString& newNickname);
    void nicknameComboboxChanged(int index);

  protected:

    /** Called from ChatWindow adjustFocus */
    virtual void childAdjustFocus();
    virtual bool areIRCColorsSupported() {return true; }
    virtual bool isInsertCharacterSupported() { return true; }
    
    bool awayChanged;
    bool awayState;

    void showEvent(QShowEvent* event);
#ifdef USE_MDI
    virtual void closeYourself(ChatWindow* view);
#endif

    QComboBox* nicknameCombobox;
    QLabel* awayLabel;
    IRCInput* statusInput;
    QCheckBox* logCheckBox;
    QString oldNick;
};

#endif
