/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageosd.cpp  -  Configuration tab for the OSD Widget
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#include <qlayout.h>
#include <qhbox.h>
#include <qhbox.h>
#include <qvgroupbox.h>
#include <qgrid.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <klistview.h>
#include <klineeditdlg.h>
#include <kfontdialog.h>

#include "prefspageosd.h"
#include "preferences.h"
#include "konversationapplication.h"

PrefsPageOSD::PrefsPageOSD(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add the layout to the page
  QGridLayout* osdLayout = new QGridLayout(parentFrame, 3, 4, marginHint(), spacingHint());

  // Set up osd widgets
  QHBox* osdBox = new QHBox(parentFrame);
  osdBox->setSpacing(spacingHint());

  useOSDCheck = new QCheckBox(i18n("Use OnScreen Display"), osdBox, "use_osd_checkbox");

  // Set up osd widgets
  osdActionsBox = new QVGroupBox(i18n("Show OSD Message"), parentFrame, "osd_actions_group");
  osdShowOwnNick = new QCheckBox(i18n("If own nick appears in channel message"), osdActionsBox, "osd_show_ownnick");
  osdShowChannel = new QCheckBox(i18n("On any channel message"), osdActionsBox, "osd_show_channel");
  osdShowQuery = new QCheckBox(i18n("On query activity"), osdActionsBox, "osd_show_query");
  osdShowChannelEvent = new QCheckBox(i18n("On Join/Part events"), osdActionsBox, "osd_show_event");

  useOSDCheck->setChecked(preferences->getOSDUsage());
  osdShowOwnNick->setChecked(preferences->getOSDShowOwnNick());
  osdShowChannel->setChecked(preferences->getOSDShowChannel());
  osdShowQuery->setChecked(preferences->getOSDShowQuery());
  osdShowChannelEvent->setChecked(preferences->getOSDShowChannelEvent());

  // Font settings
  osdFontLabel = new QLabel(i18n("OSD font:"), parentFrame);
  osdFont = preferences->getOSDFont();
  osdPreviewLabel = new QLabel(parentFrame);
  osdPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  osdFontButton = new QPushButton(i18n("Choose..."), parentFrame, "osd_font_button");

  // Take care of ghosting / unghosting close button checkboxes
  osdUsageChanged(preferences->getOSDUsage() ? 2 : 0);

  // Update the preview
  updateFonts();

  // Define the layout
  int row = 0;
  osdLayout->addMultiCellWidget(osdBox, row, row, 0, 2);
  osdLayout->addWidget(osdFontLabel, ++row, 0);
  osdLayout->addWidget(osdPreviewLabel, row, 1);
  osdLayout->addWidget(osdFontButton, row, 2);
  osdLayout->addMultiCellWidget(osdActionsBox, ++row, row + 2, 0, 1);

  row = row + 2;
  QHBox* spacer = new QHBox(parentFrame);
  osdLayout->addMultiCellWidget(spacer, ++row, row + 1, 0, 2);
  osdLayout->setRowStretch(row, 2);
  osdLayout->setColStretch(1, 2);

  connect(useOSDCheck,SIGNAL (stateChanged(int)),this,SLOT (osdUsageChanged(int)) );
  connect(osdFontButton,SIGNAL (clicked()),this,SLOT (osdFontClicked()) );
}

PrefsPageOSD::~PrefsPageOSD()
{
}

void PrefsPageOSD::osdFontClicked()
{
  KFontDialog::getFont(osdFont);
  updateFonts();
}

void PrefsPageOSD::osdUsageChanged(int state)
{
  useOSDCheck->setChecked(state);
  osdFontLabel->setEnabled(state==2);
  osdPreviewLabel->setEnabled(state==2);
  osdFontButton->setEnabled(state==2);
  osdActionsBox->setEnabled(state==2);
}

void PrefsPageOSD::applyPreferences()
{
  preferences->setOSDUsage(useOSDCheck->isChecked());
  preferences->setOSDShowOwnNick(osdShowOwnNick->isChecked());
  preferences->setOSDShowChannel(osdShowChannel->isChecked());
  preferences->setOSDShowQuery(osdShowQuery->isChecked());
  preferences->setOSDShowChannelEvent(osdShowChannelEvent->isChecked());
  preferences->setOSDFont(osdFont);

  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  konvApp->osd->setEnabled(useOSDCheck->isChecked());
  if (preferences->getOSDUsage())
  {
    konvApp->osd->setFont(osdFont);
  }

}

void PrefsPageOSD::updateFonts()
{
  osdPreviewLabel->setFont(osdFont);
  osdPreviewLabel->setText(QString("%1 %2").arg(osdFont.family().section(':',0,0)).arg(osdFont.pointSize()));
}

#include "prefspageosd.moc"
