/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2004, 2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "chat.h"

#include "dcccommon.h"
#include "application.h"
#include "irccharsets.h"
#include "server.h"
#include "upnprouter.h"
#include "transfermanager.h"
#include "connectionmanager.h"
#include "konversation_log.h"

#include <QHostAddress>
#include <QTextCodec>
#include <QTcpSocket>
#include <QTcpServer>

#include <kwidgetsaddons_version.h>
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
              m_codec(QTextCodec::codecForLocale()),
              m_dccSocket(nullptr),
              m_dccServer(nullptr),
              m_chatStatus(Configuring),
              m_chatExtension(Unknown)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
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

            Q_EMIT aboutToClose();

            if (m_dccServer)
            {
                disconnect(m_dccServer, nullptr, nullptr, nullptr);
                m_dccServer->close();

                if (Preferences::self()->dccUPnP())
                {
                    UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                    if (router)
                    {
                        router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                    }
                }
                m_dccServer = nullptr;
            }

            if (m_dccSocket)
            {
                disconnect(m_dccSocket, nullptr, nullptr, nullptr);
                m_dccSocket->close();
                m_dccSocket = nullptr;
            }
        }

        void Chat::start()
        {
            Server *server = serverByConnectionId();
            if (!server)
            {
                qCDebug(KONVERSATION_LOG) << "could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed(i18nc("%1=dcc extension like Chat or Whiteboard",
                             "Could not send a DCC %1 request to the partner via the IRC server.",
                             localizedExtensionString()));
                return;
            }

            if (m_ownIp.isEmpty())
            {
                m_ownIp = DccCommon::getOwnIp(server);
            }
            qCDebug(KONVERSATION_LOG) << "ownip: " << m_ownIp;

            if (m_selfOpened)
            {
                //we started the dcc chat
                if (m_reverse)
                {
                    qCDebug(KONVERSATION_LOG) << "passive dcc chat";
                    int token = Application::instance()->getDccTransferManager()->generateReverseTokenNumber();
                    m_token = QString::number(token);
                    qCDebug(KONVERSATION_LOG) << "token:" << m_token;
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
                    int ret = KMessageBox::questionTwoActions(nullptr,
                                                         i18nc("%1=partnerNick, %2=Servername, %3=dcc extension as chat or wboard", "%1 (on %2) offers to DCC %3 with you", m_partnerNick, server->getServerName(), localizedExtensionString()),
                                                         i18nc("%1=dcc extension as Chat or Whiteboard, %2=partnerNick", "DCC %1 offer from %2", localizedExtensionString(), m_partnerNick),
                                                         KGuiItem(i18n("Accept"), QStringLiteral("dialog-ok")),
                                                         KGuiItem(i18n("Reject"), QStringLiteral("dialog-cancel"))
                                                         );

                    if (ret == KMessageBox::SecondaryAction)
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
                    qCDebug(KONVERSATION_LOG) << "partner1: passive:1";
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
            Q_EMIT removed(this);
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
            // TODO: m_encoding is empty by default, so this condition will never be true.
            // Logic like this since code was added, needs investigation by someone (tm)
            if (m_encoding != encoding && !m_encoding.isEmpty())
            {
                m_encoding = encoding;
                QTextCodec *codec = QTextCodec::codecForName(m_encoding.toLatin1());
                if (codec)
                {
                    m_codec = codec;
                    return true;
                }
            }
            return false;
        }

        void Chat::setStatus(Chat::Status status, const QString &detailMessage)
        {
            qCDebug(KONVERSATION_LOG) << "old: " << m_chatStatus << " != " << status << " :new";
            if (m_chatStatus != status)
            {
                m_chatDetailedStatus = detailMessage;
                Q_EMIT statusChanged(this, status, m_chatStatus);
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

                auto* router = qobject_cast<UPnPRouter*>(this->sender());
                disconnect(router, &UPnPRouter::forwardComplete, this, &Chat::sendRequest);

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

                auto* router = qobject_cast<UPnPRouter*>(this->sender());
                disconnect(router, &UPnPRouter::forwardComplete, this, &Chat::sendReverseAck);

                if (error)
                {
                    failedUPnP(i18n("Failed to forward port %1. Sending DCC request to remote user regardless.", QString::number(m_ownPort)));
                }
            }

            server->dccReverseChatAck(m_partnerNick, extensionString(), DccCommon::textIpToNumericalIp(m_ownIp), m_ownPort, m_token);
        }

        void Chat::listenForPartner()
        {
            qCDebug(KONVERSATION_LOG) << "[BEGIN]";

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
            qCDebug(KONVERSATION_LOG) << "using port: " << m_ownPort ;

            setStatus(Chat::WaitingRemote, i18nc("%1=dcc extension like Chat or Whiteboard,%2=partnerNick, %3=port",
                                                 "Offering DCC %1 connection to %2 on port %3...",
                                                 localizedExtensionString(), m_partnerNick, QString::number(m_ownPort)));

            qCDebug(KONVERSATION_LOG) << "[END]";
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
            if (ext == QLatin1String("chat"))
            {
                m_chatExtension = SimpleChat;
                return;
            }
            else if (ext == QLatin1String("wboard"))
            {
                m_chatExtension = Whiteboard;
                return;
            }
            qCDebug(KONVERSATION_LOG) << "unknown chat extension:" << extension;
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
                    return QStringLiteral("wboard");
                case SimpleChat:
                default:
                    return QStringLiteral("chat");
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
            //qCDebug(KONVERSATION_LOG) << "num: " << m_partnerIp;
            //m_partnerIp = DccCommon::numericalIpToTextIp(m_partnerIp);
            qCDebug(KONVERSATION_LOG) << "partnerIP: " << m_partnerIp << " partnerport: " << m_partnerPort  << " nick: " << m_partnerNick;

            setStatus(Chat::Connecting, i18nc("%1=extension like Chat or Whiteboard ,%2 = nickname, %3 = IP, %4 = port",
                                              "Establishing DCC %1 connection to %2 (%3:%4)...", localizedExtensionString(),
                                              m_partnerNick, m_partnerIp, QString::number(m_partnerPort)));

            m_dccSocket = new QTcpSocket(this);

            //connect(m_dccSocket, SIGNAL(hostFound()), this, SLOT(lookupFinished()));
            connect(m_dccSocket, &QTcpSocket::connected, this, &Chat::connectionEstablished);
            connect(m_dccSocket, &QTcpSocket::errorOccurred, this, &Chat::connectionFailed);
            connect(m_dccSocket, &QTcpSocket::readyRead, this, &Chat::readData);
            connect(m_dccSocket, &QTcpSocket::disconnected, this, &Chat::socketClosed);

            m_dccSocket->connectToHost(m_partnerIp, m_partnerPort);
        }

        void Chat::connectionEstablished()
        {
            setStatus(Chat::Chatting, i18nc("%1=extension like Chat or Whiteboard, %2 = partnerNick",
                                            "Established DCC %1 connection to %2.",
                                            localizedExtensionString(), m_partnerNick));
            Q_EMIT connected();
        }

        void Chat::connectionFailed(QAbstractSocket::SocketError/* error*/)
        {
            setStatus(Chat::Failed, i18n("Socket error: %1", m_dccSocket->errorString()));
            close();
        }

        void Chat::readData()
        {
            char *buffer = nullptr;
            QString line;
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
                line = m_codec->toUnicode(buffer);
                delete[] buffer;

                const QStringList &lines = line.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                for (const QString &lin : lines) {
                    Q_EMIT receivedRawLine(lin);
                }
                available = m_dccSocket->bytesAvailable();
            }
        }

        void Chat::sendAction(const QString &action)
        {
            QString line(action);
            OutputFilter::replaceAliases(line);

            const QString actionText = QStringLiteral("\x01""ACTION %2\x01");
            sendRawLine(actionText.arg(line));
        }

        void Chat::sendRawLine(const QString &text)
        {
            if (m_dccSocket)
            {
                m_dccSocket->write(m_codec->fromUnicode(text + QLatin1Char('\n')));
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
            connect(m_dccSocket, &QTcpSocket::errorOccurred, this, &Chat::connectionFailed);
            // the listen socket isn't needed anymore
            disconnect(m_dccServer, nullptr, nullptr, nullptr);
            m_dccServer->close();
            m_dccServer = nullptr;

            if (Preferences::self()->dccUPnP())
            {
                UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                if (router)
                {
                    router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                }
            }

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
            Q_EMIT upnpError(description);
        }
    }
}

#include "moc_chat.cpp"
