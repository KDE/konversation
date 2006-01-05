/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The panel where the server status messages go
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
        StatusPanel(QWidget* parent);
        ~StatusPanel();

        virtual QString getTextInLine();
        virtual bool closeYourself();
        virtual bool canBeFrontView();
        virtual bool searchView();

        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();
        virtual void setName(const QString& newName) { ChatWindow::setName(newName); }
        virtual void emitUpdateInfo();

        virtual void setIdentity(const Identity *newIdentity);

        virtual bool areIRCColorsSupported() {return true; }
        virtual bool isInsertCharacterSupported() { return true; }

        signals:
        void sendFile();
        // void prefsChanged();

    public slots:
        void setNickname(const QString& newNickname);
        virtual void indicateAway(bool show);
        void setShowNicknameBox(bool show);
        void updateAppearance();
        virtual void appendInputText(const QString&);

    protected slots:
        void sendFileMenu();
        void statusTextEntered();
        void sendStatusText(const QString& line);
        // connected to IRCInput::textPasted() - used for large/multiline pastes
        void textPasted(const QString& text);
        void changeNickname(const QString& newNickname);
        void nicknameComboboxChanged();
        //Used to disable functions when not connected
        virtual void serverOnline(bool online);

    protected:

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        bool awayChanged;
        bool awayState;

        void showEvent(QShowEvent* event);

        QComboBox* nicknameCombobox;
        QLabel* awayLabel;
        IRCInput* statusInput;
        QCheckBox* logCheckBox;
        QString oldNick;
};
#endif
