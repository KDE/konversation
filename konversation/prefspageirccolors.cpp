/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageirccolors.cpp  -  Provides a user interface to customize IRC colors
  begin:     Wed 8 July 2003
  copyright: (C) 2003 Peter Simonsson
  email:     psn@linux.se
*/

#include "prefspageirccolors.h"

#include <qlayout.h>
#include <qframe.h>
#include <qstringlist.h>

#include <kcolorbutton.h>
#include <kdebug.h>

#include "prefspageirccolorsui.h"
#include "preferences.h"

PrefsPageIRCColors::PrefsPageIRCColors(QFrame* newParent, Preferences* newPreferences)
  : PrefsPage(newParent, newPreferences)
{
  m_widget = new PrefsPageIRCColorsUI(parentFrame);
  QGridLayout* gl = new QGridLayout(parentFrame);
  gl->addWidget(m_widget, 0, 0);

  QStringList colors = preferences->getIRCColorList();
  m_widget->m_whiteCBtn->setColor(QColor(colors[0]));
  m_widget->m_blackCBtn->setColor(QColor(colors[1]));
  m_widget->m_darkBlueCBtn->setColor(QColor(colors[2]));
  m_widget->m_darkGreenCBtn->setColor(QColor(colors[3]));
  m_widget->m_redCBtn->setColor(QColor(colors[4]));
  m_widget->m_brownCBtn->setColor(QColor(colors[5]));
  m_widget->m_magentaCBtn->setColor(QColor(colors[6]));
  m_widget->m_orangeCBtn->setColor(QColor(colors[7]));
  m_widget->m_yellowCBtn->setColor(QColor(colors[8]));
  m_widget->m_greenCBtn->setColor(QColor(colors[9]));
  m_widget->m_darkCyanCBtn->setColor(QColor(colors[10]));
  m_widget->m_cyanCBtn->setColor(QColor(colors[11]));
  m_widget->m_blueCBtn->setColor(QColor(colors[12]));
  m_widget->m_purpleCBtn->setColor(QColor(colors[13]));
  m_widget->m_grayCBtn->setColor(QColor(colors[14]));
  m_widget->m_lightGrayCBtn->setColor(QColor(colors[15]));
}

void PrefsPageIRCColors::applyPreferences()
{
  QStringList colors;
  colors.append(m_widget->m_whiteCBtn->color().name());
  colors.append(m_widget->m_blackCBtn->color().name());
  colors.append(m_widget->m_darkBlueCBtn->color().name());
  colors.append(m_widget->m_darkGreenCBtn->color().name());
  colors.append(m_widget->m_redCBtn->color().name());
  colors.append(m_widget->m_brownCBtn->color().name());
  colors.append(m_widget->m_magentaCBtn->color().name());
  colors.append(m_widget->m_orangeCBtn->color().name());
  colors.append(m_widget->m_yellowCBtn->color().name());
  colors.append(m_widget->m_greenCBtn->color().name());
  colors.append(m_widget->m_darkCyanCBtn->color().name());
  colors.append(m_widget->m_cyanCBtn->color().name());
  colors.append(m_widget->m_blueCBtn->color().name());
  colors.append(m_widget->m_purpleCBtn->color().name());
  colors.append(m_widget->m_grayCBtn->color().name());
  colors.append(m_widget->m_lightGrayCBtn->color().name());
  preferences->setIRCColorList(colors);
}

#include "prefspageirccolors.moc"
