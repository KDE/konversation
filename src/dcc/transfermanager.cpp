/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transfermanager.h"

#include "transferrecv.h"
#include "transfersend.h"
#include "application.h"
#include "preferences.h"
#include "upnpmcastsocket.h"
#include "upnprouter.h"
#include "transfer.h"
#include "chat.h"
#include "konversation_log.h"

#include <QDir>

using namespace Konversation::UPnP;

namespace Konversation
{
    namespace DCC
    {
        TransferManager::TransferManager( QObject* parent )
            : QObject( parent )
        {
            // initial number
            m_nextReverseTokenNumber = 1001;

            m_defaultIncomingFolder = Preferences::self()->dccPath();

            connect( Application::instance(), &Application::appearanceChanged,
                     this, &TransferManager::slotSettingsChanged );

            m_upnpRouter = nullptr;
            m_upnpSocket = nullptr;

            if (Preferences::self()->dccUPnP())
                startupUPnP();
        }

        TransferManager::~TransferManager()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            const auto currentSendItems = m_sendItems;
            for (TransferSend* sendItem : currentSendItems) {
                sendItem->abort();
            }
            const auto currentRecvItems = m_recvItems;
            for (TransferRecv* recvItem : currentRecvItems) {
                recvItem->abort();
            }
            const auto currentChatItems = m_chatItems;
            for (Chat* chatItem : currentChatItems) {
                chatItem->close();
            }

            // give the sockets a minor break to close everything 100%
            // yes I am aware of the fact that this is not really needed, theoretical
            while (!m_sendItems.isEmpty())
            {
                delete m_sendItems.takeFirst();
            }
            while (!m_recvItems.isEmpty())
            {
                delete m_recvItems.takeFirst();
            }
            while (!m_chatItems.isEmpty())
            {
                delete m_chatItems.takeFirst();
            }

            shutdownUPnP();
        }

        void TransferManager::startupUPnP()
        {
            m_upnpSocket = new UPnPMCastSocket();

            connect(m_upnpSocket, &UPnP::UPnPMCastSocket::discovered, this, &TransferManager::upnpRouterDiscovered);

            m_upnpSocket->discover();
        }

        void TransferManager::shutdownUPnP()
        {
            // This deletes the router too.
            delete m_upnpSocket;
            m_upnpSocket = nullptr;
            m_upnpRouter = nullptr;
        }

        TransferRecv* TransferManager::newDownload()
        {
            auto* transfer = new TransferRecv(this);
            m_recvItems.push_back( transfer );
            connect(transfer, &TransferRecv::removed, this, &TransferManager::removeRecvItem);
            initTransfer( transfer );
            return transfer;
        }

        TransferSend* TransferManager::newUpload()
        {
            auto* transfer = new TransferSend(this);
            m_sendItems.push_back( transfer );
            connect(transfer, &TransferSend::removed, this, &TransferManager::removeSendItem);
            initTransfer( transfer );
            return transfer;
        }

        Chat* TransferManager::newChat()
        {
            Chat* chat = new Chat(this);
            m_chatItems.append(chat);
            connect(chat, &Chat::removed, this, &TransferManager::removeChatItem);
            return chat;
        }

        TransferSend* TransferManager::rejectSend(int connectionId, const QString& partnerNick, const QString& fileName)
        {
            TransferSend* transfer = nullptr;

            // find applicable one
            for (TransferSend* it : std::as_const(m_sendItems)) {
                if ( ( it->getStatus() == Transfer::Queued || it->getStatus() == Transfer::WaitingRemote ) &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName )
                {
                    transfer = it;
                    qCDebug(KONVERSATION_LOG) << "Filename match: " << fileName;
                    break;
                }
            }

            if ( transfer )
                transfer->reject();

            return transfer;
        }

        Chat* TransferManager::rejectChat(int connectionId, const QString& partnerNick)
        {
            Chat* chat = nullptr;

            // find applicable one
            for (Chat* it : std::as_const(m_chatItems)) {
                if (it->status() == Chat::WaitingRemote &&
                    it->connectionId() == connectionId &&
                    it->partnerNick() == partnerNick)
                {
                    chat = it;
                    break;
                }
            }

            if (chat)
                chat->reject();

            return chat;
        }

