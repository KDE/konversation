/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#include "prefspagechatwinappearance.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qvgroupbox.h>
#include <qvbox.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>

#include <kfontdialog.h>
#include <kdebug.h>
#include <klocale.h>

#include "preferences.h"

PrefsPageChatWinAppearance::PrefsPageChatWinAppearance(QFrame* newParent,Preferences* newPreferences)
 : PrefsPage(newParent, newPreferences)
{
  QGridLayout* chatLayout = new QGridLayout(parentFrame, 4, 3, marginHint(), spacingHint());

  // Font settings
  QGroupBox* fontGBox = new QGroupBox(i18n("Fo&nts"), parentFrame, "fons_groupbox");
  fontGBox->setColumnLayout(0, Qt::Vertical);
  fontGBox->setMargin(marginHint());
  QGridLayout* fontLayout = new QGridLayout(fontGBox->layout(), 1, 3, spacingHint());

  QLabel* textFontLabel = new QLabel(i18n("Chat text:"),fontGBox);
  textFont=preferences->getTextFont();
  textPreviewLabel = new QLabel(fontGBox);
  textPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  QPushButton* textFontButton = new QPushButton(i18n("Choos&e..."),fontGBox,"text_font_button");
  connect(textFontButton, SIGNAL(clicked()), this, SLOT(textFontClicked()));

  QLabel* listFontLabel = new QLabel(i18n("Nickname list:"), fontGBox);
  listFont = preferences->getListFont();
  listPreviewLabel = new QLabel(fontGBox);
  listPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  QPushButton* listFontButton = new QPushButton(i18n("C&hoose..."), fontGBox, "list_font_button");
  connect(listFontButton, SIGNAL(clicked()), this, SLOT(listFontClicked()));

  fixedMOTDCheck = new QCheckBox(i18n("&Use a fixed font for MOTD messages"), fontGBox, "fixed_motd_check");
  fixedMOTDCheck->setChecked(preferences->getFixedMOTD());

  updateFonts();

  int row = 0;
  fontLayout->addWidget(textFontLabel, row, 0);
  fontLayout->addWidget(textPreviewLabel, row, 1);
  fontLayout->addWidget(textFontButton, row, 2);
  row++;
  fontLayout->addWidget(listFontLabel, row, 0);
  fontLayout->addWidget(listPreviewLabel, row, 1);
  fontLayout->addWidget(listFontButton, row, 2);
  row++;
  fontLayout->addMultiCellWidget(fixedMOTDCheck, row, row, 0, 2);
  fontLayout->setColStretch(1, 10);

  QVGroupBox* timestampBox = new QVGroupBox(i18n("Timestamps"), parentFrame);

  doTimestamping = new QCheckBox(i18n("Show &timestamps"), timestampBox, "show_timestamps_checkbox");

  showDate=new QCheckBox(i18n("Show &dates"), timestampBox, "show_date_checkbox");
  showDate->setChecked(preferences->getShowDate());

  QHBox* stampFormatBox = new QHBox(timestampBox);
  formatLabel = new QLabel(i18n("&Format:"), stampFormatBox);

  timestampFormat = new QComboBox(false, stampFormatBox, "timestamp_format_combo");
  timestampFormat->insertItem("hh");
  timestampFormat->insertItem("hh:mm");
  timestampFormat->insertItem("hh:mm:ss");
  timestampFormat->insertItem("hh ap");
  timestampFormat->insertItem("hh:mm ap");
  timestampFormat->insertItem("hh:mm:ss ap");

  // link label shortcut to combo box
  formatLabel->setBuddy(timestampFormat);

  connect(doTimestamping, SIGNAL(stateChanged(int)), this, SLOT(timestampingChanged(int)));

  // find actual timestamp format
  for(int index=0; index < timestampFormat->count(); index++) {
    if(timestampFormat->text(index) == preferences->getTimestampFormat()) {
      timestampFormat->setCurrentItem(index);
    }
  }

  // Take care of ghosting / unghosting format widget
  timestampingChanged(preferences->getTimestamping() ? 2 : 0);

  QVGroupBox* layoutGroup = new QVGroupBox(i18n("&Layout"), parentFrame, "layout_options_group");

  showTopic=new QCheckBox(i18n("&Show channel topic"), layoutGroup, "show_topic");
  showTopic->setChecked(preferences->getShowTopic());

  showModeButtons=new QCheckBox(i18n("Show channel &mode buttons"), layoutGroup, "show_modebuttons_checkbox");
  showModeButtons->setChecked(preferences->getShowModeButtons());

  showQuickButtons=new QCheckBox(i18n("Show quick &buttons"), layoutGroup, "show_quickbuttons_checkbox");
  showQuickButtons->setChecked(preferences->getShowQuickButtons());

  autoUserhostCheck=new QCheckBox(i18n("Show hostmasks &in nickname list"), layoutGroup, "auto_userhost_check");
  autoUserhostCheck->setChecked(preferences->getAutoUserhost());

  QGroupBox* backgroundImageBox = new QGroupBox("Use Back&ground Image", parentFrame);
  backgroundImageBox->setColumnLayout(0, Qt::Horizontal);
  backgroundImageBox->setMargin(marginHint());
  backgroundImageBox->setCheckable(TRUE);
  backgroundImageBox->setChecked(preferences->getShowBackgroundImage());
  backgroundImageBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

  QGridLayout* backgroundImageLayout=new QGridLayout(backgroundImageBox->layout(),4,2,spacingHint(),"background_image_layout");

  QLabel* backgroundLabel = new QLabel(i18n("&Path:"), backgroundImageBox );
  backgroundURL = new KURLRequester(backgroundImageBox, "background_image_url");

  backgroundURL->setCaption(i18n("Select Background Image"));
  backgroundURL->setURL(preferences->getBackgroundImageName());
  backgroundLabel->setBuddy(backgroundLabel);

  backgroundImageLayout->addWidget(backgroundLabel,0,0);
  backgroundImageLayout->addWidget(backgroundURL,0,1);

  connect(backgroundImageBox,SIGNAL(toggled(bool)),this, SLOT(setBackgroundImageConfig(bool)));
  connect(backgroundURL,SIGNAL(textChanged(const QString&)),this,SLOT(saveBackgroundImage(const QString&)));

  row = 0;
  chatLayout->addMultiCellWidget(fontGBox, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(timestampBox, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(layoutGroup, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(backgroundImageBox, row, row, 0, 2);
  row++;
  chatLayout->setRowStretch(row, 10);
}

PrefsPageChatWinAppearance::~PrefsPageChatWinAppearance()
{
}

void PrefsPageChatWinAppearance::textFontClicked()
{
  KFontDialog::getFont(textFont);
  updateFonts();
}

void PrefsPageChatWinAppearance::listFontClicked()
{
  KFontDialog::getFont(listFont);
  updateFonts();
}

void PrefsPageChatWinAppearance::setBackgroundImageConfig(bool state )
{
    if( !state )
        preferences->setShowBackgroundImage(FALSE );
    else {
        preferences->setBackgroundImageName(backgroundURL->url());
        preferences->setShowBackgroundImage(TRUE);
    }
}

void PrefsPageChatWinAppearance::saveBackgroundImage(const QString& url)
{
    preferences->setShowBackgroundImage(TRUE);
    preferences->setBackgroundImageName(url);
}

void PrefsPageChatWinAppearance::updateFonts()
{
  textPreviewLabel->setFont(textFont);
  listPreviewLabel->setFont(listFont);

  textPreviewLabel->setText(QString("%1 %2").arg(textFont.family().section(':',0,0)).arg(textFont.pointSize()));
  listPreviewLabel->setText(QString("%1 %2").arg(listFont.family().section(':',0,0)).arg(listFont.pointSize()));
}

void PrefsPageChatWinAppearance::timestampingChanged(int state)
{
  doTimestamping->setChecked(state == 2);
  timestampFormat->setEnabled(state == 2);
  formatLabel->setEnabled(state == 2);
  showDate->setEnabled(state == 2);

  if(state != 2)
  {
    showDate->setChecked(false);
  }
}

void PrefsPageChatWinAppearance::applyPreferences()
{
  preferences->setTextFont(textFont);
  preferences->setListFont(listFont);
  preferences->setFixedMOTD(fixedMOTDCheck->isChecked());
  preferences->setTimestamping(doTimestamping->isChecked());
  preferences->setShowDate(showDate->isChecked());
  preferences->setTimestampFormat(timestampFormat->currentText());
  preferences->setShowQuickButtons(showQuickButtons->isChecked());
  preferences->setShowModeButtons(showModeButtons->isChecked());
  preferences->setAutoUserhost(autoUserhostCheck->isChecked());
  preferences->setShowTopic(showTopic->isChecked());
}

#include "prefspagechatwinappearance.moc"
