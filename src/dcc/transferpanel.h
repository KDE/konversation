/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef TRANSFERPANEL_H
#define TRANSFERPANEL_H

#include "chatwindow.h"

#include <QModelIndex>
#include <QItemSelection>




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

                TransferView *getTransferView() const;

                void openLocation(Transfer *transfer);

            protected:
                /** Called from ChatWindow adjustFocus */
                void childAdjustFocus() override;

            private Q_SLOTS:
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

            private:
                inline void initGUI();

            private:
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

                Q_DISABLE_COPY(TransferPanel)
        };
    }
}

#endif
