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
*/

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qhbox.h>
#include <qspinbox.h>
#include <qfileinfo.h>
#include <qvgroupbox.h>

#include <klineedit.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>

#include "prefspagedccsettings.h"
#include "preferences.h"

PrefsPageDccSettings::PrefsPageDccSettings(QFrame* newParent,Preferences* newPreferences) :
                      PrefsPage(newParent,newPreferences)
{
  setName("DCC Settings");
  // Add a Layout to the DCC settings pane
  QGridLayout* dccSettingsLayout=new QGridLayout(parentFrame,6,3,marginHint(),spacingHint(),"dcc_settings_layout");

  QLabel* dccFolderLabel=new QLabel(i18n("DCC &folder:"),parentFrame);
  dccFolderInput=new KLineEdit(preferences->getDccPath(),parentFrame);
  QPushButton* dccFolderButton=new QPushButton(i18n("&Browse..."),parentFrame,"dcc_folder_button");

  dccFolderLabel->setBuddy(dccFolderInput);

  QHBox* dccSpinBoxes=new QHBox(parentFrame);
  dccSpinBoxes->setSpacing(spacingHint());

  QLabel* dccBufferLabel=new QLabel(i18n("B&uffer size:"),dccSpinBoxes);
  dccBufferSpin=new QSpinBox(512,16384,128,dccSpinBoxes,"dcc_buffer_spin");
  dccBufferSpin->setSuffix(" "+i18n("bytes"));
  dccBufferSpin->setValue(preferences->getDccBufferSize());

  dccBufferLabel->setBuddy(dccBufferSpin);

  QLabel* dccRollbackLabel=new QLabel(i18n("&Rollback:"),dccSpinBoxes);
  dccRollbackLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  dccRollbackSpin=new QSpinBox(0,65536,512,dccSpinBoxes,"dcc_rollback_spin");
  dccRollbackSpin->setSuffix(" "+i18n("bytes"));
  dccRollbackSpin->setValue(preferences->getDccRollback());

  dccRollbackLabel->setBuddy(dccRollbackSpin);

  dccSpinBoxes->setStretchFactor(dccRollbackLabel,10);
  
  // Ports specification for DCC sending
  dccSpecificSendPortsBox = new QVGroupBox(i18n("Use specific &ports for DCC Sending"), parentFrame, "dcc_specific_send_ports_box");
  dccSpecificSendPortsBox->setCheckable(TRUE);
  
  QHBox *dccSendPortsFirstBox = new QHBox(dccSpecificSendPortsBox);
  dccSendPortsFirstLabel=new QLabel(i18n("&First DCC send port:"),dccSendPortsFirstBox);
  dccSendPortsFirstSpin=new QSpinBox(0,65535,1,dccSendPortsFirstBox,"dcc_send_ports_first_spin");
  connect(dccSendPortsFirstSpin, SIGNAL(valueChanged(int)), this, SLOT(sendPortsFirstSpinValueChanged(int)));
  dccSendPortsFirstSpin->setMaximumWidth(100);
  dccSendPortsFirstLabel->setBuddy(dccSendPortsFirstSpin);
  
  QHBox *dccSendPortsLastBox = new QHBox(dccSpecificSendPortsBox);
  dccSendPortsLastLabel=new QLabel(i18n("&Last DCC send port:"),dccSendPortsLastBox);
  dccSendPortsLastSpin=new QSpinBox(0,65535,1,dccSendPortsLastBox,"dcc_send_ports_last_spin");
  connect(dccSendPortsLastSpin, SIGNAL(valueChanged(int)), this, SLOT(sendPortsLastSpinValueChanged(int)));
  dccSendPortsLastSpin->setMaximumWidth(100);
  dccSendPortsLastLabel->setBuddy(dccSendPortsLastSpin);
  
  dccSpecificSendPortsBox->setChecked(preferences->getDccSpecificSendPorts());
  dccSendPortsFirstSpin->setValue(preferences->getDccSendPortsFirst());
  dccSendPortsLastSpin->setValue(preferences->getDccSendPortsLast());
  
  // Ports specification for DCC chat
  dccSpecificChatPortsBox = new QVGroupBox(i18n("Use specific &ports for DCC Chat"), parentFrame, "dcc_specific_chat_ports_box");
  dccSpecificChatPortsBox->setCheckable(TRUE);
  
  QHBox *dccChatPortsFirstBox = new QHBox(dccSpecificChatPortsBox);
  dccChatPortsFirstLabel=new QLabel(i18n("&First DCC chat port:"),dccChatPortsFirstBox);
  dccChatPortsFirstSpin=new QSpinBox(0,65535,1,dccChatPortsFirstBox,"dcc_chat_ports_first_spin");
  connect(dccChatPortsFirstSpin, SIGNAL(valueChanged(int)), this, SLOT(chatPortsFirstSpinValueChanged(int)));
  dccChatPortsFirstSpin->setMaximumWidth(100);
  dccChatPortsFirstLabel->setBuddy(dccChatPortsFirstSpin);
  
  QHBox *dccChatPortsLastBox = new QHBox(dccSpecificChatPortsBox);
  dccChatPortsLastLabel=new QLabel(i18n("&Last DCC chat port:"),dccChatPortsLastBox);
  dccChatPortsLastSpin=new QSpinBox(0,65535,1,dccChatPortsLastBox,"dcc_chat_ports_last_spin");
  connect(dccChatPortsLastSpin, SIGNAL(valueChanged(int)), this, SLOT(chatPortsLastSpinValueChanged(int)));
  dccChatPortsLastSpin->setMaximumWidth(100);
  dccChatPortsLastLabel->setBuddy(dccChatPortsLastSpin);
  
  dccSpecificChatPortsBox->setChecked(preferences->getDccSpecificChatPorts());
  dccChatPortsFirstSpin->setValue(preferences->getDccChatPortsFirst());
  dccChatPortsLastSpin->setValue(preferences->getDccChatPortsLast());
  
  dccGetIpFromServer=new QCheckBox(i18n("&Get own IP from IRC server"),parentFrame,"dcc_get_ip_from_server_checkbox");
  dccAutoGet=new QCheckBox(i18n("Automatically accept &DCC download"),parentFrame,"dcc_autoget_checkbox");
  connect(dccAutoGet, SIGNAL(stateChanged(int)), this, SLOT(autoGetStateChanged(int)));
  dccAutoResume=new QCheckBox(i18n("Au&tomatically resume DCC download"), parentFrame,"dcc_autoresume_checkbox");
  connect(dccAutoResume, SIGNAL(stateChanged(int)), this, SLOT(autoResumeStateChanged(int)));
  dccAddSender=new QCheckBox(i18n("Add &sender to file name"),parentFrame,"dcc_sender_checkbox");
  dccCreateFolder=new QCheckBox(i18n("Cr&eate folder for sender"),parentFrame,"dcc_create_folder_checkbox");

  dccGetIpFromServer->setChecked(preferences->getDccGetIpFromServer());
  dccAddSender->setChecked(preferences->getDccAddPartner());
  dccCreateFolder->setChecked(preferences->getDccCreateFolder());
  dccAutoGet->setChecked(preferences->getDccAutoGet());
  dccAutoResume->setChecked(preferences->getDccAutoResume());

  QHBox* dccSpacer=new QHBox(parentFrame);

  int row=0;

  dccSettingsLayout->addWidget(dccFolderLabel,row,0);
  dccSettingsLayout->addWidget(dccFolderInput,row,1);
  dccSettingsLayout->addWidget(dccFolderButton,row,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccSpinBoxes,row,row,0,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccSpecificSendPortsBox,row,row,0,2);
  row++;
  
  dccSettingsLayout->addMultiCellWidget(dccSpecificChatPortsBox,row,row,0,2);
  row++;
  
  dccSettingsLayout->addMultiCellWidget(dccGetIpFromServer,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccAutoGet,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccAutoResume,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccAddSender,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccCreateFolder,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccSpacer,row,row,0,2);
  dccSettingsLayout->setRowStretch(row,10);

  // Set up signals / slots for DCC Setup page
  connect(dccFolderButton,SIGNAL (clicked()),this,SLOT (folderButtonClicked()) );
}

