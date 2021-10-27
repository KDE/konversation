/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONJOINCHANNELDIALOG_H
#define KONVERSATIONJOINCHANNELDIALOG_H

#include <QDialog>


#include "ui_joinchannelui.h"

class Server;
class QPushButton;
namespace Konversation
{

    class JoinChannelUI;

    class JoinChannelDialog : public QDialog
    {
        Q_OBJECT
            public:
            explicit JoinChannelDialog(Server* server, QWidget *parent = nullptr);
            ~JoinChannelDialog() override;

            int connectionId() const;
            QString channel() const;
            QString password() const;

        private Q_SLOTS:
            void slotOk();
            void slotNicknameChanged(const QString& nickname);
            void slotConnectionListChanged();
            void slotSelectedConnectionChanged(int);
            void slotChannelChanged(const QString& text);
            void slotChannelHistoryCleared();
            void deleteChannel();

        private:
            Ui::JoinChannelUI m_ui;
            QPushButton *mOkButton;

            Q_DISABLE_COPY(JoinChannelDialog)
    };

}

#endif
