/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef RAWLOG_H
#define RAWLOG_H

#include "chatwindow.h"

/**
 * Provides a view to the raw protocol
 */
class RawLog : public ChatWindow
{
    Q_OBJECT
    friend class Server;

    public:
        enum MessageDirection { Inbound, Outbound };

        explicit RawLog(QWidget* parent);
        ~RawLog() override;

        using ChatWindow::closeYourself;
        virtual bool closeYourself();
        bool searchView() const override;
        bool log() const override;
        using ChatWindow::appendRaw;
        virtual void appendRaw(MessageDirection dir, const QByteArray& message);
    public Q_SLOTS:
        void morphNotification();

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;

    private:
        Q_DISABLE_COPY(RawLog)
};

#endif /* RAWLOG_H */
