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

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qtextcodec.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qvbox.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>

#include <kfontdialog.h>
#include <kdebug.h>
#include <kcharsets.h>
#include <klistview.h>
#include <klocale.h>

#include "prefspageappearance.h"
#include "preferences.h"
#include "valuelistviewitem.h"

PrefsPageAppearance::PrefsPageAppearance(QFrame* newParent,Preferences* newPreferences) :
                     PrefsPage(newParent,newPreferences)
{
  // Add a Layout to the appearance pane
  QGridLayout* appearanceLayout=new QGridLayout(parentFrame,3,3,marginHint(),spacingHint());

  // Font settings
  QLabel* textFontLabel=new QLabel(i18n("Text font:"),parentFrame);
  QLabel* listFontLabel=new QLabel(i18n("Nickname list font:"),parentFrame);

  textFont=preferences->getTextFont();
  listFont=preferences->getListFont();

  textPreviewLabel=new QLabel(parentFrame);
  listPreviewLabel=new QLabel(parentFrame);

  textPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  listPreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

  QPushButton* textFontButton=new QPushButton(i18n("Choose..."),parentFrame,"text_font_button");
  QPushButton* listFontButton=new QPushButton(i18n("Choose..."),parentFrame,"list_font_button");

  QLabel* codecLabel=new QLabel(i18n("Encoding:"),parentFrame);
  codecList=new QComboBox(parentFrame);

  QStringList encodings=KGlobal::charsets()->descriptiveEncodingNames();

  // from ksirc: remove utf16/ucs2 as it just doesn't work for IRC
  QStringList::Iterator iterator=encodings.begin();
  while(iterator!=encodings.end())
  {
    if((*iterator).find("utf16")!=-1 ||
       (*iterator).find("iso-10646")!=-1)
      iterator=encodings.remove(iterator);
    else
      ++iterator;
  }

  codecList->insertStringList(encodings);

  // find actual encoding and set combo box accordingly
  encoding="( "+preferences->getCodec().lower()+" )";
  for(unsigned int index=0;index<encodings.count();index++)
  {
    if(encodings[index].lower().find(encoding)!=-1)
    {
      codecList->setCurrentItem(index);
      break;
    }
  }

  updateFonts();

  // Timestamp settings
  QHBox* timestampBox=new QHBox(parentFrame);
  timestampBox->setSpacing(spacingHint());

  doTimestamping=new QCheckBox(i18n("Show timestamps"),timestampBox,"show_timestamps_checkbox");

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

  QHBox* showButtonsBox=new QHBox(parentFrame);

  showQuickButtons=new QCheckBox(i18n("Show quick buttons"),showButtonsBox,"show_quickbuttons_checkbox");
  showQuickButtons->setChecked(preferences->getShowQuickButtons());

  showModeButtons=new QCheckBox(i18n("Show channel mode buttons"),showButtonsBox,"show_modebuttons_checkbox");
  showModeButtons->setChecked(preferences->getShowModeButtons());

  autoUserhostCheck=new QCheckBox(i18n("Show hostmasks in nick list"),parentFrame,"auto_userhost_check");
  autoUserhostCheck->setChecked(preferences->getAutoUserhost());

  useSpacingCheck=new QCheckBox(i18n("Use custom widget spacing"),parentFrame,"use_spacing_check");

  QHBox* spacingMarginBox=new QHBox(parentFrame);
  spacingMarginBox->setSpacing(spacingHint());

  spacingLabel=new QLabel(i18n("Spacing:"),spacingMarginBox);
  spacingSpin=new QSpinBox(0,10,1,spacingMarginBox,"spacing_spin_box");
  marginLabel=new QLabel(i18n("Margin:"),spacingMarginBox);
  marginSpin=new QSpinBox(0,10,1,spacingMarginBox,"margin_spin_box");

  spacingSpin->setValue(preferences->getSpacing());
  spacingSpin->setSuffix(" "+i18n("Pixel"));
  marginSpin->setValue(preferences->getMargin());
  marginSpin->setSuffix(" "+i18n("Pixel"));

  marginLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  spacingMarginBox->setStretchFactor(marginLabel,10);

  // Take care of ghosting / unghosting spacing widgets
  useSpacingChanged(preferences->getUseSpacing() ? 2 : 0);

  // paragraph spacing stuff
  QHBox* paragraphSpacingBox=new QHBox(parentFrame);
  paragraphSpacingBox->setSpacing(spacingHint());

  useParagraphSpacingCheck=new QCheckBox(i18n("Use paragraph spacing:"),paragraphSpacingBox,"use_paragraph_spacing_check");

  paragraphSpacingSpin=new QSpinBox(0,10,1,paragraphSpacingBox,"paragraph_spacing_spin_box");

  paragraphSpacingSpin->setValue(preferences->getParagraphSpacing());
  paragraphSpacingSpin->setSuffix(" "+i18n("Pixel"));

  paragraphSpacingBox->setStretchFactor(paragraphSpacingSpin,10);

  // Take care of ghosting / unghosting paragraph spacing widgets
  useParagraphSpacingChanged(preferences->getUseParagraphSpacing() ? 2 : 0);

  // close buttons on tabs
  closeButtonsCheck=new QCheckBox(i18n("Show close widgets on tabs"),parentFrame,"tab_close_widgets_check");
  closeButtonsCheck->setChecked(preferences->getCloseButtonsOnTabs());

  // Sorting
  QVGroupBox* sortOptionsGroup=new QVGroupBox(i18n("Sort options"),parentFrame,"sort_options_group");
  sortOrderGroup=new QHGroupBox(i18n("Sorting order"),parentFrame,"sort_order_group");

  sortByStatusCheck=new QCheckBox(i18n("Sort by user status"),sortOptionsGroup,"sort_by_status_check");
  sortCaseInsensitiveCheck=new QCheckBox(i18n("Sort case insensitive"),sortOptionsGroup,"sort_case_insensitive_check");

  sortByStatusCheck->setChecked(preferences->getSortByStatus());
  sortOrderGroup->setEnabled(preferences->getSortByStatus());

  sortCaseInsensitiveCheck->setChecked(preferences->getSortCaseInsensitive());

  sortingOrder=new KListView(sortOrderGroup,"sorting_order_view");
  sortingOrder->addColumn("");
  sortingOrder->setFullWidth(true);
  sortingOrder->header()->hide();
  sortingOrder->setSorting(-1);
  sortingOrder->setDragEnabled(true);
  sortingOrder->setAcceptDrops(true);
  sortingOrder->setMaximumHeight(sortingOrder->fontMetrics().height()*4);

  for(int index=4;index!=0;index>>=1)
  {
    if(preferences->getNoRightsValue()==index) new ValueListViewItem(0,sortingOrder,i18n("Normal users"));
    if(preferences->getVoiceValue()==index)    new ValueListViewItem(1,sortingOrder,i18n("Voice (+v)"));
    if(preferences->getOpValue()==index)       new ValueListViewItem(2,sortingOrder,i18n("Operators (+o)"));
  }

  QVBox* sortOrderUpDownBox=new QVBox(sortOrderGroup);

  QToolButton* sortMoveUp=new QToolButton(Qt::UpArrow,sortOrderUpDownBox,"sort_move_up_button");
  QToolButton* sortMoveDown=new QToolButton(Qt::DownArrow,sortOrderUpDownBox,"sort_move_up_button");
  sortMoveUp->setFixedWidth(16);
  sortMoveDown->setFixedWidth(16);

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
  appearanceLayout->addWidget(codecLabel,row,0);
  appearanceLayout->addMultiCellWidget(codecList,row,row,1,2);
  row++;
  appearanceLayout->addMultiCellWidget(timestampBox,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(showButtonsBox,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(autoUserhostCheck,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(useSpacingCheck,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(spacingMarginBox,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(paragraphSpacingBox,row,row,0,2);
  row++;
  appearanceLayout->addMultiCellWidget(closeButtonsCheck,row,row,0,2);
  row++;
  appearanceLayout->addWidget(sortOptionsGroup,row,0);
  appearanceLayout->addMultiCellWidget(sortOrderGroup,row,row,1,2);
  row++;
  appearanceLayout->addMultiCellWidget(new QHBox(parentFrame),row,row,0,2);
  appearanceLayout->setRowStretch(row,10);
  appearanceLayout->setColStretch(1,10);

  // Set up signals / slots for appearance page

  connect(textFontButton,SIGNAL (clicked()),this,SLOT (textFontClicked()) );
  connect(listFontButton,SIGNAL (clicked()),this,SLOT (listFontClicked()) );

  connect(codecList,SIGNAL (activated(int)),this,SLOT (encodingChanged(int)));

  connect(doTimestamping,SIGNAL (stateChanged(int)),this,SLOT (timestampingChanged(int)) );

  connect(useSpacingCheck,SIGNAL (stateChanged(int)),this,SLOT (useSpacingChanged(int)) );

  connect(useParagraphSpacingCheck,SIGNAL (stateChanged(int)),this,SLOT (useParagraphSpacingChanged(int)) );

  connect(sortByStatusCheck,SIGNAL (stateChanged(int)),this,SLOT (sortByStatusChanged(int)) );

  connect(sortMoveUp,SIGNAL (clicked()),this,SLOT (moveUp()) );
  connect(sortMoveDown,SIGNAL (clicked()),this,SLOT (moveDown()) );
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
  doTimestamping->setChecked(state==2);
  timestampFormat->setEnabled(state==2);
  formatLabel->setEnabled(state==2);
}

void PrefsPageAppearance::encodingChanged(int newEncodingIndex)
{
  if(newEncodingIndex) encoding=codecList->text(newEncodingIndex);
}

void PrefsPageAppearance::useSpacingChanged(int state)
{
  useSpacingCheck->setChecked(state);
  spacingLabel->setEnabled(state==2);
  spacingSpin->setEnabled(state==2);
  marginLabel->setEnabled(state==2);
  marginSpin->setEnabled(state==2);
}

void PrefsPageAppearance::useParagraphSpacingChanged(int state)
{
  useParagraphSpacingCheck->setChecked(state);
  paragraphSpacingSpin->setEnabled(state==2);
}

void PrefsPageAppearance::sortByStatusChanged(int state)
{
  sortOrderGroup->setEnabled(state==2);
}

void PrefsPageAppearance::moveUp()
{
  QListViewItem* item=sortingOrder->selectedItem();
  if(item)
  {
    int pos=sortingOrder->itemIndex(item);
    if(pos) item->itemAbove()->moveItem(item);
  }
}

void PrefsPageAppearance::moveDown()
{
  QListViewItem* item=sortingOrder->selectedItem();
  if(item)
  {
    int pos=sortingOrder->itemIndex(item);
    if(pos!=2) item->moveItem(item->itemBelow());
  }
}

void PrefsPageAppearance::applyPreferences()
{
  preferences->setTextFont(textFont);
  preferences->setListFont(listFont);
  preferences->setTimestamping(doTimestamping->isChecked());
  preferences->setTimestampFormat(timestampFormat->currentText());
  preferences->setShowQuickButtons(showQuickButtons->isChecked());
  preferences->setShowModeButtons(showModeButtons->isChecked());
  preferences->setCloseButtonsOnTabs(closeButtonsCheck->isChecked());

  if(encoding.startsWith("utf 16"))
    preferences->setCodec(QString::null);
  else
    preferences->setCodec(KGlobal::charsets()->encodingForName(encoding));

  preferences->setAutoUserhost(autoUserhostCheck->isChecked());
  preferences->setUseSpacing(useSpacingCheck->isChecked());
  preferences->setSpacing(spacingSpin->value());
  preferences->setMargin(marginSpin->value());
  preferences->setUseParagraphSpacing(useParagraphSpacingCheck->isChecked());
  preferences->setParagraphSpacing(paragraphSpacingSpin->value());
  preferences->setSortByStatus(sortByStatusCheck->isChecked());
  preferences->setSortCaseInsensitive(sortCaseInsensitiveCheck->isChecked());

  int flag=1;

  for(int index=0;index<3;index++)
  {
    ValueListViewItem* item=static_cast<ValueListViewItem*>(sortingOrder->itemAtIndex(index));
    int value=item->getValue();

    if(value==0) preferences->setNoRightsValue(flag);
    else if(value==1) preferences->setVoiceValue(flag);
    else if(value==2) preferences->setOpValue(flag);

    flag<<=1;
  }
}

#include "prefspageappearance.moc"
