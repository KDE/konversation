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
#include "prefspagecolorsappearance.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurlrequester.h>

#include "preferences.h"

PrefsPageColorsAppearance::PrefsPageColorsAppearance(QFrame* newParent,Preferences* newPreferences)
 : PrefsPage(newParent, newPreferences)
{
  QGridLayout* colorLayout = new QGridLayout(parentFrame, 4, 4, marginHint(), spacingHint());
  
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

  int row = 0;
  int col = 0;
  QString label;
  QString name;
  
  for(unsigned int index = 0; index < colorList.count(); index++) {
    label = colorList[index].section(',',0,0);
    name = colorList[index].section(',',1);

    QLabel* colorLabel = new QLabel(label,parentFrame);

    KColorButton* colorBtn = new KColorButton(parentFrame);
    colorBtnList.append(colorBtn);

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
    i18n("&Input fields and nick list use custom colors"), parentFrame, "input_fields_color_check");
  colorInputFieldsCheck->setChecked(preferences->getColorInputFields());
  colorLayout->addMultiCellWidget(colorInputFieldsCheck, row, row, 0, 3);
    
  row++;
  QLabel* backgroundLabel = new QLabel(i18n("Back&ground image:"), parentFrame);
  backgroundURL = new KURLRequester(parentFrame);
  backgroundURL->setCaption(i18n("Select Background Image"));

  backgroundLabel->setBuddy(backgroundURL);

  backgroundURL->setURL(preferences->getBackgroundImageName());

  QHBoxLayout* backgroundLayout = new QHBoxLayout(spacingHint());
  backgroundLayout->addWidget(backgroundLabel);
  backgroundLayout->addWidget(backgroundURL, 10);
  
  colorLayout->addMultiCellLayout(backgroundLayout, row, row, 0, 3);
  
  row++;
  QGroupBox* ircColorGroup = new QGroupBox(i18n("IRC Colors"), parentFrame);
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
  QHBox* spacer=new QHBox(parentFrame);
  colorLayout->addWidget(spacer, row, 0);
  colorLayout->setRowStretch(row, 10);
  colorLayout->setColStretch(0, 10);
  colorLayout->setColStretch(2, 10);
}


PrefsPageColorsAppearance::~PrefsPageColorsAppearance()
{
}

void PrefsPageColorsAppearance::applyPreferences()
{
  for(unsigned int index = 0; index < colorBtnList.count(); index++) {
    KColorButton* button = colorBtnList.at(index);
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

#include "prefspagecolorsappearance.moc"
