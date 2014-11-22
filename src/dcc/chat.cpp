/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
  Copyright (C) 2004,2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "chat.h"
#include "dcccommon.h"
#include "application.h"
#include "irccharsets.h"
#include "server.h"
#include "upnprouter.h"
#include "transfermanager.h"
#include "connectionmanager.h"

#include <QHostAddress>
#include <QTextCodec>
#include <QTextStream>
#include <QTcpSocket>
#include <QTcpServer>

#include <KMessageBox>
#include <KGuiItem>

using namespace Konversation::UPnP;

namespace Konversation
{
    namespace DCC
    {
        Chat::Chat(QObject *parent)
            : QObject(parent),
              m_selfOpened(true),
              m_dccSocket(0),
              m_dccServer(0),
              m_chatStatus(Configuring),
              m_chatExtension(Unknown)
        {
            qDebug();
            // set default values
            m_reverse = Preferences::self()->dccPassiveSend();

            setEncoding(Konversation::IRCCharsets::self()->encodingForLocale());
        }

        Chat::~Chat()
        {
            close();
        }

        void Chat::close()
        {
            if (!m_dccSocket && !m_dccServer)
            {
                return; //already closed
            }

            emit aboutToClose();

            m_textStream.setDevice(0);

            if (m_dccServer)
            {
                disconnect(m_dccServer, 0, 0, 0);
                m_dccServer->close();

                if (Preferences::self()->dccUPnP())
                {
                    UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                    if (router)
                    {
                        router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                    }
                }
                m_dccServer = 0;
            }

            if (m_dccSocket)
            {
                disconnect(m_dccSocket, 0, 0, 0);
                m_dccSocket->close();
                m_dccSocket = 0;
            }
        }

        void Chat::start()
        {
            Server *server = serverByConnectionId();
            if (!server)
            {
                qDebug() << "could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed(i18nc("%1=dcc extension like Chat or Whiteboard",
                             "Could not send a DCC %1 request to the partner via the IRC server.",
                             localizedExtensionString()));
                return;
            }

            if (m_ownIp.isEmpty())
            {
                m_ownIp = DccCommon::getOwnIp(server);
            }
            qDebug() << "ownip: " << m_ownIp;

