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
#include <QItemSelection>



#include <KLocalizedString>

class QSplitter;
class QMenu;
class KToolBar;

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
                explicit TransferPanel(QWidget *parent);
                ~TransferPanel() override;

                TransferView *getTransferView();

                void openLocation(Transfer *transfer);
                void openFileInfoDialog(Transfer *transfer);

            protected Q_SLOTS:
                void slotNewTransferAdded(Konversation::DCC::Transfer *transfer);
                void slotTransferStatusChanged();

                void acceptDcc();
                void abortDcc();
                void resendFile();
                void clearDcc();
                void clearCompletedDcc();
                void runDcc();
                void openLocation();
                void selectAll();
                void selectAllCompleted();

                void popupRequested (const QPoint &pos);

                void doubleClicked(const QModelIndex &index);

                void updateButton();

                void setDetailPanelItem (const QItemSelection &newindex, const QItemSelection &oldindex);

            protected:
                /** Called from ChatWindow adjustFocus */
                void childAdjustFocus() override;

            private:
                inline void initGUI();

                TransferView *m_transferView;
                QMenu *m_popup;
                KToolBar *m_toolBar;

                TransferDetailedInfoPanel *m_detailPanel;
                QSplitter *m_splitter;

                QAction *m_abort;
                QAction *m_accept;
                QAction *m_clear;
                QAction *m_clearCompleted;
                QAction *m_open;
                QAction *m_openLocation;
                QAction *m_selectAll;
                QAction *m_selectAllCompleted;
                QAction *m_resend;
        };
    }
}

#endif
