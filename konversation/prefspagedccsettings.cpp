/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides a user interface to customize DCC settings
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
                      DCC_Settings(newParent)
{
  preferences=newPreferences;
	setName("DCC Settings");
	
	kcfg_DccPath->setText(preferences->getDccPath());
	
  connect(kcfg_AutoGet, SIGNAL(stateChanged(int)), this, SLOT(autoGetStateChanged(int)));
  connect(kcfg_AutoResume, SIGNAL(stateChanged(int)), this, SLOT(autoResumeStateChanged(int)));

  kcfg_AddPartner->setChecked(preferences->getDccAddPartner());
  kcfg_CreateFolder->setChecked(preferences->getDccCreateFolder());
  kcfg_AutoGet->setChecked(preferences->getDccAutoGet());
  kcfg_AutoResume->setChecked(preferences->getDccAutoResume());

  // Dcc send timeout
  kcfg_SendTimeout->setValue(preferences->getDccSendTimeout());
  
  // own IP
  kcfg_MethodToGetOwnIp->insertItem(i18n("Network Interface"));
  kcfg_MethodToGetOwnIp->insertItem(i18n("Reply From IRC Server"));
  kcfg_MethodToGetOwnIp->insertItem(i18n("Specify Manually"));

  kcfg_SpecificOwnIp->setText(preferences->getDccSpecificOwnIp());

  connect(kcfg_MethodToGetOwnIp, SIGNAL(activated(int)), this, SLOT(methodToGetOwnIpComboBoxActivated(int)));
  kcfg_MethodToGetOwnIp->setCurrentItem(preferences->getDccMethodToGetOwnIp());
  methodToGetOwnIpComboBoxActivated(preferences->getDccMethodToGetOwnIp());

  // Ports Group 
  connect(kcfg_SendPortsFirst, SIGNAL(valueChanged(int)), this, SLOT(sendPortsFirstSpinValueChanged(int)));
  connect(kcfg_SendPortsLast, SIGNAL(valueChanged(int)), this, SLOT(sendPortsLastSpinValueChanged(int)));

  kcfg_SpecificSendPorts->setChecked(preferences->getDccSpecificSendPorts());
  kcfg_SendPortsFirst->setValue(preferences->getDccSendPortsFirst());
  kcfg_SendPortsLast->setValue(preferences->getDccSendPortsLast());

  // Ports specification for DCC chat
  connect(kcfg_ChatPortsFirst, SIGNAL(valueChanged(int)), this, SLOT(chatPortsFirstSpinValueChanged(int)));
  connect(kcfg_ChatPortsLast, SIGNAL(valueChanged(int)), this, SLOT(chatPortsLastSpinValueChanged(int)));

  kcfg_SpecificChatPorts->setChecked(preferences->getDccSpecificChatPorts());
  kcfg_ChatPortsFirst->setValue(preferences->getDccChatPortsFirst());
  kcfg_ChatPortsLast->setValue(preferences->getDccChatPortsLast());
  
  // buffer size
  kcfg_BufferSize->setValue(preferences->getDccBufferSize());
  kcfg_FastSend->setChecked(preferences->getDccFastSend());

  // Set up signals / slots for DCC Setup page
  connect(dccFolderButton,SIGNAL (clicked()),this,SLOT (folderButtonClicked()) );
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

    if(folderInfo.isDir()) kcfg_DccPath->setText(folderName);
    else KMessageBox::sorry(0,i18n("<qt>Error: %1 is not a regular folder.</qt>").arg(folderName),i18n("Incorrect Path"));
  }
}

void PrefsPageDccSettings::methodToGetOwnIpComboBoxActivated(int methodId)
{
  kcfg_SpecificOwnIp->setEnabled(methodId == 2);
	ownIP->setEnabled(methodId == 2);
}

void PrefsPageDccSettings::sendPortsFirstSpinValueChanged(int port)
{
  if(kcfg_SendPortsLast->value() < port)
    kcfg_SendPortsLast->setValue(port);
}

void PrefsPageDccSettings::sendPortsLastSpinValueChanged(int port)
{
  if(port < kcfg_SendPortsFirst->value())
    kcfg_SendPortsFirst->setValue(port);
}

void PrefsPageDccSettings::chatPortsFirstSpinValueChanged(int port)
{
  if(kcfg_ChatPortsLast->value() < port)
    kcfg_ChatPortsLast->setValue(port);
}

void PrefsPageDccSettings::chatPortsLastSpinValueChanged(int port)
{
  if(port < kcfg_ChatPortsFirst->value())
    kcfg_ChatPortsFirst->setValue(port);
}

void PrefsPageDccSettings::autoResumeStateChanged(int state)
{
  if(state == QButton::On)
  {
    kcfg_AutoGet->setChecked(true);
  }
}

void PrefsPageDccSettings::autoGetStateChanged(int state)
{
  if(state == QButton::Off && kcfg_AutoResume->isChecked())
  {
    kcfg_AutoResume->setChecked(false);
  }
}

void PrefsPageDccSettings::applyPreferences()
{
  preferences->setDccPath(kcfg_DccPath->text());
  preferences->setDccBufferSize(kcfg_BufferSize->value());
  preferences->setDccMethodToGetOwnIp(kcfg_MethodToGetOwnIp->currentItem());
  preferences->setDccSpecificOwnIp(kcfg_SpecificOwnIp->text());
  preferences->setDccSpecificSendPorts(kcfg_SpecificSendPorts->isChecked());
  preferences->setDccSendPortsFirst(kcfg_SendPortsFirst->value());
  preferences->setDccSendPortsLast(kcfg_SendPortsLast->value());
  preferences->setDccSpecificChatPorts(kcfg_SpecificChatPorts->isChecked());
  preferences->setDccChatPortsFirst(kcfg_ChatPortsFirst->value());
  preferences->setDccChatPortsLast(kcfg_ChatPortsLast->value());
  preferences->setDccAutoGet(kcfg_AutoGet->isChecked());
  preferences->setDccAutoResume(kcfg_AutoResume->isChecked());
  preferences->setDccAddPartner(kcfg_AddPartner->isChecked());
  preferences->setDccCreateFolder(kcfg_CreateFolder->isChecked());
  preferences->setDccFastSend(kcfg_FastSend->isChecked());
}

#include "prefspagedccsettings.moc"
