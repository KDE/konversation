//
// C++ Implementation: dccresumedialog
//
// Description: Lets the user decide if they want to overwrite, rename or cancel the resume
//
//
// Authors: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//          Shintaro Matsuoka <shin@shoegazed.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kurlrequester.h>

#include "dcctransferrecv.h"

#include "dccresumedialog.h"

DccResumeDialog::DccResumeDialog(DccTransferRecv* parentItem)
  : KDialogBase(0, "dcc_resume_dialog", true, 
                i18n("DCC Receive Question"),
                parentItem->bCompletedFileExists ? KDialogBase::Ok | KDialogBase::Cancel : KDialogBase::User1 | KDialogBase::Ok | KDialogBase::Cancel,
                parentItem->bCompletedFileExists ? KDialogBase::Ok : KDialogBase::User1,
                true)
  , item(parentItem)
{
  setButtonText(KDialogBase::User1, i18n("Resume"));
  
  QVBox* page = new QVBox(this);
  setMainWidget(page);
  
  QLabel* topMessage = new QLabel(page);
  if(item->bCompletedFileExists)
    topMessage->setText( i18n("<qt>A file with the name <b>%1</b> already exists.<br>")
                         .arg(item->filePath.section("/", -1))
                       );
  /*
    topMessage->setText( i18n("<qt>A file with the name <b>%1</b> already exists.<br>"
                              "%2<br>"  // full path (local)
                              "Local file size: %3 bytes<br>"
                              "Receiving file size: %4 bytes</qt>")
                         .arg(item->filePath.section("/", -1))
                         .arg(item->filePath)
                         .arg(QFile(item->filePath).size())
                         .arg(item->fileSize)
                       );
  */
  else
    topMessage->setText( i18n("<qt>A part of the file <b>%1</b> exists.<br>")
                         .arg(item->filePath.section("/", -1))
                       );
  
  urlreqFilePath = new KURLRequester(item->filePath, page);
  
  QFrame* filePathToolsFrame = new QFrame(page);
  QHBoxLayout* filePathToolsLayout = new QHBoxLayout(filePathToolsFrame);
  filePathToolsLayout->setSpacing(spacingHint());
  QPushButton* btnDefaultName = new QPushButton(i18n("Default"), filePathToolsFrame);
  QPushButton* btnSuggestNewName = new QPushButton(i18n("Suggest New Name"), filePathToolsFrame);
  filePathToolsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
  filePathToolsLayout->addWidget(btnDefaultName);
  filePathToolsLayout->addWidget(btnSuggestNewName);
  
  updateDialogButtons();
  setInitialSize(QSize(500, sizeHint().height()));
  
  connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUser1Clicked()));
  
  connect(urlreqFilePath, SIGNAL(textChanged(const QString&)), this, SLOT(updateDialogButtons()));
  
  connect(btnSuggestNewName, SIGNAL(clicked()), this, SLOT(suggestNewName()));
  connect(btnDefaultName, SIGNAL(clicked()), this, SLOT(setDefaultName()));
}

DccResumeDialog::~DccResumeDialog()
{
}

DccResumeDialog::ReceiveAction DccResumeDialog::ask(DccTransferRecv* item)  // public static
{
  DccResumeDialog dlg(item);
  int rc=dlg.exec();
  
  ReceiveAction ra;
  if(rc == QDialog::Accepted || rc == KDialogBase::User1)  // Why the hell does KDialogBase not return KDialogBase::Ok but this when using exec!?  
    ra = dlg.action;
  else
    ra = Cancel;
  
  if(ra == Rename)
  {
    item->filePath = dlg.urlreqFilePath->url().stripWhiteSpace();
    item->fileTmpPath = item->filePath + ".part";
  }
  
  return ra;
}

void DccResumeDialog::slotOkClicked()  // slot
{
  if(item->filePath == urlreqFilePath->url().stripWhiteSpace())
    action = Overwrite;
  else
    action = Rename;
}

void DccResumeDialog::slotUser1Clicked()  // slot
{
  action = Resume;
  done(KDialogBase::User1);
}

void DccResumeDialog::updateDialogButtons()  // slot
{
  if(item->filePath == urlreqFilePath->url().stripWhiteSpace())
  {
    setButtonText(KDialogBase::Ok, i18n("Overwrite"));
    if(!item->bCompletedFileExists)
      enableButton(KDialogBase::User1, true);
  }
  else
  {
    setButtonText(KDialogBase::Ok, i18n("Rename"));
    if(!item->bCompletedFileExists)
      enableButton(KDialogBase::User1, false);
  }
}

// taken and adapted from kio::renamedlg.cpp
void DccResumeDialog::suggestNewName()  // slot
{
  QString dotSuffix, suggestedName;
  QString basename = urlreqFilePath->url().section("/", -1);
  KURL baseURL(urlreqFilePath->url().section("/", 0, -2));
  
  int index = basename.find( '.' );
  if ( index != -1 ) {
    dotSuffix = basename.mid( index );
    basename.truncate( index );
  }

  int pos = basename.findRev( '_' );
  if(pos != -1 ){
    QString tmp = basename.mid( pos+1 );
    bool ok;
    int number = tmp.toInt( &ok );
    if ( !ok ) {// ok there is no number
      suggestedName = basename + "1" + dotSuffix;
    }
    else {
     // yes there's already a number behind the _ so increment it by one
      basename.replace( pos+1, tmp.length(), QString::number(number+1) );
      suggestedName = basename + dotSuffix;
    }
  }
  else // no underscore yet
    suggestedName = basename + "_1" + dotSuffix ;

  // Check if suggested name already exists
  bool exists = false;
  // TODO: network transparency. However, using NetAccess from a modal dialog
  // could be a problem, no? (given that it uses a modal widget itself....)
  if ( baseURL.isLocalFile() )
     exists = QFileInfo( baseURL.path(+1) + suggestedName ).exists();

  urlreqFilePath->setURL( baseURL.path(+1) + suggestedName );

  if ( exists ) // already exists -> recurse
    suggestNewName();
}

void DccResumeDialog::setDefaultName()  // slot
{
  urlreqFilePath->setURL(item->filePath);
}

#include "dccresumedialog.moc"
