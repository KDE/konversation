//
// C++ Implementation: dccresumedialog
//
// Description: Lets the user decide if they want to overwrite, rename or cancel the resume
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qvbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qfileinfo.h>

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>

#include "dccresumedialog.h"

QString DccResumeDialog::newFilename;  // static

DccResumeDialog::DccResumeDialog(QWidget* parent,QString filename,const KURL& newBaseURL) :
               KDialogBase(parent,"dcc_resume_dialog",true,i18n("DCC Resume Question"),
                           KDialogBase::User1 | KDialogBase::User2 | KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,true)
{
  newFilename=filename;
  baseURL=newBaseURL;

  setButtonText(KDialogBase::User1,i18n("Rename"));
  setButtonText(KDialogBase::User2,i18n("Overwrite"));
  setButtonText(KDialogBase::Ok,i18n("Resume"));

  // deactivate rename until we actually renamed the file
  enableButton(KDialogBase::User1,false);

  // Create the top level widget
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  // Add the layout to the widget
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());

  QLabel* existsLabel=new QLabel(i18n("A file with the name '%1' already exists.").arg(newFilename),page,"file_exists_label");
  QHBox* nameBox=new QHBox(page);

  dialogLayout->addWidget(existsLabel);
  dialogLayout->addWidget(nameBox);
  
  nameInput=new KLineEdit(nameBox,"rename_input");
  nameInput->setText(newFilename);

  QPushButton* suggestButton=new QPushButton(i18n("Suggest New Name"),nameBox,"suggest_name_button");

  connect(nameInput,SIGNAL (textChanged(const QString&)),this,SLOT (filenameChanged(const QString&)) );
  connect(suggestButton,SIGNAL (clicked()),this,SLOT (suggestNewName()) );
  connect(this,SIGNAL (user1Clicked()),this,SLOT (renameClicked()) );
  connect(this,SIGNAL (user2Clicked()),this,SLOT (overwriteClicked()) );
}

DccResumeDialog::~DccResumeDialog()
{
}

int DccResumeDialog::ask(QWidget* parent,QString& filename,const KURL& url)
{
  DccResumeDialog dlg(parent,filename,url);
  int rc=dlg.exec();

  filename=newFilename.stripWhiteSpace();

  if(rc==QDialog::Accepted) rc=KDialogBase::Ok; // Why the hell does KDialogBase not return KDialogBase::Ok but this when using exec!?  
  return rc;
}

void DccResumeDialog::filenameChanged(const QString& newName)
{
  newFilename=newName;
  enableButton(KDialogBase::User1,true);
}

// taken and adapted from kio::renamedlg.cpp
void DccResumeDialog::suggestNewName()
{
  QString dotSuffix, suggestedName;
  QString basename = newFilename;

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

  newFilename=suggestedName;

  if ( exists ) // already exists -> recurse
    suggestNewName();

  nameInput->setText(newFilename);
}

void DccResumeDialog::renameClicked()
{
  done(KDialogBase::User1);
}

void DccResumeDialog::overwriteClicked()
{
  done(KDialogBase::User2);
}

#include "dccresumedialog.moc"
