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
#include <kcombobox.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kfontdialog.h>

#include "prefspageosd.h"
#include "preferences.h"
#include "konversationapplication.h"
#include "osd.h"

PrefsPageOSD::PrefsPageOSD(QFrame* newParent,Preferences* newPreferences) :
                 PrefsPage(newParent,newPreferences)
{
  // Add the layout to the page
  QGridLayout* osdLayout = new QGridLayout(parentFrame, 3, 4, marginHint(), spacingHint());

  // Set up osd widgets
  QHBox* osdBox = new QHBox(parentFrame);
  osdBox->setSpacing(spacingHint());

  useOSDCheck = new QCheckBox(i18n("&Use On Screen Display"), osdBox, "use_osd_checkbox");


  // Set up osd widgets
  osdActionsBox = new QVGroupBox(i18n("Show OSD Message"), parentFrame, "osd_actions_group");
  osdShowOwnNick = new QCheckBox(i18n("If &own nick appears in channel message"), osdActionsBox, "osd_show_ownnick");
  osdShowChannel = new QCheckBox(i18n("On any &channel message"), osdActionsBox, "osd_show_channel");
  osdShowQuery = new QCheckBox(i18n("&On query activity"), osdActionsBox, "osd_show_query");
  osdShowChannelEvent = new QCheckBox(i18n("On &Join/Part events"), osdActionsBox, "osd_show_event");

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
  osdFontButton = new QPushButton(i18n("C&hoose..."), parentFrame, "osd_font_button");
  connect(osdFontButton, SIGNAL(clicked()), this, SLOT(osdFontClicked()));

  drawShadowsCheck = new QCheckBox(i18n("&Draw shadows"), parentFrame, "draw_shadows_checkbox");
  drawShadowsCheck->setChecked(preferences->getOSDDrawShadow());

  //color box
  osdColorsBox = new QVGroupBox(i18n("Colors"), parentFrame, "osd_colors_group");
  useCustomColorsCheck = new QCheckBox(i18n("&Use custom colors"), osdColorsBox, "use_custom_colors_checkbox");
  useCustomColorsCheck->setChecked(preferences->getOSDUseCustomColors());

  QHBox *osdTextColorBox = new QHBox(osdColorsBox);
  osdTextColorLabel = new QLabel(i18n("Text co&lor:"), osdTextColorBox);
  osdTextColorChooser = new KColorCombo(osdTextColorBox, "osd_text_color");
  osdTextColorChooser->setColor(preferences->getOSDTextColor());
  osdTextColorLabel->setBuddy(osdTextColorChooser);

  QHBox *osdBackgroundColorBox = new QHBox(osdColorsBox);
  osdBackgroundColorLabel = new QLabel(i18n("&Background color:"), osdBackgroundColorBox);
  osdBackgroundColorChooser = new KColorCombo(osdBackgroundColorBox, "osd_background_color");
  osdBackgroundColorChooser->setColor(preferences->getOSDBackgroundColor());
  osdBackgroundColorLabel->setBuddy(osdBackgroundColorChooser);

   //others box
  osdOthersBox = new QVGroupBox("Other settings", parentFrame, "osd_others_group");
  QHBox* durationBox = new QHBox(osdOthersBox);
  QLabel *osdDurationLabel = new QLabel(i18n("&Duration:"), durationBox);
  osdDurationSpin = new KIntSpinBox(durationBox);
  osdDurationSpin->setSuffix("ms");
  osdDurationSpin->setMaxValue( 10000 );
  osdDurationSpin->setMinValue( 500 );
  osdDurationSpin->setLineStep( 1000 );
  osdDurationSpin->setValue(preferences->getOSDDuration());
  osdDurationLabel->setBuddy(osdDurationSpin);

  QHBox* screenBox = new QHBox(osdOthersBox);
  QLabel *osdScreenLabel = new QLabel(i18n("&Screen:"), screenBox);
  osdScreenCombo = new KComboBox(screenBox,"osd_screen_combo");
  osdScreenLabel->setBuddy(osdScreenCombo);
  osdScreenCombo->setCurrentText(QString::number(preferences->getOSDScreen()));

  const int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
      osdScreenCombo->insertItem( QString::number( i ) );


  // Take care of ghosting / unghosting close button checkboxes
  osdUsageChanged(preferences->getOSDUsage() ? 2 : 0);
  customColorsCheckStateChanged(useCustomColorsCheck->state());

  // Update the preview
  updateFonts();

  // Define the layout
  int row = 0;
  osdLayout->addMultiCellWidget(osdBox, row, row, 0, 2);
  osdLayout->addWidget(osdFontLabel, ++row, 0);
  osdLayout->addWidget(osdPreviewLabel, row, 1);
  osdLayout->addWidget(osdFontButton, row, 2);
  osdLayout->addWidget(drawShadowsCheck, ++row, 0);
  osdLayout->addMultiCellWidget(osdColorsBox, ++row, row + 2, 0, 2);
  row++; row++;
  osdLayout->addMultiCellWidget(osdOthersBox, ++row, row + 1, 0, 2);
  osdLayout->addMultiCellWidget(osdActionsBox, ++row, row + 2, 0, 2);

  row = row + 2;
  QHBox* spacer = new QHBox(parentFrame);
  osdLayout->addMultiCellWidget(spacer, ++row, row + 1, 0, 2);
  osdLayout->setRowStretch(row, 2);
  osdLayout->setColStretch(1, 2);

  connect(useOSDCheck, SIGNAL(stateChanged(int)), this, SLOT(osdUsageChanged(int))); //this connect must be placed after the osdUsageChanged call.
  connect(useCustomColorsCheck, SIGNAL(stateChanged(int)), this, SLOT(customColorsCheckStateChanged(int)));

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
  drawShadowsCheck->setEnabled(state==2);
  osdFontLabel->setEnabled(state==2);
  osdPreviewLabel->setEnabled(state==2);
  osdFontButton->setEnabled(state==2);
  osdColorsBox->setEnabled(state==2);
  osdOthersBox->setEnabled(state==2);
  osdActionsBox->setEnabled(state==2);
}

