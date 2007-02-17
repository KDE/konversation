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

#include <qdatetime.h>

#include <klistview.h>
#include <kurl.h>
#include <kio/global.h>

#include "dcctransfer.h"

class QStringList;
class QTimer;

class KProgress;

namespace KIO
{
    class Job;
}

class DccTransferPanel;

/*
  @author Dario Abatianni
*/

class DccTransferPanelItem : public QObject, public KListViewItem
{
    Q_OBJECT

    public:
        DccTransferPanelItem( DccTransferPanel* panel, DccTransfer* transfer );
        virtual ~DccTransferPanelItem();

        virtual void paintCell( QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment );

        virtual int compare( QListViewItem* i, int col, bool ascending ) const;

        void runFile();
        void removeFile();
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
        QString getCurrentSpeedPrettyText()                    const;
        QString getSenderAddressPrettyText()                   const;

        static QString secToHMS( long sec );

    private slots:
        void slotStatusChanged( DccTransfer* transfer, int newStatus, int oldStatus );
        void updateView();

    private:
        DccTransferPanel* m_panel;
        DccTransfer* m_transfer;
        bool m_isTransferInstanceBackup;

    private slots:
        void slotRemoveFileDone( KIO::Job* job );

        void startAutoViewUpdate();
        void stopAutoViewUpdate();

        void backupTransferInfo( DccTransfer* transfer );

    private:
        void updateTransferInfo();
        void updateTransferMeters();

        void showProgressBar();                   // called from printCell()

        // UI
        QTimer* m_autoUpdateViewTimer;
        KProgress* m_progressBar;

        // file
        bool m_fileRemoved;

        static QString s_dccStatusText[ DccTransfer::DccStatusCount ];

};

#endif  // DCCTRANSFERPANELITEM_H
