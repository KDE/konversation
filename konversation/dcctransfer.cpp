/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  dcctransfer.cpp  -  description
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qheader.h>
#include <qhostaddress.h>
#include <qstyle.h>
#include <qtimer.h>

#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprogress.h>
#include <krun.h>

#include "dccdetaildialog.h"
#include "dccpanel.h"
#include "dcctransfer.h"
#include "konversationapplication.h"

DccTransfer::DccTransfer(KListView* _parent, DccType _dccType, const QString& _partnerNick, const QString& _fileName)
  : KListViewItem(_parent)
{
  dccType = _dccType;
  partnerNick = _partnerNick;
  fileName = _fileName;
  
  dccStatus = Queued;
  bResumed = false;
  transferringPosition = 0;
  transferStartPosition = 0;
  partnerIp = QString::null;
  partnerPort = QString::null;
  ownIp = QString::null;
  ownPort = QString::null;
  timeOffer = QDateTime::currentDateTime();
  
  bufferSize = KonversationApplication::preferences.getDccBufferSize();
  buffer = new char[bufferSize];
  
  autoUpdateViewTimer = 0;
  
  progressBar = new KProgress(100, listView()->viewport());
  progressBar->setCenterIndicator(true);
  progressBar->setPercentageVisible(true);
  
  detailDialog = 0;
  
  // FIXME: we shouldn't do these init in the instance constructer
  TypeText[Send]    = i18n("Send");
  TypeText[Receive] = i18n("Receive");
  
  StatusText[Queued]        = i18n("Queued");
  StatusText[WaitingRemote] = i18n("Offering");
  StatusText[Connecting]    = i18n("Connecting");
  StatusText[Sending]       = i18n("Sending");
  StatusText[Receiving]     = i18n("Receiving");
  StatusText[Done]          = i18n("Done");
  StatusText[Failed]        = i18n("Failed");
  StatusText[Aborted]       = i18n("Aborted");
  StatusText[Removed]       = i18n("Removed");
}

DccTransfer::~DccTransfer()
{
  stopAutoUpdateView();
  closeDetailDialog();
  delete[] buffer;
  delete progressBar;
}

void DccTransfer::updateView()  // slot
{
  setPixmap(DccPanel::Column::TypeIcon, getTypeIcon());
  setPixmap(DccPanel::Column::Status,   getStatusIcon());
  
  setText(DccPanel::Column::OfferDate,     timeOffer.toString("hh:mm:ss"));
  setText(DccPanel::Column::Status,        getStatusText());
  setText(DccPanel::Column::FileName,      fileName);
  setText(DccPanel::Column::PartnerNick,   partnerNick);
  setText(DccPanel::Column::Position,      getPositionPrettyText());
  setText(DccPanel::Column::TimeRemaining, getTimeRemainingPrettyText());
  setText(DccPanel::Column::CPS,           getCPSPrettyText());
  
  if(m_fileSize)
    progressBar->setProgress((int)(100*transferringPosition/m_fileSize));
  else  // filesize is unknown
    setText(DccPanel::Column::Progress, i18n("unknown"));
  
  if(detailDialog)
    detailDialog->updateView();
}

void DccTransfer::startAutoUpdateView()
{
  stopAutoUpdateView();
  autoUpdateViewTimer = new QTimer(this);
  connect(autoUpdateViewTimer, SIGNAL(timeout()), this, SLOT(updateView()));
  autoUpdateViewTimer->start(500);
}

void DccTransfer::stopAutoUpdateView()
{
  if(autoUpdateViewTimer)
  {
    autoUpdateViewTimer->stop();
    delete autoUpdateViewTimer;
    autoUpdateViewTimer = 0;
  }
}

void DccTransfer::paintCell(QPainter* painter, const QColorGroup& colorgroup, int column, int width, int alignment)  // public virtual
{
  KListViewItem::paintCell(painter, colorgroup, column, width, alignment);
  if(column==DccPanel::Column::Progress)
    showProgressBar();
}

