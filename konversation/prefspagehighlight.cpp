/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Preferences panel for highlight patterns
  begin:     Die Jun 10 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>
#include <qhgroupbox.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
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
#include <kpushbutton.h>

#include "prefspagehighlight.h"
#include "preferences.h"
#include "highlight.h"
#include "highlightviewitem.h"
#include "konversationapplication.h"
#include "konversationsound.h"

PrefsPageHighlight::PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences) :
  Highlight_Config(newParent)
{
  preferences = newPreferences;

  highlightListView->setSorting(-1);

  QPtrList<Highlight> highlightList=preferences->getHighlightList();
  // fill in the highlight patterns backwards to keep the right sorting order
  for(unsigned int i=highlightList.count();i!=0;i--)
  {
    Highlight* currentHighlight=highlightList.at(i-1);
    new HighlightViewItem(highlightListView,currentHighlight);
  }

  soundPlayBtn->setIconSet(SmallIconSet( "player_play" ));
  soundURL->setCaption(i18n("Select Sound File"));

  // This code was copied from KNotifyWidget::openSoundDialog() (knotifydialog.cpp) [it's under LGPL v2]
  // find the first "sound"-resource that contains files
  QStringList soundDirs = KGlobal::dirs()->resourceDirs( "sound" );

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

  enableSoundCheck->setChecked(preferences->getHighlightSoundEnabled());

  currentNickCheck->setChecked(preferences->getHighlightNick());
  currentNickColor->setColor(preferences->getHighlightNickColor());
  currentNickChanged(preferences->getHighlightNick() ? 2 : 0);

  ownLinesCheck->setChecked(preferences->getHighlightOwnLines());
  ownLinesColor->setColor(preferences->getHighlightOwnLinesColor());
  ownLinesChanged(preferences->getHighlightOwnLines() ? 2 : 0);

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
