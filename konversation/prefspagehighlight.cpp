/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagehighlight.cpp  -  Preferences panel for highlight patterns
  begin:     Die Jun 10 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qhgroupbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>

#include <kcolorcombo.h>
#include <klistview.h>
#include <klineedit.h>

#include "prefspagehighlight.h"

PrefsPageHighlight::PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences) :
                    PrefsPage(newParent,newPreferences)
{
  // Add the layout to the widget
  QGridLayout* highlightLayout=new QGridLayout(parentFrame,3,2,marginHint(),spacingHint());
  QHGroupBox* highlightListGroup=new QHGroupBox(i18n("Highlight patterns"),parentFrame,"highlight_pattern_group");

  QVBox* highlightListBox=new QVBox(highlightListGroup);
  highlightListBox->setSpacing(spacingHint());

  KListView* highlightListView=new KListView(highlightListBox,"highlight_list_view");

  highlightListView->addColumn("Highlights");
  highlightListView->setFullWidth();

  QHBox* highlightEditBox=new QHBox(highlightListBox);
  highlightEditBox->setSpacing(spacingHint());

  new QLabel(i18n("Pattern:"),highlightEditBox);
  KLineEdit* patternInput=new KLineEdit(highlightEditBox,"highlight_pattern_input");
  KColorCombo* patternColor=new KColorCombo(highlightEditBox,"highlight_pattern_color");

  QCheckBox* currentNickCheck=new QCheckBox(i18n("Always highlight current nick"),parentFrame,"highlight_current_nick_check");
  KColorCombo* currentNickColor=new KColorCombo(parentFrame,"current_nick_color");
  QCheckBox* ownLinesCheck=new QCheckBox(i18n("Always highlight own lines"),parentFrame,"highlight_own_lines_check");
  KColorCombo* ownLinesColor=new KColorCombo(parentFrame,"own_lines_color");

  QVBox* highlightButtonBox=new QVBox(highlightListGroup);
  QPushButton* newButton=new QPushButton(i18n("New"),highlightButtonBox);
  QPushButton* removeButton=new QPushButton(i18n("Remove"),highlightButtonBox);

  int row=0;
  highlightLayout->addMultiCellWidget(highlightListGroup,row,row,0,1);

  row++;
  highlightLayout->addWidget(currentNickCheck,row,0);
  highlightLayout->addWidget(currentNickColor,row,1);
  row++;
  highlightLayout->addWidget(ownLinesCheck,row,0);
  highlightLayout->addWidget(ownLinesColor,row,1);
}

PrefsPageHighlight::~PrefsPageHighlight()
{
}

#include "prefspagehighlight.moc"
