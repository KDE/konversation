/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef STATUSPANEL_H
#define STATUSPANEL_H

#include "chatwindow.h"

class AwayLabel;
class IRCInput;


class KComboBox;

class StatusPanel : public ChatWindow
{
    Q_OBJECT
    friend class Server;

    public:
        explicit StatusPanel(QWidget* parent);
        ~StatusPanel() override;

        void cycle() override;

        void setName(const QString& newName) override;

        bool closeYourself(bool askForConfirmation=true) override;
        bool canBeFrontView() const override;
        bool searchView() const override;

        void setChannelEncoding(const QString& encoding) override;
        QString getChannelEncoding() const override;
        QString getChannelEncodingDefaultDesc() const override;
        void emitUpdateInfo() override;

        void setServer(Server* newServer) override;

        void setNotificationsEnabled(bool enable) override;

    Q_SIGNALS:
        void sendFile();

    public Q_SLOTS:
        void setNickname(const QString& newNickname);
        void indicateAway(bool show) override;
        void updateAppearance() override;
        void updateName();

    protected Q_SLOTS:
        void sendText(const QString& line) override;
        //Used to disable functions when not connected
        void serverOnline(bool online) override;

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;
        void showEvent(QShowEvent* event) override;

    private Q_SLOTS:
        void sendFileMenu();
        void statusTextEntered();
        // connected to IRCInput::textPasted() - used for large/multiline pastes
        void textPasted(const QString& text);
        void changeNickname(const QString& newNickname);
        void nicknameComboboxChanged();

    private:
        bool awayChanged;
        bool awayState;

        KComboBox* nicknameCombobox;
        AwayLabel* awayLabel;
        QString oldNick;

        Q_DISABLE_COPY(StatusPanel)
};

#endif
