/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagenickcompletion.cpp  -  Provides a user interface to customize nickname completion
  begin:     Sat Nov 15 2003
  copyright: (C) 2003 by Peter Simonsson, Dario Abatianni
  email:     psn@linux.se, eisfuchs@tigress.com
*/

#include "prefspagenickcompletion.h"

#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>

#include <klineedit.h>
#include <klocale.h>

#include "preferences.h"

PrefsPageNickCompletion::PrefsPageNickCompletion(QFrame* newParent, Preferences* newPreferences)
  : PrefsPage(newParent, newPreferences)
{
  QGridLayout* gl = new QGridLayout(parentFrame, 3, 2, marginHint(),
    spacingHint(), "nick_completion_layout");

  // nick completion mode
  QLabel* modeLbl = new QLabel(i18n("Completion &mode:"), parentFrame);
  completionModeCBox = new QComboBox(parentFrame,"completion_mode_combo_box");
  completionModeCBox->insertItem(i18n("Cycle Nicklist"));
  completionModeCBox->insertItem(i18n("Shell-Like"));
  completionModeCBox->insertItem(i18n("Shell-Like with Completion Box"));
  completionModeCBox->setCurrentItem(preferences->getNickCompletionMode());
  modeLbl->setBuddy(completionModeCBox);

  // nick completion special settings
  QVBox* suffixBox=new QVBox(parentFrame);
  new QLabel(i18n("Characters to add on nick completion"),suffixBox);

  QHBox* suffixEditBox=new QHBox(suffixBox);
  suffixEditBox->setSpacing(spacingHint());
  QLabel* startOfLineLabel=new QLabel(i18n("at &start of line:"),suffixEditBox);
  suffixStartInput=new KLineEdit(preferences->getNickCompleteSuffixStart(),suffixEditBox);
  startOfLineLabel->setBuddy(suffixStartInput);

  QLabel* middleLabel=new QLabel(i18n("&Elsewhere:"),suffixEditBox);
  middleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  suffixMiddleInput=new KLineEdit(preferences->getNickCompleteSuffixMiddle(),suffixEditBox);
  middleLabel->setBuddy(suffixMiddleInput);

  QSpacerItem* spacer=new QSpacerItem(0,0,QSizePolicy::Minimum,QSizePolicy::Expanding);

  int row = 0;
  gl->addWidget(modeLbl, row, 0);
  gl->addWidget(completionModeCBox, row, 1);
  row++;
  gl->addMultiCellWidget(suffixBox, row, row, 0, 1);
  row++;
  gl->addItem(spacer, row, 0);
}

PrefsPageNickCompletion::~PrefsPageNickCompletion()
{
}

void PrefsPageNickCompletion::applyPreferences()
{
  preferences->setNickCompletionMode(completionModeCBox->currentItem());
  preferences->setNickCompleteSuffixStart(suffixStartInput->text());
  preferences->setNickCompleteSuffixMiddle(suffixMiddleInput->text());
}

#include "prefspagenickcompletion.moc"