void PrefsPageOSD::customColorsCheckStateChanged(int state)
{
  useCustomColorsCheck->setChecked(state);
  osdTextColorLabel->setEnabled(state==2);
  osdTextColorChooser->setEnabled(state==2);
  osdBackgroundColorLabel->setEnabled(state==2);
  osdBackgroundColorChooser->setEnabled(state==2);
}

void PrefsPageOSD::applyPreferences()
{
  preferences->setOSDUsage(useOSDCheck->isChecked());
  preferences->setOSDShowOwnNick(osdShowOwnNick->isChecked());
  preferences->setOSDShowChannel(osdShowChannel->isChecked());
  preferences->setOSDShowQuery(osdShowQuery->isChecked());
  preferences->setOSDShowChannelEvent(osdShowChannelEvent->isChecked());
  preferences->setOSDFont(osdFont);
  preferences->setOSDUseCustomColors(useCustomColorsCheck->isChecked());
  preferences->setOSDTextColor(osdTextColorChooser->color().name());
  preferences->setOSDBackgroundColor(osdBackgroundColorChooser->color().name());
  preferences->setOSDDuration(osdDurationSpin->value());
  preferences->setOSDScreen(osdScreenCombo->currentText().toUInt());
  preferences->setOSDDrawShadow(drawShadowsCheck->isChecked());

  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  konvApp->osd->setEnabled(useOSDCheck->isChecked());
  if (preferences->getOSDUsage())
  {
    konvApp->osd->setFont(osdFont);
    if(useCustomColorsCheck->isChecked())
    {
      konvApp->osd->setTextColor(osdTextColorChooser->color());
      konvApp->osd->setBackgroundColor(osdBackgroundColorChooser->color());
    }
    else
    {
      konvApp->osd->unsetColors();
    }
    konvApp->osd->setDuration(osdDurationSpin->value());
    konvApp->osd->setScreen(osdScreenCombo->currentText().toUInt());
    konvApp->osd->setShadow(drawShadowsCheck->isChecked());
  }

}

void PrefsPageOSD::updateFonts()
{
  osdPreviewLabel->setFont(osdFont);
  osdPreviewLabel->setText(QString("%1 %2").arg(osdFont.family().section(':',0,0)).arg(osdFont.pointSize()));
}


#include "prefspageosd.moc"
