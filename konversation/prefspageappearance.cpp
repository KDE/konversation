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
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qpushbutton.h>
#include <qhbox.h>

#include <kfontdialog.h>
#include <kdebug.h>

#include "prefspageappearance.h"

PrefsPageAppearance::PrefsPageAppearance(QFrame* newParent,Preferences* newPreferences) :
                     PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the appearance pane
  QGridLayout* appearanceLayout=new QGridLayout(parentFrame,3,3,marginHint(),spacingHint());

  // Font settings
  QLabel* textFontLabel=new QLabel(i18n("Text Font:"),parentFrame);
  QLabel* listFontLabel=new QLabel(i18n("Nickname List Font:"),parentFrame);

  textPreviewLabel=new QLabel(parentFrame);
  listPreviewLabel=new QLabel(parentFrame);

  textPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  listPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

  QPushButton* textFontButton=new QPushButton(i18n("Choose..."),parentFrame,"text_font_button");
  QPushButton* listFontButton=new QPushButton(i18n("Choose..."),parentFrame,"list_font_button");

  updateFonts();

  // Timestamp settings
  QHBox* timestampBox=new QHBox(parentFrame);
  timestampBox->setSpacing(spacingHint());

  doTimestamping=new QCheckBox(i18n("Show Timestamps"),timestampBox,"show_timestamps_checkbox");

  formatLabel=new QLabel(i18n("Format:"),timestampBox);
  formatLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  timestampFormat=new QComboBox(false,timestampBox,"timestamp_format_combo");
  timestampFormat->insertItem("hh");
  timestampFormat->insertItem("hh:mm");
  timestampFormat->insertItem("hh:mm:ss");

  // find actual timestamp format
  for(int index=0;index<timestampFormat->count();index++)
    if(timestampFormat->text(index)==preferences->getTimestampFormat()) timestampFormat->setCurrentItem(index);

  // Take care of ghosting / unghosting format widget
  timestampingChanged(preferences->getTimestamping() ? 2 : 0);

  showQuickButtons=new QCheckBox(i18n("Show Quick Buttons"),parentFrame,"show_quickbuttons_checkbox");
  showQuickButtons->setChecked(preferences->getShowQuickButtons());

  // Layout
  int row=0;
  appearanceLayout->addWidget(textFontLabel,row,0);
  appearanceLayout->addWidget(textPreviewLabel,row,1);
  appearanceLayout->addWidget(textFontButton,row,2);
  row++;
  appearanceLayout->addWidget(listFontLabel,row,0);
  appearanceLayout->addWidget(listPreviewLabel,row,1);
  appearanceLayout->addWidget(listFontButton,row,2);
  row++;
  appearanceLayout->addMultiCellWidget(timestampBox,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(showQuickButtons,row,row,0,2);
  row++;
  appearanceLayout->setRowStretch(row,10);
  appearanceLayout->setColStretch(1,10);

  // Set up signals / slots for appearance page

  connect(textFontButton,SIGNAL (clicked()),this,SLOT (textFontClicked()) );
  connect(listFontButton,SIGNAL (clicked()),this,SLOT (listFontClicked()) );
  connect(doTimestamping,SIGNAL (stateChanged(int)),this,SLOT (timestampingChanged(int)) );
  connect(showQuickButtons,SIGNAL (stateChanged(int)),this,SLOT (showQuickButtonsChanged(int)) );
  connect(timestampFormat,SIGNAL(activated(const QString&)),this,SLOT(formatChanged(const QString&)));
}

PrefsPageAppearance::~PrefsPageAppearance()
{
}

void PrefsPageAppearance::textFontClicked()
{
  QFont newFont(preferences->getTextFont());
  KFontDialog::getFont(newFont);
  preferences->setTextFont(newFont);
  updateFonts();
}

void PrefsPageAppearance::listFontClicked()
{
  QFont newFont(preferences->getListFont());
  KFontDialog::getFont(newFont);
  preferences->setListFont(newFont);
  updateFonts();
}

void PrefsPageAppearance::updateFonts()
{
  QFont textFont=preferences->getTextFont();
  QFont listFont=preferences->getListFont();

  textPreviewLabel->setText(QString("%1 %2").arg(textFont.family().section(':',0,0)).arg(textFont.pointSize()));
  listPreviewLabel->setText(QString("%1 %2").arg(listFont.family().section(':',0,0)).arg(listFont.pointSize()));

  textPreviewLabel->setFont(textFont);
  listPreviewLabel->setFont(listFont);
}

void PrefsPageAppearance::timestampingChanged(int state)
{
  preferences->setTimestamping(state==2);
  doTimestamping->setChecked(state==2);
  timestampFormat->setEnabled(state==2);
  formatLabel->setEnabled(state==2);
}

void PrefsPageAppearance::formatChanged(const QString& newFormat)
{
  kdDebug() << newFormat << endl;
  preferences->setTimestampFormat(newFormat);
}

void PrefsPageAppearance::showQuickButtonsChanged(int state)
{
  preferences->setShowQuickButtons(state==2);
}
