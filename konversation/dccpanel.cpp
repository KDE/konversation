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
*/

#include <qhbox.h>
#include <qheader.h>
#include <qpushbutton.h>
#include <qvbox.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kdialog.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>

#include "dccpanel.h"
#include "dcctransfer.h"

#ifdef KDE_IS_VERSION
#if KDE_IS_VERSION(3,1,1)
#define USE_INFOLIST
#endif
#endif

#ifdef USE_MDI
DccPanel::DccPanel(QString caption) : ChatWindow(caption)
#else
DccPanel::DccPanel(QWidget* parent) : ChatWindow(parent)
#endif
{
  setType(ChatWindow::DccPanel);

  dccListView=new KListView(this,"dcc_control_panel");
  
  dccListView->setSelectionMode(QListView::Extended);
  dccListView->setDragEnabled(true);
  dccListView->setAcceptDrops(true);
  dccListView->setSorting(-1,false);
  dccListView->setAllColumnsShowFocus(true);
  
  for(unsigned int i=0 ; i < Column::COUNT ; ++i)
  
    dccListView->addColumn("");
  
  //dccListView->setColumnText(Column::StatusIcon,    "");
  //dccListView->setColumnText(Column::TypeIcon,      "");
  dccListView->setColumnText(Column::OfferDate,     i18n("Date"));
  dccListView->setColumnText(Column::Status,        i18n("Status"));
  dccListView->setColumnText(Column::FileName,      i18n("File"));
  dccListView->setColumnText(Column::PartnerNick,   i18n("Partner"));
  dccListView->setColumnText(Column::Progress,      i18n("Progress"));
  dccListView->setColumnText(Column::Position,      i18n("Position"));
  dccListView->setColumnText(Column::TimeRemaining, i18n("Remain"));
  dccListView->setColumnText(Column::CPS,           i18n("Speed"));
  
  dccListView->setColumnWidth(Column::OfferDate,      70);
  dccListView->setColumnWidth(Column::Status,         80);
  dccListView->setColumnWidth(Column::FileName,      150);
  dccListView->setColumnWidth(Column::PartnerNick,    70);
  dccListView->setColumnWidth(Column::Progress,       90);
  dccListView->setColumnWidth(Column::Position,      120);
  dccListView->setColumnWidth(Column::TimeRemaining,  80);
  dccListView->setColumnWidth(Column::CPS,            70);
  
  dccListView->setColumnWidthMode(Column::FileName, QListView::Manual);
  
  dccListView->setColumnAlignment(Column::OfferDate,     AlignHCenter);
  dccListView->setColumnAlignment(Column::Progress,      AlignHCenter);
  dccListView->setColumnAlignment(Column::Position,      AlignHCenter);
  dccListView->setColumnAlignment(Column::TimeRemaining, AlignRight);
  dccListView->setColumnAlignment(Column::CPS,           AlignRight);
  
  QHBox* buttonsBox=new QHBox(this);
  buttonsBox->setSpacing(spacing());
  
  #define icon(s) KGlobal::iconLoader()->loadIcon( s, KIcon::Small )
  
  acceptButton=new QPushButton(icon("player_play"),i18n("Accept"),buttonsBox,"start_dcc");
  abortButton =new QPushButton(icon("stop"),i18n("Abort"),buttonsBox,"abort_dcc");
  removeButton=new QPushButton(icon("editdelete"), i18n("Remove"),buttonsBox,"remove_dcc");
  openButton  =new QPushButton(icon("exec"), i18n("Open"),buttonsBox,"open_dcc_file");
  infoButton  =new QPushButton(icon("messagebox_info"), i18n("Information"),buttonsBox,"info_on_dcc_file");
  detailButton=new QPushButton(icon("view_text"), i18n("Detail"),buttonsBox,"detail_dcc");

  connect(dccListView,SIGNAL (selectionChanged()),this,SLOT (selectionChanged()) );
  
  connect(acceptButton,SIGNAL (clicked()) ,this,SLOT (acceptDcc()) );
  connect(abortButton,SIGNAL (clicked()) ,this,SLOT (abortDcc()) );
  connect(removeButton,SIGNAL (clicked()) ,this,SLOT (removeDcc()) );
  connect(openButton,SIGNAL (clicked()) ,this,SLOT (runDcc()) );
  connect(infoButton,SIGNAL (clicked()) ,this,SLOT (showFileInfo()) );
  connect(detailButton,SIGNAL (clicked()) ,this,SLOT (openDetail()) );
}

DccPanel::~DccPanel()
{
  kdDebug() << "DccPanel::~DccPanel()" << endl;
}

void DccPanel::dccStatusChanged(const DccTransfer *item)
{
  // If the item is currently selected, update buttons.
  if (item->isSelected()) selectionChanged();
}