            if (m_selfOpened)
            {
                //we started the dcc chat
                if (m_reverse)
                {
                    qDebug() << "passive dcc chat";
                    int token = Application::instance()->getDccTransferManager()->generateReverseTokenNumber();
                    m_token = QString::number(token);
                    qDebug() << "token:" << m_token;
                    server->dccPassiveChatRequest(m_partnerNick, extensionString(), DccCommon::textIpToNumericalIp(m_ownIp), m_token);
                    setStatus(WaitingRemote, i18n("Awaiting acceptance by remote user..."));
                }
                else
                {
                    listenForPartner();
                    if (Preferences::self()->dccUPnP())
                    {
                        UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();

                        if (router && router->forward(QHostAddress(server->getOwnIpByNetworkInterface()), m_ownPort, QAbstractSocket::TcpSocket))
                        {
                            connect(router, &UPnPRouter::forwardComplete, this, &Chat::sendRequest);
                        }
                        else
                        {
                            sendRequest(true, 0); // On error try anyways
                        }
                    }
                    else
                    {
                        sendRequest(false, 0);
                    }
                }
            }
            else
            {
                if (!Preferences::self()->dccChatAutoAccept())
                {
                    int ret = KMessageBox::questionYesNo(0,
                                                         i18nc("%1=partnerNick, %2=Servername, %3=dcc extension as chat or wboard", "%1 (on %2) offers to DCC %3 with you", m_partnerNick, server->getServerName(), localizedExtensionString()),
                                                         i18nc("%1=dcc extension as Chat or Whiteboard, %2=partnerNick", "DCC %1 offer from %2", localizedExtensionString(), m_partnerNick),
                                                         KGuiItem(i18n("Accept")),
                                                         KGuiItem(i18n("Reject"))
                                                         );

                    if (ret == KMessageBox::No)
                    {
                        setStatus(Aborted, i18nc("%1=dcc extension like Chat or Whiteboard",
                                                 "You rejected the DCC %1 offer.",
                                                 localizedExtensionString()));
                        server->dccRejectChat(m_partnerNick, extensionString());
                        return;
                    }
                }

                if (m_reverse)
                {
                    qDebug() << "partner1: passive:1";
                    listenForPartner();
                    if (Preferences::self()->dccUPnP())
                    {
                        UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();

                        if (router && router->forward(QHostAddress(server->getOwnIpByNetworkInterface()), m_ownPort, QAbstractSocket::TcpSocket))
                        {
                            connect(router, &UPnPRouter::forwardComplete, this, &Chat::sendReverseAck);
                        }
                        else
                        {
                            sendReverseAck(true, 0); // On error try anyways
                        }
                    }
                    else
                    {
                        sendReverseAck(false, 0);
                    }
                }
                else
                {
                    connectToPartner();
                }
            }
        }

        void Chat::reject()
        {
            failed(i18nc("%1=dcc extension as Chat or Whiteboard",
                         "DCC %1 request was rejected",
                         localizedExtensionString()));
        }

        void Chat::removedFromView()
        {
            emit removed(this);
        }

        void Chat::setConnectionId(int connectionId)
        {
            m_connectionId = connectionId;
        }

        void Chat::setSelfOpened(bool opened)
        {
            m_selfOpened = opened;
        }

        bool Chat::selfOpened() const
        {
            return m_selfOpened;
        }

        void Chat::setPartnerNick(const QString &partnerNick)
        {
            m_partnerNick = partnerNick;
        }

        void Chat::setOwnNick(const QString &ownNick)
        {
            m_ownNick = ownNick;
        }

        void Chat::setReverse(bool reverse, const QString &token)
        {
            m_reverse = reverse;
            m_token = token;
        }

        void Chat::setPartnerIp(const QString &partnerIP)
        {
            m_partnerIp = partnerIP;
        }

        void Chat::setPartnerPort(quint16 partnerPort)
        {
            m_partnerPort = partnerPort;
        }

        QString Chat::getEncoding() const
        {
            return m_encoding;
        }

        bool Chat::setEncoding(const QString &encoding)
        {
            if (m_encoding != encoding && !m_encoding.isEmpty())
            {
                m_encoding = encoding;
                QTextCodec *codec = QTextCodec::codecForName(m_encoding.toLatin1());
                if (codec)
                {
                    m_textStream.setCodec(codec);
                    return true;
                }
            }
            return false;
        }

        void Chat::setStatus(Chat::Status status, const QString &detailMessage)
        {
            qDebug() << "old: " << m_chatStatus << " != " << status << " :new";
            if (m_chatStatus != status)
            {
                m_chatDetailedStatus = detailMessage;
                emit statusChanged(this, status, m_chatStatus);
                m_chatStatus = status;
            }
        }

        void Chat::sendRequest(bool error, quint16 port)
        {
            Server *server = serverByConnectionId();
            if (!server)
            {
                failed(i18nc("%1=dcc extension like Chat or Whiteboard",
                             "Could not send Reverse DCC %1 acknowledgement to the partner via the IRC server.",
                             localizedExtensionString()));
                return;
            }

            if (Preferences::self()->dccUPnP() && this->sender())
            {
                // Somebody elses forward succeeded
                if (port != m_ownPort)
                {
                    return;
                }

                disconnect(this->sender(), SIGNAL(forwardComplete(bool,quint16)), this, SLOT(sendRequest(bool,quint16)));

                if (error)
                {
                    failedUPnP(i18n("Failed to forward port %1. Sending DCC request to remote user regardless.", QString::number(m_ownPort)));
                }
            }

            QString ownNumericalIp = DccCommon::textIpToNumericalIp(DccCommon::getOwnIp(server));
            server->requestDccChat(m_partnerNick, extensionString(), ownNumericalIp, m_ownPort);
        }

        void Chat::sendReverseAck(bool error, quint16 port)
        {
            Server *server = serverByConnectionId();
            if (!server)
            {
                failed(i18nc("%1=extension like Chat or Whiteboard",
                             "Could not send Reverse DCC %1 acknowledgement to the partner via the IRC server.",
                             localizedExtensionString()));
                return;
            }

            if (Preferences::self()->dccUPnP() && this->sender())
            {
                // Somebody elses forward succeeded
                if (port != m_ownPort)
                {
                    return;
                }

                disconnect(this->sender(), SIGNAL(forwardComplete(bool,quint16)), this, SLOT(sendRequest(bool,quint16)));

                if (error)
                {
                    failedUPnP(i18n("Failed to forward port %1. Sending DCC request to remote user regardless.", QString::number(m_ownPort)));
                }
            }

            server->dccReverseChatAck(m_partnerNick, extensionString(), DccCommon::textIpToNumericalIp(m_ownIp), m_ownPort, m_token);
        }

        void Chat::listenForPartner()
        {
            qDebug() << "[BEGIN]";

            // Set up server socket
            QString failedReason;
            if (Preferences::self()->dccSpecificChatPorts())
            {
                m_dccServer = DccCommon::createServerSocketAndListen(this, &failedReason, Preferences::self()->dccChatPortsFirst(), Preferences::self()->dccChatPortsLast());
            }
            else
            {
                m_dccServer = DccCommon::createServerSocketAndListen(this, &failedReason);
            }

            if (!m_dccServer)
            {
                failed(i18n("Could not open a socket for listening: %1", failedReason));
                return;
            }

            connect(m_dccServer, &QTcpServer::newConnection, this, &Chat::heardPartner);

            // Get our own port number
            m_ownPort = m_dccServer->serverPort();
            qDebug() << "using port: " << m_ownPort ;

            setStatus(Chat::WaitingRemote, i18nc("%1=dcc extension like Chat or Whiteboard,%2=partnerNick, %3=port",
                                                 "Offering DCC %1 connection to %2 on port %3...",
                                                 localizedExtensionString(), m_partnerNick, QString::number(m_ownPort)));

            qDebug() << "[END]";
        }

        Chat::Status Chat::status() const
        {
            return m_chatStatus;
        }

        QString Chat::statusDetails() const
        {
            return m_chatDetailedStatus;
        }

        void Chat::setExtension(const QString& extension)
        {
            QString ext = extension.toLower();
            if (ext == "chat")
            {
                m_chatExtension = SimpleChat;
                return;
            }
            else if (ext == "wboard")
            {
                m_chatExtension = Whiteboard;
                return;
            }
            qDebug() << "unknown chat extension:" << extension;
            m_chatExtension = Unknown;
            return;
        }

        void Chat::setExtension(Extension extension)
        {
            m_chatExtension = extension;
        }

        Chat::Extension Chat::extension() const
        {
            return m_chatExtension;
        }

        QString Chat::extensionString() const
        {
            switch (extension())
            {
                case Whiteboard:
                    return "wboard";
                case SimpleChat:
                default:
                    return "chat";
            }
        }

        QString Chat::localizedExtensionString() const
        {
            switch (extension())
            {
                case Whiteboard:
                    return i18nc("DCC extension", "Whiteboard");
                case SimpleChat:
                default:
                    return i18nc("DCC extension", "Chat");
            }
        }

        int Chat::connectionId() const
        {
            return m_connectionId;
        }

        QString Chat::partnerIp() const
        {
            return m_partnerIp;
        }

        QString Chat::reverseToken() const
        {
            return m_token;
        }

        quint16 Chat::partnerPort() const
        {
            return m_partnerPort;
        }

        void Chat::connectToPartner()
        {
            //qDebug() << "num: " << m_partnerIp;
            //m_partnerIp = DccCommon::numericalIpToTextIp(m_partnerIp);
            qDebug() << "partnerIP: " << m_partnerIp << " partnerport: " << m_partnerPort  << " nick: " << m_partnerNick;

            setStatus(Chat::Connecting, i18nc("%1=extension like Chat or Whiteboard ,%2 = nickname, %3 = IP, %4 = port",
                                              "Establishing DCC %1 connection to %2 (%3:%4)...", localizedExtensionString(),
                                              m_partnerNick, m_partnerIp, QString::number(m_partnerPort)));

            m_dccSocket = new QTcpSocket(this);

            //connect(m_dccSocket, SIGNAL(hostFound()), this, SLOT(lookupFinished()));
            connect(m_dccSocket, &QTcpSocket::connected, this, &Chat::connectionEstablished);
            connect(m_dccSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &Chat::connectionFailed);
            connect(m_dccSocket, &QTcpSocket::readyRead, this, &Chat::readData);
            connect(m_dccSocket, &QTcpSocket::disconnected, this, &Chat::socketClosed);

            m_dccSocket->connectToHost(m_partnerIp, m_partnerPort);
        }

        void Chat::connectionEstablished()
        {
            m_textStream.setDevice(m_dccSocket);
            setStatus(Chat::Chatting, i18nc("%1=extension like Chat or Whiteboard, %2 = partnerNick",
                                            "Established DCC %1 connection to %2.",
                                            localizedExtensionString(), m_partnerNick));
            emit connected();
        }

        void Chat::connectionFailed(QAbstractSocket::SocketError/* error*/)
        {
            setStatus(Chat::Failed, i18n("Socket error: %1", m_dccSocket->errorString()));
            close();
        }

        void Chat::readData()
        {
            char *buffer = 0;
            QString line;
            QTextCodec *codec = m_textStream.codec();
            qint64 available = m_dccSocket->bytesAvailable();
            if (available < 0)
            {
                failed(m_dccSocket->errorString());
                return;
            }

            while (available > 1 && m_dccSocket->canReadLine())
            {
                buffer = new char[available + 1];
                qint64 actual = m_dccSocket->readLine(buffer, available);
                buffer[actual] = 0;
                line = codec->toUnicode(buffer);
                delete[] buffer;

                const QStringList &lines = line.split('\n', QString::SkipEmptyParts);
                foreach (const QString &lin, lines)
                {
                    emit receivedRawLine(lin);
                }
                available = m_dccSocket->bytesAvailable();
            }
        }

        void Chat::sendAction(const QString &action)
        {
            QString line(action);
            OutputFilter::replaceAliases(line);

            static const QString actionText("\x01""ACTION %2\x01");
            sendRawLine(actionText.arg(line));
        }

        void Chat::sendRawLine(const QString &text)
        {
            if (m_dccSocket && m_textStream.device())
            {
                m_textStream << text << endl;
            }
        }

        void Chat::sendText(const QString &text)
        {
            QString line(text);
            OutputFilter::replaceAliases(line);
            sendRawLine(line);
        }

        void Chat::heardPartner()
        {
            m_dccSocket = m_dccServer->nextPendingConnection();

            if(!m_dccSocket)
            {
                failed(i18n("Could not accept the client."));
                return;
            }

            connect(m_dccSocket, &QTcpSocket::readyRead, this, &Chat::readData);
            connect(m_dccSocket, &QTcpSocket::disconnected, this, &Chat::socketClosed);
            connect(m_dccSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &Chat::connectionFailed);

            // the listen socket isn't needed anymore
            disconnect(m_dccServer, 0, 0, 0);
            m_dccServer->close();
            m_dccServer = 0;

            if (Preferences::self()->dccUPnP())
            {
                UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                if (router)
                {
                    router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                }
            }

            m_textStream.setDevice(m_dccSocket);
            setStatus(Chat::Chatting, i18nc("%1=dcc extension as Chat or Whiteboard, %2=partnerNick",
                                            "Established DCC %1 connection to %2.",
                                            localizedExtensionString(), m_partnerNick));
        }

        void Chat::socketClosed()
        {
            setStatus(Chat::Closed, i18n("Connection closed."));
            close();
        }

        quint16 Chat::ownPort() const
        {
            return m_ownPort;
        }

        QString Chat::ownNick() const
        {
            return m_ownNick;
        }

        QString Chat::partnerNick() const
        {
            return m_partnerNick;
        }

        Server *Chat::serverByConnectionId()
        {
            return Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
        }

        void Chat::failed(const QString &description)
        {
            setStatus(Chat::Failed, description);
            close();
        }

        void Chat::failedUPnP(const QString &description)
        {
            emit upnpError(description);
        }
    }
}


