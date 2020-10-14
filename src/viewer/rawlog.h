/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  RawLog.h  -  provides a view to the raw protocol
  begin:     Die M� 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef RAWLOG_H
#define RAWLOG_H

#include "chatwindow.h"


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
};

#endif /* RAWLOG_H */