void DccPanel::selectionChanged()
{
  bool accept=true, abort=false, remove=true, open=true, info=true, detail=true;
  bool itemfound = false;
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item = static_cast<DccTransfer*>( it.current() );
      DccTransfer::DccType type = item->getType();
      DccTransfer::DccStatus status = item->getStatus();
      if( item )
      {
        itemfound = true;
        
        accept &= ( status == DccTransfer::Queued );
        
        abort  |= ( status != DccTransfer::Failed && 
                    status != DccTransfer::Aborted && 
                    status != DccTransfer::Done );
        
        open   &= ( type == DccTransfer::Send ||
                    status == DccTransfer::Done );
        
        info   &= ( type == DccTransfer::Send ||
                    status == DccTransfer::Done );
      }
    }
    ++it;
  }
  if( !itemfound ) { accept=false, abort=false, remove=false, open=false, info=false, detail=false; }
  
  acceptButton->setEnabled( accept );
  abortButton->setEnabled( abort );
  removeButton->setEnabled( remove );
  openButton->setEnabled( open );
  infoButton->setEnabled( info );
  detailButton->setEnabled( detail );
}
  
void DccPanel::acceptDcc()
{
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item )
        if( item->getType() == DccTransfer::Receive && item->getStatus() == DccTransfer::Queued )
          item->start();
    }
    ++it;
  }
}

void DccPanel::abortDcc()
{
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item )
        if( item->getStatus() != DccTransfer::Aborted && item->getStatus() != DccTransfer::Failed && item->getStatus() != DccTransfer::Done )
          item->abort();
    }
    ++it;
  }
}

void DccPanel::removeDcc()
{
  QPtrList<QListViewItem> lst;
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
      lst.append( it.current() );
    ++it;
  }
  lst.setAutoDelete( true );
  while( lst.remove() );
}

void DccPanel::runDcc()
{
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item )
        if( item->getType() == DccTransfer::Send || item->getStatus() == DccTransfer::Done )
          new KRun( KURL( item->getFilePath() ) );
    }
    ++it;
  }
}

void DccPanel::showFileInfo()
{
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item )
        if( item->getType() == DccTransfer::Send || item->getStatus() == DccTransfer::Done )
        {
          QStringList infoList;
        
          QString path=item->getFilePath();
          
          // get meta info object
          KFileMetaInfo* fileInfo=new KFileMetaInfo(path,QString::null,KFileMetaInfo::Everything);
        
          // is there any info for this file?
          if(fileInfo && !fileInfo->isEmpty())
          {
            // get list of meta information groups
            QStringList groupList=fileInfo->groups();
            // look inside for keys
            for(unsigned int index=0;index<groupList.count();index++)
            {
              // get next group
              KFileMetaInfoGroup group=fileInfo->group(groupList[index]);
              // check if there are keys in this group at all
              if(!group.isEmpty())
              {
                // append group name to list
                infoList.append(groupList[index]);
                // get list of keys in this group
                QStringList keys=group.keys();
                for(unsigned keyIndex=0;keyIndex<keys.count();keyIndex++)
                {
                  // get meta information item for this key
                  KFileMetaInfoItem item=group.item(keys[keyIndex]);
                  if(item.isValid())
                  {
                    // append item information to list
                    infoList.append("- "+item.translatedKey()+" "+item.string());
                  }
                } // endfor
              }
            } // endfor

            // display information list if any available
            if(infoList.count())
            {
#ifdef USE_INFOLIST
              KMessageBox::informationList(
                this,
                i18n("Available information for file %1:").arg(path),
                infoList,
                i18n("File information")
              );
#else
              KMessageBox::information(
                this,
                "<qt>"+infoList.join("<br>")+"</qt>",
                i18n("File information")
              );
#endif
            }
          }
          else
          {
            KMessageBox::sorry(this,i18n("No detailed information for this file found."),i18n("File information"));
          }
          delete fileInfo;
        }
    }
    ++it;
  }
}

void DccPanel::openDetail()
{
  QListViewItemIterator it( getListView() );
  while( it.current() )
  {
    if( it.current()->isSelected() )
    {
      DccTransfer* item=static_cast<DccTransfer*>( it.current() );
      if( item )
        item->openDetailDialog();
    }
    ++it;
  }
}

DccTransfer* DccPanel::getTransferByPort(const QString& port,DccTransfer::DccType type,bool resumed)
{
  int index=0;
  DccTransfer* item;
  do
  {
    // TODO: Get rid of this cast
    item=static_cast<DccTransfer*>(getListView()->itemAtIndex(index++));
    if(item)
    {
      if( (item->getStatus()==DccTransfer::Queued || item->getStatus()==DccTransfer::WaitingRemote) &&
         item->getType()==type &&
         !(resumed && !item->isResumed()) &&
         item->getOwnPort()==port) return item;
    }
  } while(item);

  return 0;
}

// To find the resuming dcc over firewalls that change the port numbers
DccTransfer* DccPanel::getTransferByName(const QString& name,DccTransfer::DccType type,bool resumed)
{
  int index=0;
  DccTransfer* item;
  do
  {
    // TODO: Get rid of this cast
    item=static_cast<DccTransfer*>(getListView()->itemAtIndex(index++));
    if(item)
    {
      if( (item->getStatus()==DccTransfer::Queued || item->getStatus()==DccTransfer::WaitingRemote) &&
         item->getType()==type &&
         !(resumed && !item->isResumed()) &&
         item->getFileName()==name) return item;
    }
  } while(item);

  return 0;
}

#ifdef USE_MDI
void DccPanel::closeYourself(ChatWindow*)
{
  emit chatWindowCloseRequest(this);
}
#endif

// virtual
void DccPanel::adjustFocus()
{
}

KListView* DccPanel::getListView() { return dccListView; }

#include "dccpanel.moc"
