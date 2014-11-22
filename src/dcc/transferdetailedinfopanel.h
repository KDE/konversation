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

#ifndef TRANSFERDETAILEDINFOPANEL_H
#define TRANSFERDETAILEDINFOPANEL_H

#include "ui_transferdetailedinfopanelui.h"
#include "ui_transferdetailedtimeinfopanelui.h"


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
                explicit TransferDetailedInfoPanel(QWidget *parent = 0);
                virtual ~TransferDetailedInfoPanel();

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
        };
    }
}

#endif  // TRANSFERDETAILEDINFOPANEL_H
