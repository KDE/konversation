/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef CHAT_H
#define CHAT_H

#include <QAbstractSocket>

class QTcpSocket;
class QTcpServer;
class Server;

namespace Konversation
{
    namespace DCC
    {
        class Chat : public QObject
        {
            Q_OBJECT

            public:

                enum Status
                {
                    Configuring = 0,                      // Not queud yet (this means that user can't see the item at this time)
                    Queued,                               // Newly added DCC, waiting user's response
                    WaitingRemote,                        // Waiting for remote host's response
                    Connecting,                           // RECV: trying to connect to the server
                    Chatting,
                    Closed,

                    Aborted,
                    Failed
                };

                enum Extension
                {
                    Unknown = 0,
                    SimpleChat,
                    Whiteboard
                };

                explicit Chat(QObject *parent);
                ~Chat();

                quint16 ownPort() const;
                QString ownNick() const;
                QString partnerNick() const;
                quint16 partnerPort() const;
                QString partnerIp() const;
                QString reverseToken() const;

                void setConnectionId(int connectionId);
                void setSelfOpened(bool opened);
                bool selfOpened() const;

                void setPartnerNick(const QString &partnerNick);
                void setReverse(bool reverse, const QString &token);
                void setPartnerIp(const QString &partnerIP);
                void setPartnerPort(quint16 partnerPort);
                void setOwnNick(const QString &ownNick);

                void start();
                void reject();
                void close();

                bool setEncoding(const QString &encoding);
                QString getEncoding() const;

                Status status() const;
                QString statusDetails() const;

                void setExtension(const QString& extension);
                void setExtension(Extension extension);
                Extension extension() const;
                QString extensionString() const;
                QString localizedExtensionString() const;

                int connectionId() const;

                void removedFromView();

                // used public by transfermanager
                void connectToPartner();

            public Q_SLOTS:
                void sendText(const QString &text);
                void sendAction(const QString &action);
                void sendRawLine(const QString &text);

            Q_SIGNALS:
                void statusChanged(Konversation::DCC::Chat *chat, Konversation::DCC::Chat::Status newStatus, Konversation::DCC::Chat::Status oldStatus);
                void removed(Konversation::DCC::Chat *chat);

                void receivedRawLine(const QString &line);

                void aboutToClose();
                void closed();
                void error(QAbstractSocket::SocketError errorCode, const QString &errorMessage);

                void upnpError(const QString &errorMessage);

                void connected();

            protected Q_SLOTS:
                void connectionEstablished();
                void connectionFailed(QAbstractSocket::SocketError errorCode);
                void heardPartner();
                void socketClosed();

                void readData();

                void sendRequest(bool error, quint16 port);
                void sendReverseAck(bool error, quint16 port);

            protected:
                void setStatus(Status status, const QString &detailMessage = QString());

                void listenForPartner();

            private:
                inline Server *serverByConnectionId();
                inline void failed(const QString &description);
                inline void failedUPnP(const QString &description);

                QString m_ownNick;
                quint16 m_ownPort;
                QString m_ownIp;

                QString m_partnerNick;
                QString m_partnerIp;
                quint16 m_partnerPort;
                QString m_token;
                bool m_reverse;
                bool m_selfOpened;

                int m_connectionId;

                QTextStream m_textStream;

                QTcpSocket *m_dccSocket;
                QTcpServer *m_dccServer;

                QString m_encoding;

                Status m_chatStatus;
                QString m_chatDetailedStatus;

                Extension m_chatExtension;
        };
    }
}

#endif