PrefsPageDccSettings::~PrefsPageDccSettings()
{
}

void PrefsPageDccSettings::folderButtonClicked()
{
  QString folderName=KFileDialog::getExistingDirectory(
                                                        preferences->getDccPath(),
                                                        0,
                                                        i18n("Select DCC Download Folder")
                                                      );
  if(!folderName.isEmpty())
  {
    QFileInfo folderInfo(folderName);

    if(folderInfo.isDir()) dccFolderInput->setText(folderName);
    else KMessageBox::sorry(0,i18n("<qt>Error: %1 is not a regular folder.</qt>").arg(folderName),i18n("Incorrect Path"));
  }
}

void PrefsPageDccSettings::sendPortsFirstSpinValueChanged(int port)
{
  if(dccSendPortsLastSpin->value() < port)
    dccSendPortsLastSpin->setValue(port);
}

void PrefsPageDccSettings::sendPortsLastSpinValueChanged(int port)
{
  if(port < dccSendPortsFirstSpin->value())
    dccSendPortsFirstSpin->setValue(port);
}

void PrefsPageDccSettings::chatPortsFirstSpinValueChanged(int port)
{
  if(dccChatPortsLastSpin->value() < port)
    dccChatPortsLastSpin->setValue(port);
}

void PrefsPageDccSettings::chatPortsLastSpinValueChanged(int port)
{
  if(port < dccChatPortsFirstSpin->value())
    dccChatPortsFirstSpin->setValue(port);
}

void PrefsPageDccSettings::autoResumeStateChanged(int state)
{
  if(state == QButton::On)
  {
    dccAutoGet->setChecked(true);
  }
}

void PrefsPageDccSettings::autoGetStateChanged(int state)
{
  if(state == QButton::Off && dccAutoResume->isChecked())
  {
    dccAutoResume->setChecked(false);
  }
}

void PrefsPageDccSettings::applyPreferences()
{
  preferences->setDccPath(dccFolderInput->text());
  preferences->setDccBufferSize(dccBufferSpin->value());
  preferences->setDccRollback(dccRollbackSpin->value());
  preferences->setDccSpecificSendPorts(dccSpecificSendPortsBox->isChecked());
  preferences->setDccSendPortsFirst(dccSendPortsFirstSpin->value());
  preferences->setDccSendPortsLast(dccSendPortsLastSpin->value());
  preferences->setDccSpecificChatPorts(dccSpecificChatPortsBox->isChecked());
  preferences->setDccChatPortsFirst(dccChatPortsFirstSpin->value());
  preferences->setDccChatPortsLast(dccChatPortsLastSpin->value());
  preferences->setDccGetIpFromServer(dccGetIpFromServer->isChecked());
  preferences->setDccAutoGet(dccAutoGet->isChecked());
  preferences->setDccAutoResume(dccAutoResume->isChecked());
  preferences->setDccAddPartner(dccAddSender->isChecked());
  preferences->setDccCreateFolder(dccCreateFolder->isChecked());
}

#include "prefspagedccsettings.moc"
