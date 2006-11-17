/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#ifndef DCCCHAT_H
#define DCCCHAT_H

#include "chatwindow.h"

/*
 @author Dario Abatianni
*/

class IRCInput;
class KLineEdit;
class Server;

namespace KNetwork
{
    class KServerSocket;
    class KStreamSocket;
}

namespace Konversation
{
    class TopicLabel;
}

class DccChat : public ChatWindow
{
    Q_OBJECT

        public:
        DccChat(QWidget* parent, const QString& myNickname,const QString& nickname,const QStringList& parameters,bool listen);
        ~DccChat();

        virtual QString getTextInLine();
        virtual bool closeYourself();
        virtual bool canBeFrontView();
        virtual bool searchView();

        int getPort();

        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();

        virtual bool isInsertSupported() { return true; }

        QString getMyNick() { return m_myNick; }

    public slots:
        void appendInputText(const QString& s, bool fromCursor);
        void updateAppearance();

    protected slots:
        void lookupFinished();
        void dccChatConnectionSuccess();
        void dccChatBroken(int error);
        void readData();
        void dccChatTextEntered();
        void sendDccChatText(const QString& sendLine);
        void textPasted(const QString& text);
        void heardPartner();
        void socketClosed();

    protected:
        void listenForPartner();
        void connectToPartner();

        virtual void showEvent(QShowEvent* event);

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        QString m_myNick;
        QString m_partnerNick;
        QString host;

        //QString m_ip;
        int m_port;

        QSplitter* m_headerSplitter;
        Konversation::TopicLabel* m_sourceLine;
        IRCInput* m_dccChatInput;
        KNetwork::KStreamSocket* m_dccSocket;
        KNetwork::KServerSocket* m_listenSocket;

        QString m_encoding;

        bool m_initialShow;
};
#endif
