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

  $Id$
*/

#include <qhbox.h>

#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <krun.h>

#include "dccpanel.h"

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
  dccListView->addColumn(i18n("Port"));
  dccListView->addColumn(i18n("Status"));

  dccListView->setDragEnabled(true);
  dccListView->setAcceptDrops(true);
  dccListView->setSorting(-1,false);
  dccListView->setAllColumnsShowFocus(true);

  QHBox* buttonsBox=new QHBox(this);
  buttonsBox->setSpacing(spacing());
  acceptButton=new QPushButton(i18n("Accept"),buttonsBox,"start_dcc");
  sendButton  =new QPushButton(i18n("Send File"),buttonsBox,"send_dcc");
  abortButton =new QPushButton(i18n("Abort"),buttonsBox,"abort_dcc");
  removeButton=new QPushButton(i18n("Remove"),buttonsBox,"remove_dcc");
  openButton  =new QPushButton(i18n("Open"),buttonsBox,"open_dcc_file");
  infoButton  =new QPushButton(i18n("Information"),buttonsBox,"info_on_dcc_file");

  connect(dccListView,SIGNAL (selectionChanged()),this,SLOT (dccSelected()) );

  connect(acceptButton,SIGNAL (clicked()) ,this,SLOT (acceptDcc()) );
  connect(removeButton,SIGNAL (clicked()) ,this,SLOT (removeDcc()) );
  connect(openButton,SIGNAL (clicked()) ,this,SLOT (runDcc()) );
}

DccPanel::~DccPanel()
{
}

void DccPanel::setButtons(bool accept,bool abort,bool remove,bool open,bool info)
{
  acceptButton->setEnabled(accept);
  abortButton->setEnabled(abort);
  removeButton->setEnabled(remove);
  openButton->setEnabled(open);
  infoButton->setEnabled(info);
}

void DccPanel::dccSelected()
{
  DccTransfer* item=(DccTransfer*) getListView()->selectedItem();
  kdDebug() << "(item)" << item << endl;

  if(item)
  {
    DccTransfer::DccStatus status=item->getStatus();
    switch(status)
    {
      case DccTransfer::Queued:
      case DccTransfer::Lookup:
      case DccTransfer::Connecting:
        setButtons(true,false,true,false,false);
        break;
      case DccTransfer::Offering:
      case DccTransfer::Resuming:
        setButtons(false,true,true,true,true);
        break;
      case DccTransfer::Sending:
      case DccTransfer::Receiving:
      case DccTransfer::Stalled:
        setButtons(false,true,true,true,true);
        break;
      case DccTransfer::Failed:
      case DccTransfer::Done:
        setButtons(false,false,true,true,true);
        break;
      default:
        setButtons(false,false,false,false,false);
    }
  }
  else setButtons(false,false,false,false,false);
}

void DccPanel::acceptDcc()
{
  DccTransfer* item=(DccTransfer*) getListView()->selectedItem();
  if(item)
  {
    if(item->getType()==DccTransfer::Get && item->getStatus()==DccTransfer::Queued) item->startGet();
  }
}

void DccPanel::runDcc()
{
  DccTransfer* item=(DccTransfer*) getListView()->selectedItem();
  if(item)
  {
    new KRun(item->getFile());
  }
}

void DccPanel::removeDcc()
{
  DccTransfer* item=(DccTransfer*) getListView()->selectedItem();

  if(item)
  {
    DccTransfer::DccStatus status=item->getStatus();
    bool doDelete=true;

    if(status!=DccTransfer::Queued &&
       status!=DccTransfer::Offering &&
       status!=DccTransfer::Aborted &&
       status!=DccTransfer::Failed &&
       status!=DccTransfer::Done)
    {
      // TODO: do some user question here
      doDelete=false;
    }

    if(doDelete) delete item;
  }
}

DccTransfer* DccPanel::getTransferByPort(QString port,DccTransfer::DccType type)
{
  int index=0;
  DccTransfer* item;
  do
  {
    // TODO: Get rid of this cast
    item=(DccTransfer*) getListView()->itemAtIndex(index++);
    if(item)
    {
      kdDebug() << item->getType() << endl;
      if(item->getStatus()<DccTransfer::Failed &&
         item->getType()==type &&
         item->getPort()==port) return item;
    }
  } while(item);

  return 0;
}
// To find the resuming dcc over firewalls that change the port numbers
DccTransfer* DccPanel::getTransferByName(QString name,DccTransfer::DccType type)
{
  int index=0;
  DccTransfer* item;
  do
  {
    // TODO: Get rid of this cast
    item=(DccTransfer*) getListView()->itemAtIndex(index++);
    if(item)
    {
      kdDebug() << name << " == " << item->getFile() << endl;
      if(item->getStatus()<DccTransfer::Failed &&
         item->getType()==type &&
         item->getFile()==name) return item;
    }
  } while(item);

  return 0;
}

int DccPanel::spacing() { return KDialog::spacingHint(); }
int DccPanel::margin()  { return KDialog::marginHint(); }

KListView* DccPanel::getListView() { return dccListView; }
