/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dccpanel.h  -  description
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
        enum Object { TypeIcon, OfferDate, Status, FileName, PartnerNick, Progress, Position, TimeRemaining, CPS, COUNT };
    };
    class Popup
    {
      public:
        enum Object { Accept, Abort, Clear, ClearAllCompleted, Open, Info, Detail };
    };
    
#ifdef USE_MDI
    DccPanel(QString caption);
#else
    DccPanel(QWidget* parent);
#endif
    ~DccPanel();

    KListView* getListView();
    DccTransfer* getTransferByPort(const QString& port,DccTransfer::DccType type,bool resumed=false);
    DccTransfer* getTransferByName(const QString& name,DccTransfer::DccType type,bool resumed=false);
    void dccStatusChanged(const DccTransfer* item);

  public slots:
    void adjustFocus();

  protected slots:
    void acceptDcc();
    void abortDcc();
    void clearDcc();
    void runDcc();
    void showFileInfo();
    void openDetail();
    void clearAllCompletedDcc();
        
    void popupRequested(QListViewItem* item,const QPoint& pos,int col);
    void popupActivated(int id);
    
    void doubleClicked(QListViewItem* _item,const QPoint& _pos,int _col);
    
    void selectionChanged();

  protected:
#ifdef USE_MDI
    virtual void closeYourself(ChatWindow*);
#endif
    
    KListView* dccListView;
    KPopupMenu* popup;
    
    QPushButton* acceptButton;
    QPushButton* abortButton;
    QPushButton* clearButton;
    QPushButton* openButton;
    QPushButton* infoButton;
    QPushButton* detailButton;
};

#endif
