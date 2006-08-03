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

#ifndef DCCPANEL_H
#define DCCPANEL_H

/*
  @author Dario Abatianni
*/

#include "chatwindow.h"
#include "dcctransfer.h"

class QContextMenuEvent;
class QPushButton;
class KListView;
class KPopupMenu;

class DccPanel : public ChatWindow
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
                    TimeRemaining,
                    CPS,
                    SenderAddress,
                    COUNT
                };
        };

        class Popup
        {
            public:
                enum Object
                {
                    SelectAll,
                    SelectAllCompleted,
                    Accept,
                    Abort,
                    Clear,
                    Open,
                    Remove,
                    Info,
                    Detail
                };
        };

        DccPanel(QWidget* parent);
        ~DccPanel();

        KListView* getListView();
        DccTransfer* getTransferByPort(const QString& port,DccTransfer::DccType type,bool resumed=false);
        DccTransfer* getTransferByName(const QString& name,DccTransfer::DccType type,bool resumed=false);

        bool isLocalFileInWritingProcess( const KURL& localUrl );

        void selectMe(DccTransfer* item);

    public slots:
        void dccStatusChanged(const DccTransfer* item);

    protected slots:
        void acceptDcc();
        void abortDcc();
        void clearDcc();
        void runDcc();
        void removeFile();
        void showFileInfo();
        void openDetail();
        void selectAll();
        void selectAllCompleted();

        void popupRequested(QListViewItem* item,const QPoint& pos,int col);
        void popupActivated(int id);

        void doubleClicked(QListViewItem* _item,const QPoint& _pos,int _col);

        void updateButton();

    protected:
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        KListView* m_listView;
        KPopupMenu* m_popup;

        QPushButton* m_buttonAccept;
        QPushButton* m_buttonAbort;
        QPushButton* m_buttonClear;
        QPushButton* m_buttonOpen;
        QPushButton* m_buttonRemove;
        QPushButton* m_buttonDetail;
};
#endif
