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
/*
  Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERPANEL_H
#define TRANSFERPANEL_H

#include "chatwindow.h"
#include <QModelIndex>

class QPushButton;
class KMenu;

namespace Konversation
{
    namespace DCC
    {
        class TransferDetailedInfoPanel;
        class TransferView;
        class Transfer;

        class TransferPanel : public ChatWindow
        {
            Q_OBJECT

            public:
                TransferPanel(QWidget *parent);
                ~TransferPanel();

                TransferView *getTransferView();

                void runFile(Transfer *transfer);
                void openLocation(Transfer *transfer);
                void openFileInfoDialog(Transfer *transfer);

            protected slots:
                void slotNewTransferAdded(Konversation::DCC::Transfer *transfer);
                void slotTransferStatusChanged();

                void acceptDcc();
                void abortDcc();
                void resendFile();
                void clearDcc();
                void runDcc();
                void openLocation();
                void showFileInfo();
                void selectAll();
                void selectAllCompleted();

                void popupRequested (const QPoint &pos);
                void popupActivated(QAction *action);

                void doubleClicked(const QModelIndex &index);

                void updateButton();

                void setDetailPanelItem (const QModelIndex &newindex, const QModelIndex &oldindex);

            protected:
                /** Called from ChatWindow adjustFocus */
                virtual void childAdjustFocus();

            private:
                inline void initGUI();

                TransferView *m_transferView;
                KMenu *m_popup;

                TransferDetailedInfoPanel *m_detailPanel;

                QPushButton *m_buttonAccept;
                QPushButton *m_buttonAbort;
                QPushButton *m_buttonClear;
                QPushButton *m_buttonOpen;
                QPushButton *m_buttonOpenLocation;
                QPushButton *m_buttonDetail;
                QAction *m_abort;
                QAction *m_accept;
                QAction *m_clear;
                QAction *m_info;
                QAction *m_open;
                QAction *m_openLocation;
                QAction *m_selectAll;
                QAction *m_selectAllCompleted;
                QAction *m_resend;
        };
    }
}

#endif
