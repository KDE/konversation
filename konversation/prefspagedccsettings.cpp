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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfileinfo.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
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

  dccAddSender=new QCheckBox(i18n("Add &sender to file name"),parentFrame,"dcc_sender_checkbox");
  dccCreateFolder=new QCheckBox(i18n("Cr&eate folder for sender"),parentFrame,"dcc_create_folder_checkbox");
  dccAutoGet=new QCheckBox(i18n("Automatically accept &DCC download"),parentFrame,"dcc_autoget_checkbox");
  connect(dccAutoGet, SIGNAL(stateChanged(int)), this, SLOT(autoGetStateChanged(int)));
  dccAutoResume=new QCheckBox(i18n("Au&tomatically resume DCC download"), parentFrame,"dcc_autoresume_checkbox");
  connect(dccAutoResume, SIGNAL(stateChanged(int)), this, SLOT(autoResumeStateChanged(int)));

  dccAddSender->setChecked(preferences->getDccAddPartner());
  dccCreateFolder->setChecked(preferences->getDccCreateFolder());
  dccAutoGet->setChecked(preferences->getDccAutoGet());
  dccAutoResume->setChecked(preferences->getDccAutoResume());

  // Dcc send timeout
  QFrame* dccSendTimeoutFrame=new QFrame(parentFrame);
  QHBoxLayout* dccSendTimeoutLayout=new QHBoxLayout(dccSendTimeoutFrame);
  dccSendTimeoutLayout->setSpacing(spacingHint());
  QLabel* dccSendTimeoutLabel=new QLabel(i18n("DCC send t&imeout:"),dccSendTimeoutFrame);
  dccSendTimeoutSpin=new QSpinBox(1,300,1,dccSendTimeoutFrame,"dcc_send_timeout");
  dccSendTimeoutSpin->setSuffix(" "+i18n("sec"));
  dccSendTimeoutSpin->setValue(preferences->getDccSendTimeout());
  dccSendTimeoutLabel->setBuddy(dccSendTimeoutSpin);
  dccSendTimeoutLayout->addWidget(dccSendTimeoutLabel);
  dccSendTimeoutLayout->addWidget(dccSendTimeoutSpin);
  dccSendTimeoutLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
  
  // own IP
  QVGroupBox* dccOwnIpGroup = new QVGroupBox(i18n("IP"), parentFrame, "dcc_own_ip_group");

  QHBox* dccMethodToGetOwnIpBox = new QHBox(dccOwnIpGroup);
  QLabel* dccMethodToGetOwnIpLabel = new QLabel(i18n("&Method to get own IP:"), dccMethodToGetOwnIpBox);
  dccMethodToGetOwnIpComboBox = new QComboBox(dccMethodToGetOwnIpBox, "dcc_method_to_get_own_ip_combo");
  dccMethodToGetOwnIpComboBox->insertItem(i18n("Network Interface"));
  dccMethodToGetOwnIpComboBox->insertItem(i18n("Reply From IRC Server"));
  dccMethodToGetOwnIpComboBox->insertItem(i18n("Specify Manually"));
  dccMethodToGetOwnIpLabel->setBuddy(dccMethodToGetOwnIpComboBox);

  dccSpecificOwnIpFrame = new QFrame(dccOwnIpGroup);
  QHBoxLayout* dccSpecificOwnIpLayout = new QHBoxLayout(dccSpecificOwnIpFrame);
  dccSpecificOwnIpLayout->setSpacing(spacingHint());
  QLabel* dccSpecificOwnIpLabel = new QLabel(i18n("O&wn IP:"), dccSpecificOwnIpFrame);
  dccSpecificOwnIpInput = new KLineEdit(preferences->getDccSpecificOwnIp(),dccSpecificOwnIpFrame);
  dccSpecificOwnIpInput->setFixedWidth(150);
  dccSpecificOwnIpLabel->setBuddy(dccSpecificOwnIpInput);
  dccSpecificOwnIpLayout->addWidget(dccSpecificOwnIpLabel);
  dccSpecificOwnIpLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
  dccSpecificOwnIpLayout->addWidget(dccSpecificOwnIpInput);

  connect(dccMethodToGetOwnIpComboBox, SIGNAL(activated(int)), this, SLOT(methodToGetOwnIpComboBoxActivated(int)));
  dccMethodToGetOwnIpComboBox->setCurrentItem(preferences->getDccMethodToGetOwnIp());
  methodToGetOwnIpComboBoxActivated(preferences->getDccMethodToGetOwnIp());

  // Ports Group {
  QVGroupBox* dccPortsGroup = new QVGroupBox(i18n("Port"), parentFrame, "dcc_ports_group");

  // Ports specification for DCC sending
  dccSpecificSendPortsCheckBox = new QCheckBox(i18n("Use specific &ports for DCC send"), dccPortsGroup, "dcc_specific_send_ports_box");

  QFrame* dccSpecificSendPortsFrame = new QFrame(dccPortsGroup);
  QHBoxLayout* dccSpecificSendPortsLayout = new QHBoxLayout(dccSpecificSendPortsFrame);
  dccSpecificSendPortsLayout->setSpacing(spacingHint());

  QLabel* dccSendPortsLabel=new QLabel(i18n("DCC se&nd ports:"),dccSpecificSendPortsFrame);
  dccSendPortsFirstSpin=new QSpinBox(0,65535,1,dccSpecificSendPortsFrame,"dcc_send_ports_first_spin");
  connect(dccSendPortsFirstSpin, SIGNAL(valueChanged(int)), this, SLOT(sendPortsFirstSpinValueChanged(int)));
  dccSendPortsFirstSpin->setFixedWidth(80);
  QLabel* dccSendPortsCenterLabel=new QLabel(i18n("to"),dccSpecificSendPortsFrame);
  dccSendPortsLastSpin=new QSpinBox(0,65535,1,dccSpecificSendPortsFrame,"dcc_send_ports_last_spin");
  connect(dccSendPortsLastSpin, SIGNAL(valueChanged(int)), this, SLOT(sendPortsLastSpinValueChanged(int)));
  dccSendPortsLastSpin->setFixedWidth(80);

  dccSpecificSendPortsFrame->setEnabled(dccSpecificSendPortsCheckBox->isChecked());
  connect(dccSpecificSendPortsCheckBox, SIGNAL(toggled(bool)), dccSpecificSendPortsFrame, SLOT(setEnabled(bool)));

  dccSendPortsLabel->setBuddy(dccSendPortsFirstSpin);
  dccSpecificSendPortsLayout->addWidget(dccSendPortsLabel);
  dccSpecificSendPortsLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
  dccSpecificSendPortsLayout->addWidget(dccSendPortsFirstSpin);
  dccSpecificSendPortsLayout->addWidget(dccSendPortsCenterLabel);
  dccSpecificSendPortsLayout->addWidget(dccSendPortsLastSpin);

  dccSpecificSendPortsCheckBox->setChecked(preferences->getDccSpecificSendPorts());
  dccSendPortsFirstSpin->setValue(preferences->getDccSendPortsFirst());
  dccSendPortsLastSpin->setValue(preferences->getDccSendPortsLast());

  // Ports specification for DCC chat
  dccSpecificChatPortsCheckBox = new QCheckBox(i18n("Use specific p&orts for DCC chat"), dccPortsGroup, "dcc_specific_chat_ports_box");

  QFrame* dccSpecificChatPortsFrame = new QFrame(dccPortsGroup);
  QHBoxLayout* dccSpecificChatPortsLayout = new QHBoxLayout(dccSpecificChatPortsFrame);
  dccSpecificChatPortsLayout->setSpacing(spacingHint());

  QLabel* dccChatPortsLabel=new QLabel(i18n("DCC &chat ports:"),dccSpecificChatPortsFrame);
  dccChatPortsFirstSpin=new QSpinBox(0,65535,1,dccSpecificChatPortsFrame,"dcc_chat_ports_first_spin");
  connect(dccChatPortsFirstSpin, SIGNAL(valueChanged(int)), this, SLOT(chatPortsFirstSpinValueChanged(int)));
  dccChatPortsFirstSpin->setFixedWidth(80);
  QLabel* dccChatPortsCenterLabel=new QLabel(i18n("to"),dccSpecificChatPortsFrame);
  dccChatPortsLastSpin=new QSpinBox(0,65535,1,dccSpecificChatPortsFrame,"dcc_chat_ports_last_spin");
  connect(dccChatPortsLastSpin, SIGNAL(valueChanged(int)), this, SLOT(chatPortsLastSpinValueChanged(int)));
  dccChatPortsLastSpin->setFixedWidth(80);

  dccSpecificChatPortsFrame->setEnabled(dccSpecificChatPortsCheckBox->isChecked());
  connect(dccSpecificChatPortsCheckBox, SIGNAL(toggled(bool)), dccSpecificChatPortsFrame, SLOT(setEnabled(bool)));

  dccChatPortsLabel->setBuddy(dccChatPortsFirstSpin);
  dccSpecificChatPortsLayout->addWidget(dccChatPortsLabel);
  dccSpecificChatPortsLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
  dccSpecificChatPortsLayout->addWidget(dccChatPortsFirstSpin);
  dccSpecificChatPortsLayout->addWidget(dccChatPortsCenterLabel);
  dccSpecificChatPortsLayout->addWidget(dccChatPortsLastSpin);

  dccSpecificChatPortsCheckBox->setChecked(preferences->getDccSpecificChatPorts());
  dccChatPortsFirstSpin->setValue(preferences->getDccChatPortsFirst());
  dccChatPortsLastSpin->setValue(preferences->getDccChatPortsLast());
  // }
  
  // buffer size
  QFrame* dccBufferSizeFrame=new QFrame(parentFrame);
  QHBoxLayout* dccBufferSizeLayout=new QHBoxLayout(dccBufferSizeFrame);
  dccBufferSizeLayout->setSpacing(spacingHint());
  QLabel* dccBufferSizeLabel=new QLabel(i18n("Buffer si&ze:"),dccBufferSizeFrame);
  dccBufferSpin=new QSpinBox(512,13684,128,dccBufferSizeFrame,"dcc_buffer_spin");
  dccBufferSpin->setSuffix(" "+i18n("bytes"));
  dccBufferSpin->setValue(preferences->getDccBufferSize());
  dccBufferSizeLabel->setBuddy(dccBufferSpin);
  dccBufferSizeLayout->addWidget(dccBufferSizeLabel);
  dccBufferSizeLayout->addWidget(dccBufferSpin);
  dccBufferSizeLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
  
  dccFastSend=new QCheckBox(i18n("&Use fast DCC send (Might not work with all clients)"),parentFrame,"dcc_fast_dcc_sending");
  dccFastSend->setChecked(preferences->getDccFastSend());

  QHBox* dccSpacer=new QHBox(parentFrame);

  int row=0;

  dccSettingsLayout->addWidget(dccFolderLabel,row,0);
  dccSettingsLayout->addWidget(dccFolderInput,row,1);
  dccSettingsLayout->addWidget(dccFolderButton,row,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccAddSender,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccCreateFolder,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccAutoGet,row,row,0,2);
  row++;
  dccSettingsLayout->addMultiCellWidget(dccAutoResume,row,row,0,2);
  row++;
  
  dccSettingsLayout->addMultiCellWidget(dccSendTimeoutFrame,row,row,0,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccOwnIpGroup,row,row,0,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccPortsGroup,row,row,0,2);
  row++;

  dccSettingsLayout->addMultiCellWidget(dccBufferSizeFrame,row,row,0,2);
  row++;
  
  dccSettingsLayout->addMultiCellWidget(dccFastSend,row,row,0,2);
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

