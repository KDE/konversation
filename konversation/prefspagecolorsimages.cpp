/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagecolorsimages.cpp  -  Color and image preferences
  begin:     Don Jun 5 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <klocale.h>

#include "prefspagecolorsimages.h"
#include "preferences.h"

PrefsPageColorsImages::PrefsPageColorsImages(QFrame* newParent,Preferences* newPreferences) :
                       PrefsPage(newParent,newPreferences)
{
  colorList.append(i18n("Action text color:")+",ActionMessage");
  colorList.append(i18n("Backlog text color:")+",BacklogMessage");
  colorList.append(i18n("Channel message text color:")+",ChannelMessage");
  colorList.append(i18n("Command message text color:")+",CommandMessage");
  colorList.append(i18n("Hyperlink text color:")+",LinkMessage");
  colorList.append(i18n("Query message text color:")+",QueryMessage");
  colorList.append(i18n("Server message text color:")+",ServerMessage");
  colorList.append(i18n("Timestamp color:")+",Time");
  colorList.append(i18n("Background color:")+",TextViewBackground");
  colorList.append(i18n("Alternate background color:")+",AlternateBackground");

  QGridLayout* colorSettingsLayout=new QGridLayout(parentFrame,4,3,marginHint(),spacingHint(),"log_settings_layout");

  int row=0;
  for(unsigned int index=0;index<colorList.count();index++)
  {
    QString label(colorList[index].section(',',0,0));
    QString name(colorList[index].section(',',1));

    QLabel* colorLabel=new QLabel(label,parentFrame);

    KColorCombo* colorCombo=new KColorCombo(parentFrame);
    colorComboList.append(colorCombo);

    QString color=preferences->getColor(name);
    colorCombo->setColor(color.prepend('#'));
    // give this color combo a name so we can save colors with their appropriate name later
    colorCombo->setName(name.latin1());

    colorSettingsLayout->addWidget(colorLabel,row,0);
    colorSettingsLayout->addMultiCellWidget(colorCombo,row,row,1,2);

    row++;
  }

  QLabel* backgroundLabel=new QLabel(i18n("Background image:"),parentFrame);
  backgroundName=new KLineEdit(parentFrame,"background_image_name");
  QPushButton* backgroundSelect=new QPushButton(i18n("Choose..."),parentFrame,"background_image_choose_button");

  backgroundName->setText(preferences->getBackgroundImageName());

  colorSettingsLayout->addWidget(backgroundLabel,row,0);
  colorSettingsLayout->addWidget(backgroundName,row,1);
  colorSettingsLayout->addWidget(backgroundSelect,row,2);

  row++;

  QHBox* spacer=new QHBox(parentFrame);
  colorSettingsLayout->addWidget(spacer,row,0);
  colorSettingsLayout->setRowStretch(row,10);

  connect(backgroundSelect,SIGNAL (clicked()),this,SLOT (selectBackground()) );
}

PrefsPageColorsImages::~PrefsPageColorsImages()
{
}

void PrefsPageColorsImages::selectBackground()
{
  QString fileName=KFileDialog::getOpenFileName(
                                                 backgroundName->text(),
                                                 QString::null,
                                                 parentFrame,
                                                 i18n("Select Background Image")
                                               );

  if(!fileName.isEmpty()) backgroundName->setText(fileName);
}

void PrefsPageColorsImages::applyPreferences()
{
  for(unsigned int index=0;index<colorComboList.count();index++)
  {
    KColorCombo* combo=colorComboList.at(index);
    preferences->setColor(combo->name(),combo->color().name().mid(1));
  }

  preferences->setBackgroundImageName(backgroundName->text());
}

#include "prefspagecolorsimages.moc"
