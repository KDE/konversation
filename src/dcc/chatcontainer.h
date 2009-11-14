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
class IRCInput;

namespace Konversation
{
    class TopicLabel;

    namespace DCC
    {
        class ChatContainer : public ChatWindow
        {
            Q_OBJECT

            public:
                ChatContainer(QWidget *parent, Chat *chat);
                ~ChatContainer();

            // ChatWindow
                virtual QString getTextInLine();
                virtual bool closeYourself(bool askForConfirmation=true);
                virtual bool canBeFrontView();
                virtual bool searchView();

                virtual void setChannelEncoding(const QString &encoding);
                virtual QString getChannelEncoding();
                virtual QString getChannelEncodingDefaultDesc();
                virtual void emitUpdateInfo();

                virtual bool isInsertSupported();
                QString ownNick() const;

            protected:
                /** Called from ChatWindow adjustFocus */
                virtual void childAdjustFocus();

            public slots:
                void updateAppearance();
                void appendInputText(const QString &text, bool fromCursor);
            // ChatWindow end

            public slots:
                void setPartnerNick(const QString &nick);

            public slots:
                void textEntered();
                void textPasted(const QString &text);

                void receivedLine(const QString &line);
                void chatStatusChanged(Konversation::DCC::Chat *chat, Konversation::DCC::Chat::Status newstatus, Konversation::DCC::Chat::Status oldstatus);

            protected slots:
                void upnpError(const QString &errorMessage);

            private:
                QString m_encoding;
                QSplitter *m_headerSplitter;

                Konversation::TopicLabel *m_topicLabel;
                IRCInput *m_dccChatInput;

                Chat *m_chat;
        };
    }
}

#endif // CHATCONTAINER_H
