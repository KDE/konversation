/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dccpanel.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  §Id$
*/

#include <qhbox.h>
#include <qpushbutton.h>

#include <kdialog.h>
#include <klocale.h>

#include "dccpanel.h"
#include "dcctransfer.h"

DccPanel::DccPanel(QWidget* parent) :
          ChatWindow(parent)
{
  setSpacing(spacing());
  setMargin(margin());

  setType(ChatWindow::DccPanel);

  dccListView=new KListView(this,"dcc_control_panel");

  dccListView->addColumn(i18n("Partner"));
  dccListView->addColumn(i18n("File"));
  dccListView->addColumn(i18n("Size"));
  dccListView->addColumn(i18n("Position"));
  dccListView->addColumn(i18n("% done"));
  dccListView->addColumn(i18n("CPS"));
  dccListView->addColumn(i18n("IP Address"));
  dccListView->addColumn(i18n("Status"));

  dccListView->setDragEnabled(true);
  dccListView->setAcceptDrops(true);
  dccListView->setSorting(-1,false);
  dccListView->setAllColumnsShowFocus(true);

  QHBox* buttonsBox=new QHBox(this);
  buttonsBox->setSpacing(spacing());
  new QPushButton(i18n("Accept"),buttonsBox,"start_dcc");
  new QPushButton(i18n("Resume"),buttonsBox,"resume_dcc");
  new QPushButton(i18n("Abort"),buttonsBox,"abort_dcc");
  new QPushButton(i18n("Remove"),buttonsBox,"remove_dcc");
  new QPushButton(i18n("Open"),buttonsBox,"open_dcc_file");
  new QPushButton(i18n("Information"),buttonsBox,"info_on_dcc_file");

  new DccTransfer(dccListView,DccTransfer::Get,"GetPartner","get/file/name.html");
  new DccTransfer(dccListView,DccTransfer::Send,"SendPartner","send/file/name.html");
}

DccPanel::~DccPanel()
{
}

int DccPanel::spacing() { return KDialog::spacingHint(); }
int DccPanel::margin()  { return KDialog::marginHint(); }
