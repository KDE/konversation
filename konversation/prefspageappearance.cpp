/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageappearance.cpp  -  The preferences panel that holds the appearance settings
  begin:     Son Dez 22 2002
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
  email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qvbox.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qtabwidget.h>

#include <kfontdialog.h>
#include <kdebug.h>
#include <kcharsets.h>
#include <klistview.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kurlrequester.h>

#include "prefspageappearance.h"
#include "preferences.h"
#include "valuelistviewitem.h"

PrefsPageAppearance::PrefsPageAppearance(QFrame* newParent,Preferences* newPreferences) :
                     PrefsPage(newParent,newPreferences)
{
  QVBoxLayout* layout = new QVBoxLayout(parentFrame);
  QTabWidget* tabWidget = new QTabWidget(parentFrame);
  
  QWidget* chatTab = new QWidget(tabWidget, "chatWindowAppearance");
  tabWidget->addTab(chatTab, i18n("Chat &Window"));
  QGridLayout* chatLayout = new QGridLayout(chatTab, 4, 3, marginHint(), spacingHint());
  
  // Font settings
  QGroupBox* fontGBox = new QGroupBox(i18n("Fonts"), chatTab, "fons_groupbox");
  fontGBox->setColumnLayout(0, Qt::Vertical);
  fontGBox->setMargin(marginHint());
  QGridLayout* fontLayout = new QGridLayout(fontGBox->layout(), 1, 3, spacingHint());
  
  QLabel* textFontLabel = new QLabel(i18n("Chat text:"),fontGBox);
  textFont=preferences->getTextFont();
  textPreviewLabel = new QLabel(fontGBox);
  textPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  QPushButton* textFontButton = new QPushButton(i18n("&Choose..."),fontGBox,"text_font_button");
  connect(textFontButton, SIGNAL(clicked()), this, SLOT(textFontClicked()));

  QLabel* listFontLabel = new QLabel(i18n("Nickname list:"), fontGBox);
  listFont = preferences->getListFont();
  listPreviewLabel = new QLabel(fontGBox);
  listPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  QPushButton* listFontButton = new QPushButton(i18n("C&hoose..."), fontGBox, "list_font_button");
  connect(listFontButton, SIGNAL(clicked()), this, SLOT(listFontClicked()));

  fixedMOTDCheck = new QCheckBox(i18n("Use a fixed font for &MOTD messages"), fontGBox, "fixed_motd_check");
  fixedMOTDCheck->setChecked(preferences->getFixedMOTD());
  
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
  
  QVGroupBox* timestampBox = new QVGroupBox(i18n("Timestamps"), chatTab);
  
  doTimestamping=new QCheckBox(i18n("Show &timestamps"),timestampBox,"show_timestamps_checkbox");

  showDate=new QCheckBox(i18n("Show &dates"),timestampBox,"show_date_checkbox");
  showDate->setChecked(preferences->getShowDate());

  QHBox* stampFormatBox = new QHBox(timestampBox);
  formatLabel=new QLabel(i18n("&Format:"),stampFormatBox);

  timestampFormat=new QComboBox(false,stampFormatBox,"timestamp_format_combo");
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

  QVGroupBox* layoutGroup = new QVGroupBox(i18n("Layout"), chatTab, "layout_options_group");
  
  showTopic=new QCheckBox(i18n("Show channel topic"), layoutGroup, "show_topic");
  showTopic->setChecked(preferences->getShowTopic());
    
  showModeButtons=new QCheckBox(i18n("Show channel &mode buttons"), layoutGroup, "show_modebuttons_checkbox");
  showModeButtons->setChecked(preferences->getShowModeButtons());
  
  showQuickButtons=new QCheckBox(i18n("Show quick &buttons"), layoutGroup, "show_quickbuttons_checkbox");
  showQuickButtons->setChecked(preferences->getShowQuickButtons());

  autoUserhostCheck=new QCheckBox(i18n("Show h&ostmasks in nickname list"), layoutGroup, "auto_userhost_check");
  autoUserhostCheck->setChecked(preferences->getAutoUserhost());
  
  row = 0;
  chatLayout->addMultiCellWidget(fontGBox, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(timestampBox, row, row, 0, 2);
  row++;
  chatLayout->addMultiCellWidget(layoutGroup, row, row, 0, 2);
  row++;
  chatLayout->setRowStretch(row, 10);

  QWidget* colorTab = new QWidget(tabWidget, "colorAppearance");
  tabWidget->addTab(colorTab, i18n("&Colors"));
  QGridLayout* colorLayout = new QGridLayout(colorTab, 4, 4, marginHint(), spacingHint());
  
  colorList.append(i18n("&Action:")+",ActionMessage");
  colorList.append(i18n("Bac&klog:")+",BacklogMessage");
  colorList.append(i18n("&Channel message:")+",ChannelMessage");
  colorList.append(i18n("C&ommand message:")+",CommandMessage");
  colorList.append(i18n("&Hyperlink:")+",LinkMessage");
  colorList.append(i18n("&Query message:")+",QueryMessage");
  colorList.append(i18n("&Server message:")+",ServerMessage");
  colorList.append(i18n("&Timestamp:")+",Time");
  colorList.append(i18n("&Background:")+",TextViewBackground");
  colorList.append(i18n("A&lternate background:")+",AlternateBackground");

  row = 0;
  int col = 0;
  QString label;
  QString name;
  
  for(unsigned int index = 0; index < colorList.count(); index++) {
    label = colorList[index].section(',',0,0);
    name = colorList[index].section(',',1);

    QLabel* colorLabel = new QLabel(label,colorTab);

    KColorButton* colorBtn = new KColorButton(colorTab);
    colorComboList.append(colorBtn);

    colorLabel->setBuddy(colorBtn);

    QString color = preferences->getColor(name);
    colorBtn->setColor(color.prepend('#'));
    // give this color button a name so we can save colors with their appropriate name later
    colorBtn->setName(name.latin1());

    colorLayout->addWidget(colorLabel, row, col);
    col++;
    colorLayout->addWidget(colorBtn, row, col);
    col++;

    if(col > 3) {
      row++;
      col = 0;
    }
  }

  colorInputFieldsCheck = new QCheckBox(
    i18n("&Input fields and nick list use custom colors"), colorTab, "input_fields_color_check");
  colorInputFieldsCheck->setChecked(preferences->getColorInputFields());
  colorLayout->addMultiCellWidget(colorInputFieldsCheck, row, row, 0, 3);
    
  row++;
  QLabel* backgroundLabel = new QLabel(i18n("Back&ground image:"), colorTab);
  backgroundURL = new KURLRequester(colorTab);
  backgroundURL->setCaption(i18n("Select Background Image"));

  backgroundLabel->setBuddy(backgroundURL);

  backgroundURL->setURL(preferences->getBackgroundImageName());

  QHBoxLayout* backgroundLayout = new QHBoxLayout(spacingHint());
  backgroundLayout->addWidget(backgroundLabel);
  backgroundLayout->addWidget(backgroundURL, 10);
  
  colorLayout->addMultiCellLayout(backgroundLayout, row, row, 0, 3);
  
  row++;
  QGroupBox* ircColorGroup = new QGroupBox(i18n("IRC Colors"), colorTab);
  ircColorGroup->setColumnLayout(0, Qt::Vertical);
  ircColorGroup->setMargin(marginHint());
  QGridLayout* ircColorLayout = new QGridLayout(ircColorGroup->layout(), 2, 4, spacingHint());
  
  int r = 0;
  parseIrcColorsCheck = new QCheckBox(i18n("Parse color codes"), ircColorGroup);
  parseIrcColorsCheck->setChecked(!preferences->getFilterColors());
  
  ircColorLayout->addMultiCellWidget(parseIrcColorsCheck, r, r, 0, 3);
  
  QStringList colors = preferences->getIRCColorList();
  col = 0;
  r = 1;
  
  for(int i = 0; i < 16; i++) {
    QLabel* label = new QLabel(QString::number(i) + ":", ircColorGroup);
    KColorButton* button = new KColorButton(ircColorGroup);
    ircColorBtnList.append(button);
    button->setColor(colors[i]);
    
    ircColorLayout->addWidget(label, r, col);
    ircColorLayout->addWidget(button, r, col + 1);
    r++;
    
    if(r > 4) {
      r = 1;
      col += 2;
    }
  }
  
  colorLayout->addMultiCellWidget(ircColorGroup, row, row, 0, 3);

  row++;
  QHBox* spacer=new QHBox(colorTab);
  colorLayout->addWidget(spacer, row, 0);
  colorLayout->setRowStretch(row, 10);
  colorLayout->setColStretch(0, 10);
  colorLayout->setColStretch(2, 10);

  layout->addWidget(tabWidget, 0);
  updateFonts();
}

PrefsPageAppearance::~PrefsPageAppearance()
{
}

void PrefsPageAppearance::textFontClicked()
{
  KFontDialog::getFont(textFont);
  updateFonts();
}

void PrefsPageAppearance::listFontClicked()
{
  KFontDialog::getFont(listFont);
  updateFonts();
}

void PrefsPageAppearance::updateFonts()
{
  textPreviewLabel->setFont(textFont);
  listPreviewLabel->setFont(listFont);

  textPreviewLabel->setText(QString("%1 %2").arg(textFont.family().section(':',0,0)).arg(textFont.pointSize()));
  listPreviewLabel->setText(QString("%1 %2").arg(listFont.family().section(':',0,0)).arg(listFont.pointSize()));
}

void PrefsPageAppearance::timestampingChanged(int state)
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

void PrefsPageAppearance::applyPreferences()
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

  for(unsigned int index = 0; index < colorComboList.count(); index++) {
    KColorButton* button = colorComboList.at(index);
    preferences->setColor(button->name(), button->color().name().mid(1));
  }

  preferences->setColorInputFields(colorInputFieldsCheck->isChecked());
  preferences->setBackgroundImageName(backgroundURL->url());
  
  QStringList colorList;
  
  for(unsigned int i = 0; i < ircColorBtnList.count(); i++) {
    KColorButton* button = ircColorBtnList.at(i);
    colorList.append(button->color().name());
  }
  
  preferences->setIRCColorList(colorList);
  preferences->setFilterColors(!parseIrcColorsCheck->isChecked());
}

#include "prefspageappearance.moc"
