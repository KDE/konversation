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
#include <kurlrequester.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kiconloader.h>

#include "prefspagehighlight.h"
#include "preferences.h"
#include "highlight.h"
#include "highlightviewitem.h"
#include "konversationapplication.h"
#include "konversationsound.h"

PrefsPageHighlight::PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences) :
                    PrefsPage(newParent,newPreferences)
{
  // Add the layout to the widget
  QGridLayout* highlightLayout=new QGridLayout(parentFrame,3,2,marginHint(),spacingHint());
  QHGroupBox* highlightListGroup=new QHGroupBox(i18n("&Highlight List"),parentFrame,"highlight_pattern_group");

  QVBox* highlightListBox=new QVBox(highlightListGroup);
  highlightListBox->setSpacing(spacingHint());

  highlightListView=new KListView(highlightListBox,"highlight_list_view");

  highlightListView->addColumn(i18n("RE"));
  highlightListView->addColumn(i18n("Highlights"));
  highlightListView->addColumn(i18n("Sound"));
  highlightListView->addColumn(i18n("Auto Text"));
  highlightListView->setAllColumnsShowFocus(true);
  highlightListView->setFullWidth(true);
  highlightListView->setDragEnabled(true);
  highlightListView->setAcceptDrops(true);
  highlightListView->setSorting(-1);

  QPtrList<Highlight> highlightList=preferences->getHighlightList();
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

  QHBox* highlightSoundBox=new QHBox(highlightListBox);
  highlightSoundBox->setSpacing(spacingHint());

  soundLabel = new QLabel(i18n("&Sound:"), highlightSoundBox);
  soundPlayBtn = new QPushButton(highlightSoundBox, "highlight_sound_play_button");
  soundPlayBtn->setPixmap(SmallIcon( "player_play" ));
  soundURL = new KURLRequester(highlightSoundBox, "highlight_sound_url");
  soundLabel->setBuddy(soundURL);

  QHBox* autoTextBox=new QHBox(highlightListBox);
  autoTextBox->setSpacing(spacingHint());
  autoTextLabel=new QLabel(i18n("Auto &text:"),autoTextBox);
  autoTextInput=new KLineEdit(autoTextBox,"auto_text_input");
  autoTextLabel->setBuddy(autoTextInput);

  patternLabel->setEnabled(false);
  patternInput->setEnabled(false);
  patternColor->setEnabled(false);
  soundURL->setEnabled(false);
  soundLabel->setEnabled(false);
  soundPlayBtn->setEnabled(false);
  autoTextInput->setEnabled(false);
  autoTextLabel->setEnabled(false);

  QString filter = "audio/x-wav audio/x-mp3 application/ogg audio/x-adpcm";
  soundURL->setFilter(filter);
  soundURL->setCaption(i18n("Select Sound File"));

  // This code was copied from KNotifyWidget::openSoundDialog() (knotifydialog.cpp) [it's under LGPL v2]
  // find the first "sound"-resource that contains files
  QStringList soundDirs = KGlobal::dirs()->findDirs("data", "konversation/sounds");
  soundDirs += KGlobal::dirs()->resourceDirs( "sound" );

  if ( !soundDirs.isEmpty() ) {
    KURL url;
    QDir dir;
    dir.setFilter( QDir::Files | QDir::Readable );
    QStringList::ConstIterator it = soundDirs.begin();
    while ( it != soundDirs.end() ) {
      dir = *it;
      if ( dir.isReadable() && dir.count() > 2 ) {
        url.setPath( *it );
        soundURL->fileDialog()->setURL( url );
        break;
      }
      ++it;
    }
  }
  // End copy

  enableSoundCheck = new QCheckBox(i18n("&Enable sounds for highlight list items"),
    parentFrame, "highlight_enable_sound_check");
  enableSoundCheck->setChecked(preferences->getHighlightSoundEnabled());

  currentNickCheck=new QCheckBox(i18n("Al&ways highlight current nick:"),parentFrame,"highlight_current_nick_check");
  currentNickCheck->setChecked(preferences->getHighlightNick());
  currentNickColor=new KColorCombo(parentFrame,"current_nick_color");
  currentNickColor->setColor(preferences->getHighlightNickColor());
  currentNickChanged(preferences->getHighlightNick() ? 2 : 0);

  ownLinesCheck=new QCheckBox(i18n("Always highlight own &lines:"),parentFrame,"highlight_own_lines_check");
  ownLinesColor=new KColorCombo(parentFrame,"own_lines_color");
  ownLinesCheck->setChecked(preferences->getHighlightOwnLines());
  ownLinesColor->setColor(preferences->getHighlightOwnLinesColor());
  ownLinesChanged(preferences->getHighlightOwnLines() ? 2 : 0);

  QVBox* highlightButtonBox=new QVBox(highlightListGroup);
  highlightButtonBox->setSpacing(spacingHint());
  QPushButton* newButton=new QPushButton(i18n("&New"),highlightButtonBox);
  QPushButton* removeButton=new QPushButton(i18n("&Remove"),highlightButtonBox);
  // add spacer below the two buttons
  new QVBox(highlightButtonBox);

  int row=0;
  highlightLayout->addMultiCellWidget(highlightListGroup,row,row,0,1);

  row++;
  highlightLayout->addMultiCellWidget(enableSoundCheck, row, row, 0, 1);

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
  connect(soundURL, SIGNAL(textChanged(const QString&)), this, SLOT(soundURLChanged(const QString&)));
  connect(soundPlayBtn, SIGNAL(clicked()), this, SLOT(playSound()));

  connect(autoTextInput, SIGNAL(textChanged(const QString&)), this, SLOT(autoTextChanged(const QString&)));

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
    soundURL->setEnabled(true);
    soundLabel->setEnabled(true);
    soundPlayBtn->setEnabled(true);
    autoTextLabel->setEnabled(true);
    autoTextInput->setEnabled(true);

    patternColor->setColor(highlightItem->getColor());
    patternInput->setText(highlightItem->getPattern());
    soundURL->setURL(highlightItem->getSoundURL().prettyURL());
    autoTextInput->setText(highlightItem->getAutoText());
  }
  else
  {
    patternLabel->setEnabled(false);
    patternInput->setEnabled(false);
    patternColor->setEnabled(false);
    soundURL->setEnabled(false);
    soundLabel->setEnabled(false);
    soundPlayBtn->setEnabled(false);
  }
}

