/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagedccsettings.cpp  -  description
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

#include <klineedit.h>

#include <kdebug.h>

#include "prefspagedccsettings.h"

PrefsPageDccSettings::PrefsPageDccSettings(QFrame* newParent,Preferences* newPreferences) :
                      PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the DCC settings pane
  QGridLayout* dccSettingsLayout=new QGridLayout(parentFrame,5,3,marginHint(),spacingHint(),"dcc_settings_layout");

  QLabel* dccFolderLabel=new QLabel(i18n("DCC Folder:"),parentFrame);
  KLineEdit* dccFolderInput=new KLineEdit(preferences->getDccPath(),parentFrame);
  QPushButton* dccFolderButton=new QPushButton(i18n("Browse"),parentFrame,"dcc_folder_button");

  QLabel* dccBufferLabel=new QLabel(i18n("Buffer Size:"),parentFrame);
  KLineEdit* dccBufferInput=new KLineEdit(QString::number(preferences->getDccBufferSize()),parentFrame,"dcc_buffer_size_input");
  QLabel* dccBufferUnit=new QLabel(i18n("bytes"),parentFrame);

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

  dccSettingsLayout->addWidget(dccBufferLabel,row,0);
  dccSettingsLayout->addWidget(dccBufferInput,row,1);
  dccSettingsLayout->addWidget(dccBufferUnit,row,2);
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
  connect(dccBufferInput,SIGNAL (textChanged(const QString&)),this,SLOT (bufferInputChanged(const QString&)) );
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

void PrefsPageDccSettings::bufferInputChanged(const QString& newBuffer)
{
  preferences->setDccBufferSize(newBuffer.toUInt());
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
