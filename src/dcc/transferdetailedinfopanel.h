/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#ifndef TRANSFERDETAILEDINFOPANEL_H
#define TRANSFERDETAILEDINFOPANEL_H

#include "ui_transferdetailedinfopanelui.h"

class QTimer;

namespace Konversation
{
    namespace DCC
    {
        class Transfer;
        class TransferPanelItem;

        class TransferDetailedInfoPanel : public QWidget, private Ui::DccTransferDetailedInfoPanelUI
        {
            Q_OBJECT

            public:
                explicit TransferDetailedInfoPanel( QWidget* parent = 0 );
                virtual ~TransferDetailedInfoPanel();

                void setItem( TransferPanelItem* item );

            private slots:
                void updateView();
                // Only updates labels that can change during transfer
                void updateChangeableView();
                void slotTransferStatusChanged( Konversation::DCC::Transfer* transfer, int newStatus, int oldStatus );
                void slotLocationChanged( const QString& url );

            private:
                TransferPanelItem* m_item;
                QTimer* m_autoViewUpdateTimer;
        };
    }
}

#endif  // TRANSFERDETAILEDINFOPANEL_H
