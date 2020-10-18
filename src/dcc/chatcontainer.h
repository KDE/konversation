/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
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
                bool canBeFrontView() const override;
                bool searchView() const override;

                void setChannelEncoding(const QString &encoding) override;
                QString getChannelEncoding() const override;
                QString getChannelEncodingDefaultDesc() const override;
                void emitUpdateInfo() override;

                QString ownNick() const;

            public Q_SLOTS:
                void setPartnerNick(const QString &nick);

                void textEntered();
                void textPasted(const QString &text);

                void receivedLine(const QString &line);
                void chatStatusChanged(Konversation::DCC::Chat *chat, Konversation::DCC::Chat::Status newstatus, Konversation::DCC::Chat::Status oldstatus);

            protected:
                /** Called from ChatWindow adjustFocus */
                void childAdjustFocus() override;

            private Q_SLOTS:
                void upnpError(const QString &errorMessage);

            private:
                QString m_encoding;
                QSplitter *m_headerSplitter;

                Konversation::TopicLabel *m_topicLabel;

                Chat *m_chat;
                WhiteBoard *m_whiteBoard;

                Q_DISABLE_COPY(ChatContainer)
        };
    }
}

#endif // CHATCONTAINER_H