        TransferRecv* TransferManager::resumeDownload( int connectionId, const QString& partnerNick, const QString& fileName, quint16 ownPort, quint64 position )
        {
            TransferRecv* transfer = nullptr;

            // find applicable one
            for (TransferRecv* it : std::as_const(m_recvItems)) {
                if ( ( it->getStatus() == Transfer::Queued || it->getStatus() == Transfer::WaitingRemote ) &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName &&
                    it->isResumed() )
                {
                    transfer = it;
                    qCDebug(KONVERSATION_LOG) << "Filename match: " << fileName << ", claimed port: " << ownPort << ", item port: " << transfer->getOwnPort();
                    // the port number can be changed behind NAT, so we pick an item which only the filename is correspondent in that case.
                    if ( transfer->getOwnPort() == ownPort )
                    {
                        break;
                    }
                }
            }

            if ( transfer )
                transfer->startResume( position );

            return transfer;
        }

        TransferSend* TransferManager::resumeUpload( int connectionId, const QString& partnerNick, const QString& fileName, quint16 ownPort, quint64 position )
        {
            TransferSend* transfer = nullptr;

            // find applicable one
            for (TransferSend* it : std::as_const(m_sendItems)) {
                if ( ( it->getStatus() == Transfer::Queued || it->getStatus() == Transfer::WaitingRemote ) &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName &&
                    !it->isResumed() )
                {
                    transfer = it;
                    qCDebug(KONVERSATION_LOG) << "Filename match: " << fileName << ", claimed port: " << ownPort << ", item port: " << transfer->getOwnPort();
                    // the port number can be changed behind NAT, so we pick an item which only the filename is correspondent in that case.
                    if ( transfer->getOwnPort() == ownPort )
                    {
                        break;
                    }
                }
            }

            if ( transfer )
                transfer->setResume( position );

            return transfer;
        }

        TransferSend* TransferManager::startReverseSending( int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, quint16 partnerPort, quint64 fileSize, const QString& token )
        {
            qCDebug(KONVERSATION_LOG) << "Server group ID: " << connectionId << ", partner: " << partnerNick << ", filename: " << fileName << ", partner IP: " << partnerHost << ", parnter port: " << partnerPort << ", filesize: " << fileSize << ", token: " << token;
            TransferSend* transfer = nullptr;

            // find applicable one
            for (TransferSend* it : std::as_const(m_sendItems)) {
                if (
                    it->getStatus() == Transfer::WaitingRemote &&
                    it->getConnectionId() == connectionId &&
                    it->getPartnerNick() == partnerNick &&
                    it->getFileName() == fileName &&
                    it->getFileSize() == fileSize &&
                    it->getReverseToken() == token
                )
                {
                    transfer = it;
                    break;
                }
            }

            if ( transfer )
                transfer->connectToReceiver( partnerHost, partnerPort );

            return transfer;
        }


        Chat* TransferManager::startReverseChat(int connectionId, const QString& partnerNick, const QString& partnerHost, quint16 partnerPort, const QString& token)
        {
            qCDebug(KONVERSATION_LOG) << "Server group ID: " << connectionId << ", partner: " << partnerNick << ", partner IP: " << partnerHost << ", parnter port: " << partnerPort << ", token: " << token;
            Chat* chat = nullptr;

            // find applicable one
            for (Chat* it :std::as_const(m_chatItems)) {
                if (
                    it->status() == Chat::WaitingRemote &&
                    it->connectionId() == connectionId &&
                    it->partnerNick() == partnerNick &&
                    it->reverseToken() == token
                )
                {
                    chat = it;
                    break;
                }
            }

            if (chat)
            {
                chat->setPartnerIp(partnerHost);
                chat->setPartnerPort(partnerPort);
                chat->connectToPartner();
            }

            return chat;
        }

