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
#include <kdebug.h>
#include <klocale.h>

#include "prefspagehighlight.h"
#include "preferences.h"
#include "highlight.h"
#include "highlightviewitem.h"

PrefsPageHighlight::PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences) :
                    PrefsPage(newParent,newPreferences)
{
  // Add the layout to the widget
  QGridLayout* highlightLayout=new QGridLayout(parentFrame,3,2,marginHint(),spacingHint());
  QHGroupBox* highlightListGroup=new QHGroupBox(i18n("Highlight List"),parentFrame,"highlight_pattern_group");

  QVBox* highlightListBox=new QVBox(highlightListGroup);
  highlightListBox->setSpacing(spacingHint());

  highlightListView=new KListView(highlightListBox,"highlight_list_view");

  highlightListView->addColumn(i18n("Highlights"));
  highlightListView->setFullWidth();
  highlightListView->setDragEnabled(true);
  highlightListView->setAcceptDrops(true);
  highlightListView->setSorting(-1);

  QPtrList<Highlight> highlightList=preferences->getHilightList();
  // fill in the highlight patterns backwards to keep the right sorting order
  for(unsigned int i=highlightList.count();i!=0;i--)
  {
    Highlight* currentHighlight=highlightList.at(i-1);
    new HighlightViewItem(highlightListView,currentHighlight);
  }

  QHBox* highlightEditBox=new QHBox(highlightListBox);
  highlightEditBox->setSpacing(spacingHint());

  patternLabel=new QLabel(i18n("&Pattern:"),highlightEditBox);
  patternInput=new KLineEdit(highlightEditBox,"highlight_pattern_input");
  patternColor=new KColorCombo(highlightEditBox,"highlight_pattern_color");
  patternLabel->setBuddy(patternInput);

  patternLabel->setEnabled(false);
  patternInput->setEnabled(false);
  patternColor->setEnabled(false);

  currentNickCheck=new QCheckBox(i18n("Always highlight &current nick:"),parentFrame,"highlight_current_nick_check");
  currentNickCheck->setChecked(preferences->getHilightNick());
  currentNickColor=new KColorCombo(parentFrame,"current_nick_color");
  currentNickColor->setColor(preferences->getHilightNickColor());
  currentNickChanged(preferences->getHilightNick() ? 2 : 0);

  ownLinesCheck=new QCheckBox(i18n("Always highlight &own lines:"),parentFrame,"highlight_own_lines_check");
  ownLinesColor=new KColorCombo(parentFrame,"own_lines_color");
  ownLinesCheck->setChecked(preferences->getHilightOwnLines());
  ownLinesColor->setColor(preferences->getHilightOwnLinesColor());
  ownLinesChanged(preferences->getHilightOwnLines() ? 2 : 0);

  QVBox* highlightButtonBox=new QVBox(highlightListGroup);
  highlightButtonBox->setSpacing(spacingHint());
  QPushButton* newButton=new QPushButton(i18n("&New"),highlightButtonBox);
  QPushButton* removeButton=new QPushButton(i18n("&Remove"),highlightButtonBox);
  // add spacer below the two buttons
  new QVBox(highlightButtonBox);

  int row=0;
  highlightLayout->addMultiCellWidget(highlightListGroup,row,row,0,1);

  row++;
  highlightLayout->addWidget(currentNickCheck,row,0);
  highlightLayout->addWidget(currentNickColor,row,1);

  row++;
  highlightLayout->addWidget(ownLinesCheck,row,0);
  highlightLayout->addWidget(ownLinesColor,row,1);

  connect(highlightListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (highlightSelected(QListViewItem*)) );
  connect(highlightListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (highlightSelected(QListViewItem*)) );

  connect(patternInput,SIGNAL (textChanged(const QString&)),this,SLOT (highlightTextChanged(const QString&)) );
  connect(patternColor,SIGNAL (activated(const QColor&)),this,SLOT (highlightColorChanged(const QColor&)) );

  connect(newButton,SIGNAL (clicked()),this,SLOT (addHighlight()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeHighlight()) );

  connect(currentNickCheck,SIGNAL(stateChanged(int)),this,SLOT(currentNickChanged(int)));
  connect(ownLinesCheck,SIGNAL(stateChanged(int)),this,SLOT(ownLinesChanged(int)));
}

PrefsPageHighlight::~PrefsPageHighlight()
{
}

void PrefsPageHighlight::highlightSelected(QListViewItem* item)
{
  if(item)
  {
    HighlightViewItem* highlightItem=static_cast<HighlightViewItem*>(item);

    patternLabel->setEnabled(true);
    patternInput->setEnabled(true);
    patternColor->setEnabled(true);

    patternColor->setColor(highlightItem->getColor());
    patternInput->setText(highlightItem->getText());
  }
  else
  {
    patternLabel->setEnabled(false);
    patternInput->setEnabled(false);
    patternColor->setEnabled(false);
  }
}

void PrefsPageHighlight::highlightTextChanged(const QString& newPattern)
{
  QListViewItem* item=highlightListView->selectedItem();

  if(item) item->setText(0,newPattern);
}

void PrefsPageHighlight::highlightColorChanged(const QColor& newColor)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(item)
  {
    item->setColor(newColor);
    item->repaint();
  }
}

void PrefsPageHighlight::addHighlight()
{
  Highlight* newHighlight=new Highlight(i18n("New"),QColor("#ff0000"));

  HighlightViewItem* item=new HighlightViewItem(highlightListView,newHighlight);
  highlightListView->setSelected(item,true);
}

void PrefsPageHighlight::removeHighlight()
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(item)
  {
    delete item;

    item=static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if(item)
      highlightListView->setSelected(item,true);
    else
    {
      patternLabel->setEnabled(false);
      patternInput->setEnabled(false);
      patternColor->setEnabled(false);
    }
  }
}

QPtrList<Highlight> PrefsPageHighlight::getHighlightList()
{
  QPtrList<Highlight> newList;

  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->firstChild());
  while(item)
  {
    newList.append(new Highlight(item->getText(),item->getColor()));
    item=item->itemBelow();
  }

  return newList;
}

void PrefsPageHighlight::currentNickChanged(int state)
{
  currentNickColor->setEnabled(state==2);
}

void PrefsPageHighlight::ownLinesChanged(int state)
{
  ownLinesColor->setEnabled(state==2);
}

void PrefsPageHighlight::applyPreferences()
{
  preferences->setHilightList(getHighlightList());
  preferences->setHilightNick(currentNickCheck->isChecked());
  preferences->setHilightOwnLines(ownLinesCheck->isChecked());
  preferences->setHilightNickColor(currentNickColor->color().name());
  preferences->setHilightOwnLinesColor(ownLinesColor->color().name());
}

#include "prefspagehighlight.moc"