void PrefsPageHighlight::highlightTextChanged(const QString& newPattern)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(item) item->setPattern(newPattern);
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

void PrefsPageHighlight::soundURLChanged(const QString& newURL)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(item)
  {
    item->setSoundURL(KURL(newURL));
  }
}

void PrefsPageHighlight::autoTextChanged(const QString& newAutoText)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(item)
  {
    item->setAutoText(newAutoText);
  }
}

void PrefsPageHighlight::addHighlight()
{
  Highlight* newHighlight=new Highlight(i18n("New"),false,QColor("#ff0000"),KURL(),QString::null);

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
      soundURL->setEnabled(false);
      soundLabel->setEnabled(false);
      soundPlayBtn->setEnabled(false);
      autoTextLabel->setEnabled(false);
      autoTextInput->setEnabled(false);
    }
  }
}

QPtrList<Highlight> PrefsPageHighlight::getHighlightList()
{
  QPtrList<Highlight> newList;

  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->firstChild());
  while(item)
  {
    newList.append(new Highlight(item->getPattern(),
                                 item->getRegExp(),
                                 item->getColor(),
                                 item->getSoundURL(),
                                 item->getAutoText()));
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
  preferences->setHighlightList(getHighlightList());
  preferences->setHighlightNick(currentNickCheck->isChecked());
  preferences->setHighlightOwnLines(ownLinesCheck->isChecked());
  preferences->setHighlightNickColor(currentNickColor->color().name());
  preferences->setHighlightOwnLinesColor(ownLinesColor->color().name());
  preferences->setHighlightSoundEnabled(enableSoundCheck->isChecked());
}

void PrefsPageHighlight::playSound()
{
  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  konvApp->sound()->play(KURL(soundURL->url()));
}

#include "prefspagehighlight.moc"
