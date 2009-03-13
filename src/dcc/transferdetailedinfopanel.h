/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#ifndef DCCTRANSFERDETAILEDINFOPANEL_H
#define DCCTRANSFERDETAILEDINFOPANEL_H

#include "ui_transferdetailedinfopanelui.h"

class QTimer;

class DccTransfer;
class DccTransferPanelItem;

class DccTransferDetailedInfoPanel : public QWidget, private Ui::DccTransferDetailedInfoPanelUI
{
    Q_OBJECT

    public:
        explicit DccTransferDetailedInfoPanel( QWidget* parent = 0 );
        virtual ~DccTransferDetailedInfoPanel();

        void setItem( DccTransferPanelItem* item );

    private slots:
        void updateView();
        void slotTransferStatusChanged( DccTransfer* transfer, int newStatus, int oldStatus );
        void slotLocationChanged( const QString& url );

    private:
        DccTransferPanelItem* m_item;
        QTimer* m_autoViewUpdateTimer;
};

#endif  // DCCTRANSFERDETAILEDINFOPANEL_H
