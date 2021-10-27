/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERDETAILEDINFOPANEL_H
#define TRANSFERDETAILEDINFOPANEL_H

#include "ui_transferdetailedinfopanelui.h"
#include "ui_transferdetailedtimeinfopanelui.h"

#include <QTabWidget>

class QTimer;

namespace Konversation
{
    namespace DCC
    {
        class Transfer;

        class TransferDetailedInfoPanel : public QTabWidget
        {
            Q_OBJECT

            public:
                explicit TransferDetailedInfoPanel(QWidget *parent = nullptr);
                ~TransferDetailedInfoPanel() override;

                void setTransfer(Transfer *item);
                Transfer *transfer() const;
                void clear();

            private Q_SLOTS:
                void updateView();
                // Only updates labels that can change during transfer
                void updateChangeableView();
                void slotTransferStatusChanged(Konversation::DCC::Transfer *transfer, int newStatus, int oldStatus);
                void slotLocationChanged(const QString& url);

            private:
                Ui::DccTransferDetailedInfoPanelUI m_locationInfo;
                Ui::DccTransferDetailedTimeInfoPanelUI m_timeInfo;

                Transfer *m_transfer;
                QTimer *m_autoViewUpdateTimer;

                Q_DISABLE_COPY(TransferDetailedInfoPanel)
        };
    }
}

#endif  // TRANSFERDETAILEDINFOPANEL_H
