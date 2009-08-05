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

#include <QLabel>
#include <QTimer>

#include <KProgressDialog>
#include <KRun>

namespace Konversation
{
    namespace DCC
    {
        TransferDetailedInfoPanel::TransferDetailedInfoPanel(QWidget * parent)
            : QWidget(parent)
        {
            setupUi(this);

            m_autoViewUpdateTimer = new QTimer(this);

            connect(m_urlreqLocation, SIGNAL(textChanged(const QString&)), this, SLOT(slotLocationChanged(const QString&)));
            connect(Application::instance()->getDccTransferManager(), SIGNAL(fileURLChanged(Konversation::DCC::TransferRecv*)),
                    this, SLOT(updateView()));  // it's a little rough..

            //only enable when needed
            m_urlreqLocation->lineEdit()->setReadOnly(true);
            m_urlreqLocation->button()->setEnabled(false);
            m_urlreqLocation->setMode(KFile::File | KFile::LocalOnly);
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

        void TransferDetailedInfoPanel::clear()
        {
            m_autoViewUpdateTimer->stop();

            // disconnect all slots once
            disconnect(this);
            // we can't do disconnect( m_item->transfer(), 0, this, 0 ) here
            // because m_item can have been deleted already.

            m_transfer = 0;

            m_urlreqLocation->lineEdit()->setReadOnly(true);
            m_urlreqLocation->lineEdit()->setFrame(false);
            m_urlreqLocation->button()->setEnabled(false);

            m_labelDccType->clear();
            m_labelFilename->clear();
            m_urlreqLocation->clear();
            m_labelPartner->clear();
            m_labelSelf->clear();
            m_labelFileSize->clear();
            m_labelIsResumed->clear();
            m_labelTimeOffered->clear();
            m_labelTimeStarted->clear();
            m_labelTimeFinished->clear();

            m_labelStatus->clear();
            m_progress->setValue(0);
            m_labelCurrentPosition->clear();
            m_labelCurrentSpeed->clear();
            m_labelAverageSpeed->clear();
            m_labelTransferringTime->clear();
            m_labelTimeLeft->clear();
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
            m_labelDccType->setText(type);

            // Filename:
            m_labelFilename->setText(m_transfer->getFileName());

            // Location:
            m_urlreqLocation->setUrl(m_transfer->getFileURL().prettyUrl());
            //m_urlreqLocation->lineEdit()->setFocusPolicy( m_item->getStatus() == Transfer::Queued ? Qt::StrongFocus : ClickFocus );
            m_urlreqLocation->lineEdit()->setReadOnly(m_transfer->getStatus() != Transfer::Queued);
            m_urlreqLocation->lineEdit()->setFrame(m_transfer->getStatus() == Transfer::Queued);
            m_urlreqLocation->button()->setEnabled(m_transfer->getStatus() == Transfer::Queued);

            // Partner:
            QString partnerInfoServerName;
            if (m_transfer->getConnectionId() == -1)
                partnerInfoServerName = i18n("Unknown server");
            else if (Application::instance()->getConnectionManager()->getServerByConnectionId(m_transfer->getConnectionId()))
                partnerInfoServerName = Application::instance()->getConnectionManager()->getServerByConnectionId(m_transfer->getConnectionId())->getServerName();
            else
                partnerInfoServerName = i18n("Unknown server");
            QString partnerInfo(i18n("%1 on %2",
                m_transfer->getPartnerNick().isEmpty() ? "?" : m_transfer->getPartnerNick(),
                partnerInfoServerName));
            if (!m_transfer->getPartnerIp().isEmpty())
                partnerInfo += i18n(", %1 (port %2)", m_transfer->getPartnerIp(), QString::number(m_transfer->getPartnerPort()));
            m_labelPartner->setText(partnerInfo);

            // Self:
            if (!m_transfer->getOwnIp().isEmpty())
                m_labelSelf->setText(i18n("%1 (port %2)", m_transfer->getOwnIp(), QString::number(m_transfer->getOwnPort())));

            // File Size:
            m_labelFileSize->setText(KGlobal::locale()->formatNumber(m_transfer->getFileSize(), 0));

            // Resumed:
            if (m_transfer->isResumed())
                m_labelIsResumed->setText(i18n("Yes, %1", KGlobal::locale()->formatNumber(m_transfer->getTransferStartPosition(), 0)));
            else
                m_labelIsResumed->setText(i18n("No"));

            // Offered at:
            m_labelTimeOffered->setText(m_transfer->getTimeOffer().toString("hh:mm:ss"));

            // Started at:
            if (!m_transfer->getTimeTransferStarted().isNull())
                m_labelTimeStarted->setText(m_transfer->getTimeTransferStarted().toString("hh:mm:ss"));

            // Finished at:
            if (!m_transfer->getTimeTransferFinished().isNull())
            {
                m_labelTimeFinished->setText(m_transfer->getTimeTransferFinished().toString("hh:mm:ss"));
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
                m_labelStatus->setText(TransferListModel::getStatusText(m_transfer->getStatus(),
                                       m_transfer->getType()) + " ( " +
                                       TransferListModel::getSpeedPrettyText(m_transfer->getCurrentSpeed()) + " )");
            }
            else
            {
                m_labelStatus->setText(m_transfer->getStatusDetail().isEmpty() ?
                                       TransferListModel::getStatusText(m_transfer->getStatus(), m_transfer->getType()) :
                                       TransferListModel::getStatusText(m_transfer->getStatus(), m_transfer->getType()) + " (" + m_transfer->getStatusDetail() + ')');
            }

            // Progress:
            m_progress->setValue(m_transfer->getProgress());

            // Current Position:
            m_labelCurrentPosition->setText(KGlobal::locale()->formatNumber(m_transfer->getTransferringPosition(), 0));

            // Current Speed:
            m_labelCurrentSpeed->setText(TransferListModel::getSpeedPrettyText(m_transfer->getCurrentSpeed()));

            // Average Speed:
            m_labelAverageSpeed->setText(TransferListModel::getSpeedPrettyText(m_transfer->getAverageSpeed()));

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
                    m_labelTransferringTime->setText(TransferListModel::secToHMS(m_itemringTime));
                else
                    m_labelTransferringTime->setText(i18n("< 1sec"));
            }

            // Estimated Time Left:
            m_labelTimeLeft->setText(TransferListModel::getTimeLeftPrettyText(m_transfer->getTimeLeft()));
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