void PrefsPageDccSettings::methodToGetOwnIpComboBoxActivated(int methodId)
{
  dccSpecificOwnIpFrame->setEnabled(methodId == 2);
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
  preferences->setDccMethodToGetOwnIp(dccMethodToGetOwnIpComboBox->currentItem());
  preferences->setDccSpecificOwnIp(dccSpecificOwnIpInput->text());
  preferences->setDccSpecificSendPorts(dccSpecificSendPortsCheckBox->isChecked());
  preferences->setDccSendPortsFirst(dccSendPortsFirstSpin->value());
  preferences->setDccSendPortsLast(dccSendPortsLastSpin->value());
  preferences->setDccSpecificChatPorts(dccSpecificChatPortsCheckBox->isChecked());
  preferences->setDccChatPortsFirst(dccChatPortsFirstSpin->value());
  preferences->setDccChatPortsLast(dccChatPortsLastSpin->value());
  preferences->setDccAutoGet(dccAutoGet->isChecked());
  preferences->setDccAutoResume(dccAutoResume->isChecked());
  preferences->setDccAddPartner(dccAddSender->isChecked());
  preferences->setDccCreateFolder(dccCreateFolder->isChecked());
  preferences->setDccFastSend(dccFastSend->isChecked());
}

#include "prefspagedccsettings.moc"
