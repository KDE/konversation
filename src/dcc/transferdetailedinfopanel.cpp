/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transferdetailedinfopanel.h"
#include "transfer.h"
#include "transferrecv.h"
#include "transfermanager.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "transferlistmodel.h"

#include <QTimer>


namespace Konversation
{
    namespace DCC
    {
        TransferDetailedInfoPanel::TransferDetailedInfoPanel(QWidget * parent)
            : KTabWidget(parent)
        {
            QWidget *tab = new QWidget(this);
            m_locationInfo.setupUi(tab);
            addTab(tab, i18n("Status") );
            tab = new QWidget(this);
            m_timeInfo.setupUi(tab);
            addTab(tab, i18n("Details"));

            m_transfer = 0;
            m_autoViewUpdateTimer = new QTimer(this);

            connect(m_locationInfo.m_urlreqLocation, SIGNAL(textChanged(const QString&)), this, SLOT(slotLocationChanged(const QString&)));
            connect(Application::instance()->getDccTransferManager(), SIGNAL(fileURLChanged(Konversation::DCC::TransferRecv*)),
                    this, SLOT(updateView()));  // it's a little rough..

            //only enable when needed
            m_locationInfo.m_urlreqLocation->lineEdit()->setReadOnly(true);
            m_locationInfo.m_urlreqLocation->button()->setEnabled(false);
            m_locationInfo.m_urlreqLocation->setMode(KFile::File | KFile::LocalOnly);
        }

        TransferDetailedInfoPanel::~TransferDetailedInfoPanel()
        {
        }

        void TransferDetailedInfoPanel::setTransfer(Transfer *item)
        {
            if (item == m_transfer)
            {
                return;
            }

            clear();
            m_transfer = item;

            // set up the auto view-update timer
            connect(m_autoViewUpdateTimer, SIGNAL(timeout()), this, SLOT(updateChangeableView()));

            // If the file is already being transferred, the timer must be started here,
            // otherwise the information will not be updated every 0.5sec
            if (m_transfer->getStatus() == Transfer::Transferring)
                m_autoViewUpdateTimer->start(500);

            connect(item, SIGNAL(statusChanged(Konversation::DCC::Transfer*, int, int)),
                    this, SLOT(slotTransferStatusChanged(Konversation::DCC::Transfer*, int, int)));

            updateView();
        }

        Transfer *TransferDetailedInfoPanel::transfer() const
        {
            return m_transfer;
        }

        void TransferDetailedInfoPanel::clear()
        {
            m_autoViewUpdateTimer->stop();

            // disconnect all slots once
            disconnect(this);
            // we can't do disconnect( m_item->transfer(), 0, this, 0 ) here
            // because m_item can have been deleted already.

            m_transfer = 0;

            m_locationInfo.m_urlreqLocation->lineEdit()->setReadOnly(true);
            m_locationInfo.m_urlreqLocation->lineEdit()->setFrame(false);
            m_locationInfo.m_urlreqLocation->button()->setEnabled(false);

            m_locationInfo.m_labelDccType->clear();
            m_locationInfo.m_labelFilename->clear();
            m_locationInfo.m_urlreqLocation->clear();
            m_locationInfo.m_labelPartner->clear();
            m_locationInfo.m_labelSelf->clear();
            m_timeInfo.m_labelFileSize->clear();
            m_timeInfo.m_labelIsResumed->clear();
            m_timeInfo.m_labelTimeOffered->clear();
            m_timeInfo.m_labelTimeStarted->clear();
            m_timeInfo.m_labelTimeFinished->clear();

            m_locationInfo.m_labelStatus->clear();
            m_locationInfo.m_progress->setValue(0);
            m_timeInfo.m_labelCurrentPosition->clear();
            m_timeInfo.m_labelCurrentSpeed->clear();
            m_timeInfo.m_labelAverageSpeed->clear();
            m_timeInfo.m_labelTransferringTime->clear();
            m_timeInfo.m_labelTimeLeft->clear();
        }

