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

        void cycle() Q_DECL_OVERRIDE;

        void setName(const QString& newName) Q_DECL_OVERRIDE;

        bool closeYourself(bool askForConfirmation=true) Q_DECL_OVERRIDE;
        bool canBeFrontView() Q_DECL_OVERRIDE;
        bool searchView() Q_DECL_OVERRIDE;

        void setChannelEncoding(const QString& encoding) Q_DECL_OVERRIDE;
        QString getChannelEncoding() Q_DECL_OVERRIDE;
        QString getChannelEncodingDefaultDesc() Q_DECL_OVERRIDE;
        void emitUpdateInfo() Q_DECL_OVERRIDE;

        void setServer(Server* newServer) Q_DECL_OVERRIDE;

        void setNotificationsEnabled(bool enable) Q_DECL_OVERRIDE;

    Q_SIGNALS:
        void sendFile();

    public Q_SLOTS:
        void setNickname(const QString& newNickname);
        void indicateAway(bool show) Q_DECL_OVERRIDE;
        void updateAppearance();
        void updateName();

    protected Q_SLOTS:
        void sendFileMenu();
        void statusTextEntered();
        void sendText(const QString& line) Q_DECL_OVERRIDE;
        void changeNickname(const QString& newNickname);
        void nicknameComboboxChanged();
        //Used to disable functions when not connected
        void serverOnline(bool online) Q_DECL_OVERRIDE;

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() Q_DECL_OVERRIDE;

        bool awayChanged;
        bool awayState;

        void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

        KComboBox* nicknameCombobox;
        AwayLabel* awayLabel;
        QCheckBox* logCheckBox;
        QString oldNick;
};
#endif