void DccTransfer::showProgressBar()
{  
  // I've referenced the Apollon's code for progressbar things. Thank you, the Apollon team! (shin)
  if(m_fileSize)
  {
    QRect rect = listView()->itemRect(this);
    QHeader *head = listView()->header();
    rect.setLeft(head->sectionPos(DccPanel::Column::Progress) - head->offset());
    rect.setWidth(head->sectionSize(DccPanel::Column::Progress));
    progressBar->setGeometry(rect);
    
    progressBar->show();
  }
}

void DccTransfer::runFile()  // public
{
  if ( dccType == Send || dccStatus == Done )
    new KRun( m_fileURL );
}

bool DccTransfer::removeFile()  // public
{
  if ( dccType != Receive || dccStatus != Done )
    return false;
  if ( !QFile( m_fileURL.path() ).remove() )
  {
    KMessageBox::sorry( 0, i18n("Cannot remove file '%1'.").arg( m_fileURL.url() ), i18n("DCC Error") );
    return false;
  }
  setStatus( Removed );
  updateView();
  return true;
}

void DccTransfer::openFileInfoDialog()
{
  if( dccType == Send || dccStatus == Done )
  {
    QStringList infoList;
    
    QString path=m_fileURL.path();
    
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
          listView(),
          i18n("Available information for file %1:").arg(path),
          infoList,
          i18n("File information")
        );
#else
        KMessageBox::information(
          listView(),
          "<qt>"+infoList.join("<br>")+"</qt>",
          i18n("File information")
        );
#endif
      }
    }
    else
    {
      KMessageBox::sorry(listView(),i18n("No detailed information for this file found."),i18n("File information"));
    }
    delete fileInfo;
  }
}

void DccTransfer::openDetailDialog()  // public
{
  if(!detailDialog)
    detailDialog = new DccDetailDialog(this);
  detailDialog->show();
}

void DccTransfer::closeDetailDialog()  // public
{
  if(detailDialog)
  {
    delete detailDialog;
    detailDialog = 0;
  }
}

void DccTransfer::setStatus(DccStatus status, const QString& statusDetail)
{
  DccStatus oldStatus=dccStatus;
  dccStatus=status;
  dccStatusDetail=statusDetail;
  if (oldStatus!=status) emit statusChanged(this);
}

QString DccTransfer::getTypeText() const
{
  return TypeText[dccType];
}

QPixmap DccTransfer::getTypeIcon() const
{
  QString icon;
  if(dccType == Send)
    icon = "up";
  else if(dccType == Receive)
    icon = "down";
  return KGlobal::iconLoader()->loadIcon(icon, KIcon::Small);
}

QPixmap DccTransfer::getStatusIcon() const
{
  QString icon;
  switch(dccStatus)
  {
    case Queued:
      icon = "player_stop";
      break;
    case WaitingRemote:
    case Connecting:
      icon = "goto";
      break;
    case Sending:
    case Receiving:
      icon = "player_play";
      break;
    case Done:
      icon = "ok";
      break;
    case Aborted:
    case Failed:
      icon = "stop";
      break;
    case Removed:
      icon = "trashcan_full";
      break;
    default: ;  // sugar
  }
  return KGlobal::iconLoader()->loadIcon(icon, KIcon::Small);
}

QString DccTransfer::getStatusText() const
{
  return StatusText[dccStatus];
}

QString DccTransfer::getFileSizePrettyText() const
{
  return KIO::convertSize(m_fileSize);
}

