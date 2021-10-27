/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include <QObject>

#include <QUrl>

namespace Konversation
{
    namespace UPnP
    {
        class UPnPMCastSocket;
        class UPnPRouter;
    }

    namespace DCC
    {
        class Transfer;
        class TransferRecv;
        class TransferSend;
        class Chat;

        /**
         * DccTransferManager controls DccTransfer instances.
         * All DccTransferRecv/DccTransferSend instances are created and deleted by this class.
         * Each DccTransfer instance is deleted immediately after its transfer done.
         */
        class TransferManager : public QObject
        {
            Q_OBJECT

            public:
                explicit TransferManager(QObject* parent = nullptr);
                ~TransferManager() override;

            Q_SIGNALS:
                /*
                 * The status of the item is DccTransfer::Configuring when this signal is emitted.
                 */
                void newTransferAdded(Konversation::DCC::Transfer* transfer);
                /*
                 * The status of the item is DccTransfer::Queued when this signal is emitted.
                 */
                void newDccTransferQueued(Konversation::DCC::Transfer* transfer);

                void fileURLChanged(Konversation::DCC::TransferRecv* transfer);

            public:
                TransferRecv* newDownload();
                TransferSend* newUpload();
                Chat* newChat();

                TransferSend* rejectSend(int connectionId, const QString& partnerNick, const QString& fileName);
                Chat* rejectChat(int connectionId, const QString& partnerNick);

                /**
                 * @return a DccTransferRecv item if applicable one found, otherwise 0.
                 */
                TransferRecv* resumeDownload(int connectionId, const QString& partnerNick, const QString& fileName, quint16 ownPort, quint64 position);

                /**
                 * @return a DccTransferSend item if applicable one found, otherwise 0.
                 */
                TransferSend* resumeUpload(int connectionId, const QString& partnerNick, const QString& fileName, quint16 ownPort, quint64 position);

                TransferSend* startReverseSending(int connectionId, const QString& partnerNick, const QString& fileName, const QString& partnerHost, quint16 partnerPort, quint64 fileSize, const QString& token);

                Chat* startReverseChat(int connectionId, const QString& partnerNick, const QString& partnerHost, quint16 partnerPort, const QString& token);

                void acceptDccGet(int connectionId, const QString& partnerNick, const QString& fileName);

                bool isLocalFileInWritingProcess(const QUrl &localUrl) const;

                int generateReverseTokenNumber();

                bool hasActiveTransfers() const;
                bool hasActiveChats() const;

                UPnP::UPnPRouter *getUPnPRouter() const;
                void startupUPnP();
                void shutdownUPnP();

            private:
                /*
                 * initTransfer() does the common jobs for newDownload() and newUpload()
                 */
                void initTransfer(Transfer* transfer);

            private Q_SLOTS:
                void slotTransferStatusChanged(Konversation::DCC::Transfer* item, int newStatus, int oldStatus);
                void removeSendItem(Konversation::DCC::Transfer* item);
                void removeRecvItem(Konversation::DCC::Transfer* item);
                void removeChatItem(Konversation::DCC::Chat* chat);

                void slotSettingsChanged();

                void upnpRouterDiscovered(Konversation::UPnP::UPnPRouter *router);

            private:
                QList< TransferSend* > m_sendItems;
                QList< TransferRecv* > m_recvItems;
                QList< Chat* > m_chatItems;

                UPnP::UPnPMCastSocket *m_upnpSocket;
                UPnP::UPnPRouter *m_upnpRouter;

                int m_nextReverseTokenNumber;
                QUrl m_defaultIncomingFolder;  // store here to know if this settings is changed

                Q_DISABLE_COPY(TransferManager)
        };
    }
}

#endif  // TRANSFERMANAGER_H
