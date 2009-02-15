/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>

#ifndef DCCTRANSFERPANEL_H
#define DCCTRANSFERPANEL_H

#include "chatwindow.h"
#include "transferpanelitem.h" ////// header renamed


class QPushButton;
class K3ListView;
class KMenu;

class DccTransferDetailedInfoPanel;

class DccTransferPanel : public ChatWindow
{
    Q_OBJECT

        public:
        class Column
        {
            public:
                enum Object
                {
                    TypeIcon,
                    OfferDate,
                    Status,
                    FileName,
                    PartnerNick,
                    Progress,
                    Position,
                    TimeLeft,
                    CurrentSpeed,
                    SenderAddress,
                    COUNT
                };
        };

        DccTransferPanel(QWidget* parent);
        ~DccTransferPanel();

        K3ListView* getListView();

    protected slots:
        void slotNewTransferAdded( DccTransfer* transfer );
        void slotTransferStatusChanged();

        void acceptDcc();
        void abortDcc();
        void resendFile();
        void clearDcc();
        void runDcc();
        void showFileInfo();
        void selectAll();
        void selectAllCompleted();

        void popupRequested(Q3ListViewItem* item,const QPoint& pos,int col);
        void popupActivated( QAction *);

        void doubleClicked(Q3ListViewItem* _item,const QPoint& _pos,int _col);

        void updateButton();

        void setDetailPanelItem(Q3ListViewItem* item_);

    protected:
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        void initGUI();

        K3ListView* m_listView;
        KMenu* m_popup;

        DccTransferDetailedInfoPanel* m_detailPanel;

        QPushButton* m_buttonAccept;
        QPushButton* m_buttonAbort;
        QPushButton* m_buttonClear;
        QPushButton* m_buttonOpen;
        QPushButton* m_buttonDetail;
    QAction *m_abort;
    QAction *m_accept;
    QAction *m_clear;
    QAction *m_info;
    QAction *m_open;
    QAction *m_selectAll;
    QAction *m_selectAllCompleted;
    QAction *m_resend;
};
#endif
