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

  $Id$
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>

#include "prefspagecolorsimages.h"

PrefsPageColorsImages::PrefsPageColorsImages(QFrame* newParent,Preferences* newPreferences) :
                       PrefsPage(newParent,newPreferences)
{
  colorList.append(i18n("Action text color")+",ActionMessage");
  colorList.append(i18n("Backlog text color")+",BacklogMessage");
  colorList.append(i18n("Channel message text color")+",ChannelMessage");
  colorList.append(i18n("Command message text color")+",CommandMessage");
  colorList.append(i18n("Hyperlink text color")+",LinkMessage");
  colorList.append(i18n("Query message text color")+",QueryMessage");
  colorList.append(i18n("Server message text color")+",ServerMessage");
  colorList.append(i18n("Timestamp color")+",Time");
  colorList.append(i18n("Background color")+",TextViewBackground");

  QGridLayout* colorSettingsLayout=new QGridLayout(parentFrame,4,2,marginHint(),spacingHint(),"log_settings_layout");

  int row=0;
  for(unsigned int index=0;index<colorList.count();index++)
  {
    QString label(colorList[index].section(',',0,0));
//    QString name(colorList[index].section(',',1));

    QLabel* colorLabel=new QLabel(label,parentFrame);
    colorSettingsLayout->addWidget(colorLabel,row,0);
    row++;
  }

  QHBox* spacer=new QHBox(parentFrame);
  colorSettingsLayout->addWidget(spacer,row,0);
  colorSettingsLayout->setRowStretch(row,10);
}

PrefsPageColorsImages::~PrefsPageColorsImages()
{
}

#include "prefspagecolorsimages.moc"
