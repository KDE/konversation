/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedccsettings.cpp  -  Provides a user interface to customize DCC settings
  begin:     Wed Oct 23 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qhbox.h>
#include <qspinbox.h>
#include <qfileinfo.h>

#include <klineedit.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "prefspagedccsettings.h"

PrefsPageDccSettings::PrefsPageDccSettings(QFrame* newParent,Preferences* newPreferences) :
                      PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the DCC settings pane
  QGridLayout* dccSettingsLayout=new QGridLayout(parentFrame,5,3,marginHint(),spacingHint(),"dcc_settings_layout");

  QLabel* dccFolderLabel=new QLabel(i18n("DCC Folder:"),parentFrame);
  dccFolderInput=new KLineEdit(preferences->getDccPath(),parentFrame);
  QPushButton* dccFolderButton=new QPushButton(i18n("Browse"),parentFrame,"dcc_folder_button");

  QHBox* dccSpinBoxes=new QHBox(parentFrame);
  dccSpinBoxes->setSpacing(spacingHint());

  new QLabel(i18n("Buffer Size:"),dccSpinBoxes);
  QSpinBox* dccBufferSpin=new QSpinBox(512,16384,128,dccSpinBoxes,"dcc_buffer_spin");
  dccBufferSpin->setSuffix(" "+i18n("bytes"));
  dccBufferSpin->setValue(preferences->getDccBufferSize());

  QLabel* dccRollbackLabel=new QLabel(i18n("Rollback:"),dccSpinBoxes);
  dccRollbackLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  QSpinBox* dccRollbackSpin=new QSpinBox(0,65536,512,dccSpinBoxes,"dcc_rollback_spin");
  dccRollbackSpin->setSuffix(" "+i18n("bytes"));
  dccRollbackSpin->setValue(preferences->getDccRollback());

  dccSpinBoxes->setStretchFactor(dccRollbackLabel,10);

  QCheckBox* dccAutoGet=new QCheckBox(i18n("Automatically accept DCC download"),parentFrame,"dcc_autoget_checkbox");
  QCheckBox* dccAddSender=new QCheckBox(i18n("Add sender to file name"),parentFrame,"dcc_sender_checkbox");
  QCheckBox* dccCreateFolder=new QCheckBox(i18n("Create folder for Sender"),parentFrame,"dcc_create_folder_checkbox");

  dccAddSender->setChecked(preferences->getDccAddPartner());
  dccCreateFolder->setChecked(preferences->getDccCreateFolder());
  dccAutoGet->setChecked(preferences->getDccAutoGet());

  QHBox* dccSpacer=new QHBox(parentFrame);

  int row=0;

  dccSettingsLayout->addWidget(dccFolderLabel,row,0);
  dccSettingsLayout->addWidget(dccFolderInput,row,1);
  dccSettingsLayout->addWidget(dccFolderButton,row,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccSpinBoxes,row,row,0,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccAutoGet,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccAddSender,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccCreateFolder,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccSpacer,row,row,0,2);
  dccSettingsLayout->setRowStretch(row,10);

  // Set up signals / slots for DCC Setup page
  connect(dccFolderInput,SIGNAL (textChanged(const QString&)),this,SLOT (folderInputChanged(const QString&)) );
  connect(dccFolderButton,SIGNAL (clicked()),this,SLOT (folderButtonClicked()) );
  connect(dccBufferSpin,SIGNAL (valueChanged(int)),this,SLOT (bufferValueChanged(int)) );
  connect(dccRollbackSpin,SIGNAL (valueChanged(int)),this,SLOT (rollbackValueChanged(int)));
  connect(dccAutoGet,SIGNAL (stateChanged(int)),this,SLOT (autoGetChanged(int)) );
  connect(dccAddSender,SIGNAL (stateChanged(int)),this,SLOT (addSenderChanged(int)) );
  connect(dccCreateFolder,SIGNAL (stateChanged(int)),this,SLOT (createFolderChanged(int)) );
}

PrefsPageDccSettings::~PrefsPageDccSettings()
{
}

void PrefsPageDccSettings::folderInputChanged(const QString& newFolder)
{
  preferences->setDccPath(newFolder);
}

void PrefsPageDccSettings::folderButtonClicked()
{
  QString folderName=KFileDialog::getExistingDirectory(
                                                        preferences->getDccPath(),
                                                        0,
                                                        i18n("Select DCC download folder")
                                                      );
  if(folderName!="")
  {
    QFileInfo folderInfo(folderName);

    if(folderInfo.isDir())
    {
      preferences->setDccPath(folderName);
      dccFolderInput->setText(folderName);
    }
    else KMessageBox::sorry(0,i18n("<qt>Error: %1 is not a regular folder!</qt>").arg(folderName),i18n("Incorrect path"));
  }
}

void PrefsPageDccSettings::bufferValueChanged(int newBuffer)
{
  preferences->setDccBufferSize(newBuffer);
}

void PrefsPageDccSettings::rollbackValueChanged(int newRollback)
{
  preferences->setDccRollback(newRollback);
}

void PrefsPageDccSettings::autoGetChanged(int state)
{
  preferences->setDccAutoGet(state==2);
}

void PrefsPageDccSettings::addSenderChanged(int state)
{
  preferences->setDccAddPartner(state==2);
}

void PrefsPageDccSettings::createFolderChanged(int state)
{
  preferences->setDccCreateFolder(state==2);
}

#include "prefspagedccsettings.moc"