QString DccTransfer::getPositionPrettyText() const
{
  // TODO: make it pretty!
  return KIO::convertSize(transferringPosition) + " / " + KIO::convertSize(m_fileSize);
}
QString DccTransfer::getTimeRemainingPrettyText() const
{
  if(dccStatus != Sending && dccStatus != Receiving )
    return QString::null;
  if(!m_fileSize)
    return i18n("unknown");
  // not use getCPS() for exact result
  int trnsfdTime = timeTransferStarted.secsTo(QDateTime::currentDateTime());
  KIO::fileoffset_t trnsfdBytes = transferringPosition - transferStartPosition;
  if(trnsfdBytes == 0)
    return QString::null;
  KIO::fileoffset_t remBytes = m_fileSize - transferringPosition;
  int remTime = (int)((double)remBytes / (double)trnsfdBytes * trnsfdTime);
  int remHour = remTime / 3600; remTime -= remHour * 3600;
  int remMin = remTime / 60; remTime -= remMin * 60;
  QString text;
  if(remHour)
    text += QString::number(remHour) + ":";
  if(remMin)
    text += QString::number(remMin) + ":";
  if(text.isEmpty())
    text = QString::number(remTime) + i18n(" sec");
  else
    text += QString::number(remTime);
  return text;
}

QString DccTransfer::getCPSPrettyText() const
{
  // TODO: make it pretty!
  return QString::number(getCPS()) + " B/s";
}

unsigned long DccTransfer::getCPS() const
{
  int elapsed=timeTransferStarted.secsTo(QDateTime::currentDateTime());
  if(elapsed == 0 && transferringPosition-transferStartPosition > 0)
    elapsed = 1;
  // prevent division by zero
  return elapsed ? (transferringPosition-transferStartPosition)/elapsed : 0;
}

QString DccTransfer::getNumericalIpText(const QString& _ip)  // static
{
  QHostAddress ip;
  ip.setAddress(_ip);
  
  return QString::number(ip.ip4Addr());
}

QString DccTransfer::getErrorString(int code)  // static
{
  QString errorString(QString::null);

  switch(code)
  {
    case IO_Ok:
      errorString=i18n("The operation was successful. Should never happen in an error dialog.");
    break;
    case IO_ReadError:
      errorString=i18n("Could not read from file \"%1\".");
    break;
    case IO_WriteError:
      errorString=i18n("Could not write to file \"%1\".");
    break;
    case IO_FatalError:
      errorString=i18n("A fatal unrecoverable error occurred.");
    break;
    case IO_OpenError:
      errorString=i18n("Could not open file \"%1\".");
    break;

        // Same case value? Damn!
//        case IO_ConnectError:
//          errorString="Could not connect to the device.";
//        break;

    case IO_AbortError:
      errorString=i18n("The operation was unexpectedly aborted.");
    break;
    case IO_TimeOutError:
      errorString=i18n("The operation timed out.");
    break;
    case IO_UnspecifiedError:
      errorString=i18n("An unspecified error happened on close.");
    break;
    default:
      errorString=i18n("Unknown error. Code %1").arg(code);
    break;
  }

  return errorString;
}

unsigned long DccTransfer::intel(unsigned long value)  // static
{
  value=((value & 0xff000000) >> 24) +
        ((value & 0xff0000) >> 8) +
        ((value & 0xff00) << 8) +
        ((value & 0xff) << 24);

  return value;
}


DccTransfer::DccType DccTransfer::getType() const { return dccType; }
DccTransfer::DccStatus DccTransfer::getStatus() const { return dccStatus; }
QString DccTransfer::getOwnIp() const { return ownIp; }
QString DccTransfer::getOwnPort() const { return ownPort; }
QString DccTransfer::getPartnerNick() const { return partnerNick; }
QString DccTransfer::getFileName() const { return fileName; }
KIO::filesize_t DccTransfer::getFileSize() const { return m_fileSize; }
KURL DccTransfer::getFileURL() const { return m_fileURL; }
bool DccTransfer::isResumed() const { return bResumed; }

QString DccTransfer::TypeText[DccTransfer::DccTypeCount];
QString DccTransfer::StatusText[DccTransfer::DccStatusCount];



#include "dcctransfer.moc"