        void TransferDetailedInfoPanel::updateView()
        {
            if (!m_transfer)
            {
                return;
            }

            // Type:
            QString type(m_transfer->getType() == Transfer::Send ? i18n("DCC Send") : i18n("DCC Receive"));
            if (m_transfer->isReverse())
            {
                type += i18n(" (Reverse DCC)");
            }
            m_locationInfo.m_labelDccType->setText(type);

            // Filename:
            m_locationInfo.m_labelFilename->setText(m_transfer->getFileName());

            // Location:
            m_locationInfo.m_urlreqLocation->setUrl(m_transfer->getFileURL().prettyUrl());
            //m_urlreqLocation->lineEdit()->setFocusPolicy( m_item->getStatus() == Transfer::Queued ? Qt::StrongFocus : ClickFocus );
            m_locationInfo.m_urlreqLocation->lineEdit()->setReadOnly(m_transfer->getStatus() != Transfer::Queued);
            m_locationInfo.m_urlreqLocation->lineEdit()->setFrame(m_transfer->getStatus() == Transfer::Queued);
            m_locationInfo.m_urlreqLocation->button()->setEnabled(m_transfer->getStatus() == Transfer::Queued);

            // Partner:
            QString partnerInfoServerName;
            if (m_transfer->getConnectionId() == -1)
                partnerInfoServerName = i18n("Unknown server");
            else if (Application::instance()->getConnectionManager()->getServerByConnectionId(m_transfer->getConnectionId()))
                partnerInfoServerName = Application::instance()->getConnectionManager()->getServerByConnectionId(m_transfer->getConnectionId())->getServerName();
            else
                partnerInfoServerName = i18n("Unknown server");

            if (!m_transfer->getPartnerIp().isEmpty())
            {
                m_locationInfo.m_labelPartner->setText(i18nc("%1=partnerNick, %2=IRC Servername, %3=partnerIP, %4=partnerPort",
                                                             "%1 on %2, %3 (port %4)",
                                                             m_transfer->getPartnerNick().isEmpty() ? "?" : m_transfer->getPartnerNick(),
                                                             partnerInfoServerName, m_transfer->getPartnerIp(), QString::number(m_transfer->getPartnerPort())));
            }
            else
            {
                m_locationInfo.m_labelPartner->setText(i18nc("%1 = PartnerNick, %2 = Partner IRC Servername","%1 on %2",
                                                             m_transfer->getPartnerNick().isEmpty() ? "?" : m_transfer->getPartnerNick(),
                                                             partnerInfoServerName));
            }

            // Self:
            if (!m_transfer->getOwnIp().isEmpty())
                m_locationInfo.m_labelSelf->setText(i18nc("%1=ownIP, %2=ownPort", "%1 (port %2)",
                                                          m_transfer->getOwnIp(), QString::number(m_transfer->getOwnPort())));

            // File Size:
            m_timeInfo.m_labelFileSize->setText(KGlobal::locale()->formatNumber(m_transfer->getFileSize(), 0));

            // Resumed:
            if (m_transfer->isResumed())
                m_timeInfo.m_labelIsResumed->setText(i18nc("%1=Transferstartposition","Yes, %1", KGlobal::locale()->formatNumber(m_transfer->getTransferStartPosition(), 0)));
            else
                m_timeInfo.m_labelIsResumed->setText(i18nc("no - not a resumed transfer","No"));

            // Offered at:
            m_timeInfo.m_labelTimeOffered->setText(m_transfer->getTimeOffer().toString("hh:mm:ss"));

            // Started at:
            if (!m_transfer->getTimeTransferStarted().isNull())
                m_timeInfo.m_labelTimeStarted->setText(m_transfer->getTimeTransferStarted().toString("hh:mm:ss"));

            // Finished at:
            if (!m_transfer->getTimeTransferFinished().isNull())
            {
                m_timeInfo.m_labelTimeFinished->setText(m_transfer->getTimeTransferFinished().toString("hh:mm:ss"));
            }

            updateChangeableView();
        }

        void TransferDetailedInfoPanel::updateChangeableView()
        {
            if (!m_transfer)
            {
                return;
            }

            // Status:
            if (m_transfer->getStatus() == Transfer::Transferring)
            {
                m_locationInfo.m_labelStatus->setText(TransferListModel::getStatusText(m_transfer->getStatus(),
                                       m_transfer->getType()) + " ( " +
                                       TransferListModel::getSpeedPrettyText(m_transfer->getCurrentSpeed()) + " )");
            }
            else
            {
                m_locationInfo.m_labelStatus->setText(m_transfer->getStatusDetail().isEmpty() ?
                                       TransferListModel::getStatusText(m_transfer->getStatus(), m_transfer->getType()) :
                                       TransferListModel::getStatusText(m_transfer->getStatus(), m_transfer->getType()) + " (" + m_transfer->getStatusDetail() + ')');
            }

            // Progress:
            m_locationInfo.m_progress->setValue(m_transfer->getProgress());

            // Current Position:
            m_timeInfo.m_labelCurrentPosition->setText(KGlobal::locale()->formatNumber(m_transfer->getTransferringPosition(), 0));

            // Current Speed:
            m_timeInfo.m_labelCurrentSpeed->setText(TransferListModel::getSpeedPrettyText(m_transfer->getCurrentSpeed()));

            // Average Speed:
            m_timeInfo.m_labelAverageSpeed->setText(TransferListModel::getSpeedPrettyText(m_transfer->getAverageSpeed()));

            // Transferring Time:
            if (!m_transfer->getTimeTransferStarted().isNull())
            {
                int m_itemringTime;

                // The m_item is still in progress
                if (m_transfer->getTimeTransferFinished().isNull())
                    m_itemringTime = m_transfer->getTimeTransferStarted().secsTo(QDateTime::currentDateTime());
                // The m_item has finished
                else
                    m_itemringTime = m_transfer->getTimeTransferStarted().secsTo(m_transfer->getTimeTransferFinished());

                if (m_itemringTime >= 1)
                    m_timeInfo.m_labelTransferringTime->setText(TransferListModel::secToHMS(m_itemringTime));
                else
                    m_timeInfo.m_labelTransferringTime->setText(i18n("< 1sec"));
            }

            // Estimated Time Left:
            m_timeInfo.m_labelTimeLeft->setText(TransferListModel::getTimeLeftPrettyText(m_transfer->getTimeLeft()));
        }

        void TransferDetailedInfoPanel::slotTransferStatusChanged(Transfer */* transfer */, int newStatus, int oldStatus)
        {
            updateView();
            if (newStatus == Transfer::Transferring)
            {
                // start auto view-update timer
                m_autoViewUpdateTimer->start(500);
            }
            else if (oldStatus == Transfer::Transferring)
            {
                // stop auto view-update timer
                m_autoViewUpdateTimer->stop();
            }
        }

        void TransferDetailedInfoPanel::slotLocationChanged(const QString &url)
        {
            if (m_transfer &&  m_transfer->getType() == Transfer::Receive)
            {
                TransferRecv *transfer = static_cast<TransferRecv*>(m_transfer);
                transfer->setFileURL(KUrl(url));
                updateView();
            }
        }

    }
}

#include "transferdetailedinfopanel.moc"
