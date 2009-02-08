/*
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef DCCTRANSFERPANELITEM_H
#define DCCTRANSFERPANELITEM_H

#include "transfer.h" ////// header renamed

#include <qdatetime.h>

#include <k3listview.h>
#include <kurl.h>
#include <kio/global.h>


class QTimer;
class QProgressBar;

namespace KIO
{
    }

class DccTransferPanel;

class DccTransferPanelItem : public QObject, public K3ListViewItem
{
    Q_OBJECT

    public:
        DccTransferPanelItem( DccTransferPanel* panel, DccTransfer* transfer );
        virtual ~DccTransferPanelItem();

        virtual void paintCell( QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment );

        virtual int compare( Q3ListViewItem* i, int col, bool ascending ) const;

        void runFile();
        void openFileInfoDialog();

        DccTransfer* transfer() const { return m_transfer; }

        // called from updateView()
        QString getTypeText()                                  const;
        QPixmap getTypeIcon()                                  const;
        QPixmap getStatusIcon()                                const;
        QString getStatusText()                                const;
        QString getFileSizePrettyText()                        const;
        QString getPositionPrettyText( bool detailed = false ) const;
        QString getTimeLeftPrettyText()                        const;
        QString getAverageSpeedPrettyText()                    const;
        QString getCurrentSpeedPrettyText()                    const;
        QString getSenderAddressPrettyText()                   const;

        static QString getSpeedPrettyText( transferspeed_t speed );
        static QString secToHMS( long sec );

    private slots:
        void slotStatusChanged( DccTransfer* transfer, int newStatus, int oldStatus );
        void updateView();

    private:
        DccTransferPanel* m_panel;
        DccTransfer* m_transfer;
        bool m_isTransferInstanceBackup;

    private slots:
        void startAutoViewUpdate();
        void stopAutoViewUpdate();

        void backupTransferInfo( DccTransfer* transfer );

    private:
        void updateTransferInfo();
        void updateTransferMeters();

        void showProgressBar();                   // called from printCell()

        // UI
        QTimer* m_autoUpdateViewTimer;
        QProgressBar* m_progressBar;
};

#endif  // DCCTRANSFERPANELITEM_H
