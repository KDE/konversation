/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef CHATCONTAINER_H
#define CHATCONTAINER_H

#include "chatwindow.h"
#include "chat.h"

class QSplitter;

namespace Konversation
{
    class TopicLabel;

    namespace DCC
    {
        class WhiteBoard;

        class ChatContainer : public ChatWindow
        {
            Q_OBJECT

            public:
                ChatContainer(QWidget *parent, Chat *chat);
                ~ChatContainer() override;

            // ChatWindow
                bool closeYourself(bool askForConfirmation=true) override;
                bool canBeFrontView() override;
                bool searchView() override;

                void setChannelEncoding(const QString &encoding) override;
                QString getChannelEncoding() override;
                QString getChannelEncodingDefaultDesc() override;
                void emitUpdateInfo() override;

                QString ownNick() const;

            protected:
                /** Called from ChatWindow adjustFocus */
                void childAdjustFocus() override;

            public Q_SLOTS:
                void setPartnerNick(const QString &nick);

            public Q_SLOTS:
                void textEntered();
                void textPasted(const QString &text);

                void receivedLine(const QString &line);
                void chatStatusChanged(Konversation::DCC::Chat *chat, Konversation::DCC::Chat::Status newstatus, Konversation::DCC::Chat::Status oldstatus);

            protected Q_SLOTS:
                void upnpError(const QString &errorMessage);

            private:
                QString m_encoding;
                QSplitter *m_headerSplitter;

                Konversation::TopicLabel *m_topicLabel;

                Chat *m_chat;
                WhiteBoard *m_whiteBoard;
        };
    }
}

#endif // CHATCONTAINER_H
