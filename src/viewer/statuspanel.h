/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2003 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef STATUSPANEL_H
#define STATUSPANEL_H

#include "chatwindow.h"

class AwayLabel;
class IRCInput;

class QCheckBox;

class KComboBox;

class StatusPanel : public ChatWindow
{
    Q_OBJECT

    public:
        explicit StatusPanel(QWidget* parent);
        ~StatusPanel() override;

        void cycle() override;

        void setName(const QString& newName) override;

        bool closeYourself(bool askForConfirmation=true) override;
        bool canBeFrontView() override;
        bool searchView() override;

        void setChannelEncoding(const QString& encoding) override;
        QString getChannelEncoding() override;
        QString getChannelEncodingDefaultDesc() override;
        void emitUpdateInfo() override;

        void setServer(Server* newServer) override;

        void setNotificationsEnabled(bool enable) override;

    Q_SIGNALS:
        void sendFile();

    public Q_SLOTS:
        void setNickname(const QString& newNickname);
        void indicateAway(bool show) override;
        void updateAppearance();
        void updateName();

    protected Q_SLOTS:
        void sendFileMenu();
        void statusTextEntered();
        void sendText(const QString& line) override;
        // connected to IRCInput::textPasted() - used for large/multiline pastes
        void textPasted(const QString& text);
        void changeNickname(const QString& newNickname);
        void nicknameComboboxChanged();
        //Used to disable functions when not connected
        void serverOnline(bool online) override;

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;

        bool awayChanged;
        bool awayState;

        void showEvent(QShowEvent* event) override;

        KComboBox* nicknameCombobox;
        AwayLabel* awayLabel;
        QCheckBox* logCheckBox;
        QString oldNick;
};
#endif