        void TransferManager::acceptDccGet(int connectionId, const QString& partnerNick, const QString& fileName)
        {
            qCDebug(KONVERSATION_LOG) << "Server group ID: " << connectionId << ", partner: " << partnerNick << ", filename: " << fileName;

            bool nickEmpty = partnerNick.isEmpty();
            bool fileEmpty = fileName.isEmpty();

            const auto currentRecvItems = m_recvItems;
            for (TransferRecv* it : currentRecvItems) {
                if (
                    it->getStatus() == Transfer::Queued &&
                    it->getConnectionId() == connectionId &&
                    (nickEmpty || it->getPartnerNick() == partnerNick) &&
                    (fileEmpty || it->getFileName() == fileName)
                )
                {
                    it->start();
                }
            }
        }

        void TransferManager::initTransfer( Transfer* transfer )
        {
            connect(transfer, &TransferSend::statusChanged, this, &TransferManager::slotTransferStatusChanged);

            Q_EMIT newTransferAdded( transfer );
        }

        bool TransferManager::isLocalFileInWritingProcess( const QUrl &url ) const
        {
            for (TransferRecv* it : m_recvItems) {
                if ( ( it->getStatus() == Transfer::Connecting ||
                       it->getStatus() == Transfer::Transferring ) &&
                    it->getFileURL() == url )
                {
                    return true;
                }
            }
            return false;
        }

        int TransferManager::generateReverseTokenNumber()
        {
            return m_nextReverseTokenNumber++;
        }

        bool TransferManager::hasActiveTransfers() const
        {
            for (TransferSend* it : m_sendItems) {
                if (it->getStatus() == Transfer::Transferring)
                    return true;
            }

            for (TransferRecv* it : m_recvItems) {
                if (it->getStatus() == Transfer::Transferring)
                    return true;
            }

            return false;
        }

        bool TransferManager::hasActiveChats() const
        {
            for (Chat* chat : m_chatItems) {
                if (chat->status() == Chat::Chatting)
                    return true;
            }
            return false;
        }

        void TransferManager::slotTransferStatusChanged( Transfer* item, int newStatus, int oldStatus )
        {
            qCDebug(KONVERSATION_LOG) << oldStatus << " -> " << newStatus << " " << item->getFileName() << " (" << item->getType() << ")";

            if ( newStatus == Transfer::Queued )
                Q_EMIT newDccTransferQueued( item );
        }

        void TransferManager::slotSettingsChanged()
        {
            // update the default incoming directory for already existed DCCRECV items
            if ( Preferences::self()->dccPath() != m_defaultIncomingFolder )
            {
                for (TransferRecv* it : std::as_const(m_recvItems)) {
                    if ( it->getStatus() == Transfer::Queued &&
                         it->getFileURL().adjusted(QUrl::RemoveFilename) == m_defaultIncomingFolder.adjusted(QUrl::RemoveFilename))
                    {
                        QUrl url = QUrl::fromLocalFile(Preferences::self()->dccPath().adjusted(QUrl::StripTrailingSlash).toString() + QDir::separator() + it->getFileURL().fileName());
                        it->setFileURL(url);

                        Q_EMIT fileURLChanged( it );
                    }
                }

                m_defaultIncomingFolder = Preferences::self()->dccPath();
            }
        }

        void TransferManager::removeSendItem( Transfer* item )
        {
            auto* transfer = qobject_cast< TransferSend* > ( item );
            m_sendItems.removeOne( transfer );
            item->deleteLater();
        }

        void TransferManager::removeRecvItem( Transfer* item )
        {
            auto* transfer = qobject_cast< TransferRecv* > ( item );
            m_recvItems.removeOne( transfer );
            item->deleteLater();
        }

        void TransferManager::removeChatItem(Konversation::DCC::Chat* chat)
        {
            m_chatItems.removeOne(chat);
            chat->deleteLater();
        }

        void TransferManager::upnpRouterDiscovered(UPnPRouter *router)
        {
            qCDebug(KONVERSATION_LOG) << "Router discovered!";

            // Assuming only 1 router for now
            m_upnpRouter = router;
        }

        UPnPRouter* TransferManager::getUPnPRouter() const
        {
            return m_upnpRouter;
        }
    }
}

#include "moc_transfermanager.cpp"
