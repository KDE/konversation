/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
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
            explicit JoinChannelDialog(Server* server, QWidget *parent = 0);
            ~JoinChannelDialog();

            int connectionId() const;
            QString channel() const;
            QString password() const;

        protected Q_SLOTS:
            virtual void slotOk();
            void slotNicknameChanged(const QString& nickname);
            void slotConnectionListChanged();
            void slotSelectedConnectionChanged(int);
            void slotChannelChanged(const QString& text);
            void slotChannelHistoryCleared();

        private:
            Ui::JoinChannelUI m_ui;
            QPushButton *mOkButton;
    };

}
#endif
