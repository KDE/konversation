// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef DCCDETAILDIALOG_H
#define DCCDETAILDIALOG_H

#include <kdialog.h>

class KLineEdit;
class KProgress;
class KPushButton;
class KURLRequester;
class DccTransfer;
class DccTransferPanelItem;

class DccDetailDialog : public KDialog
{
    Q_OBJECT

        public:
        explicit DccDetailDialog( DccTransferPanelItem* item );
        virtual ~DccDetailDialog();

        void updateView();

    protected slots:
        void slotLocalFileURLChanged( const QString& newURL );
        void slotOpenFile();
        void slotRemoveFile();
        void slotAccept();
        void slotAbort();
        void slotClose();

    protected:
        DccTransferPanelItem* m_item;
        DccTransfer* m_transfer;

        // UI
        KPushButton* m_buttonOpenFile;
        KPushButton* m_buttonRemoveFile;
        KURLRequester* m_localFileURL;
        KLineEdit* m_partner;
        KLineEdit* m_self;
        KLineEdit* m_status;
        KProgress* m_progress;
        KLineEdit* m_position;

        KPushButton* m_buttonAccept;
        KPushButton* m_buttonAbort;
};
#endif                                            // DCCDETAILDIALOG_H
