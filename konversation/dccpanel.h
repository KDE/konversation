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

class QPushButton;

class KListView;

class DccPanel : public ChatWindow
{
  Q_OBJECT

  public:
    DccPanel(QWidget* parent);
    ~DccPanel();

    KListView* getListView();
    DccTransfer* getTransferByPort(QString port,DccTransfer::DccType type);
    DccTransfer* getTransferByName(QString name,DccTransfer::DccType type);
  
  public slots:
    void adjustFocus();

  protected slots:
    void acceptDcc();
    void runDcc();
    void abortDcc();
    void removeDcc();
    void dccSelected();
    void showFileInfo();

  protected:
    void setButtons(bool accept,bool abort,bool remove,bool open,bool info);

    KListView* dccListView;

    QPushButton* acceptButton;
    QPushButton* abortButton;
    QPushButton* removeButton;
    QPushButton* openButton;
    QPushButton* infoButton;
};

#endif
