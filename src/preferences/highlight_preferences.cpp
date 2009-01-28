/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#include "highlight_preferences.h"
#include "highlightviewitem.h"
#include "application.h" ////// header renamed
#include "sound.h" ////// header renamed
#include "config/preferences.h"

#include <qdir.h>
#include <qlabel.h>
#include <q3header.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
//Added by qt3to4:
#include <Q3PtrList>

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <k3listview.h>
#include <klineedit.h>
#include <kcolorbutton.h>
#include <klocale.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>
#include <kiconloader.h>


Highlight_Config::Highlight_Config(QWidget* parent, const char* name)
 : Highlight_ConfigUI(parent,name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  loadSettings();

  // make list accept drag & drop
  highlightListView->setSorting(-1);
  highlightListView->header()->setMovingEnabled(false);

  soundPlayBtn->setIconSet(SmallIconSet( "player_play" ));
  soundURL->setCaption(i18n("Select Sound File"));

  // This code was copied from KNotifyWidget::openSoundDialog() (knotifydialog.cpp) [it's under LGPL v2]
  // find the first "sound"-resource that contains files
  QStringList soundDirs = KGlobal::dirs()->findDirs("data", "konversation/sounds");
  soundDirs += KGlobal::dirs()->resourceDirs( "sound" );

  if ( !soundDirs.isEmpty() ) {
    KUrl url;
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

  connect(highlightListView,SIGNAL (selectionChanged(Q3ListViewItem*)),this,SLOT (highlightSelected(Q3ListViewItem*)) );
  connect(highlightListView,SIGNAL (clicked(Q3ListViewItem*)),this,SLOT (highlightSelected(Q3ListViewItem*)) );
  connect(highlightListView,SIGNAL (spacePressed(Q3ListViewItem*)),this,SLOT (highlightSelected(Q3ListViewItem*)) );

  connect(highlightListView,SIGNAL (moved()),this,SIGNAL (modified()) );

  connect(patternInput,SIGNAL (textChanged(const QString&)),this,SLOT (highlightTextChanged(const QString&)) );
  connect(patternButton,SIGNAL (clicked()),this,SLOT(highlightTextEditButtonClicked()));
  connect(patternColor,SIGNAL (changed(const QColor&)),this,SLOT (highlightColorChanged(const QColor&)) );

  connect(soundURL, SIGNAL(textChanged(const QString&)), this, SLOT(soundURLChanged(const QString&)));
  connect(soundPlayBtn, SIGNAL(clicked()), this, SLOT(playSound()));

  connect(autoTextInput,SIGNAL (textChanged(const QString&)),this,SLOT (autoTextChanged(const QString&)) );

  connect(newButton,SIGNAL (clicked()),this,SLOT (addHighlight()) );
  connect(removeButton,SIGNAL (clicked()),this,SLOT (removeHighlight()) );

  updateButtons();
}

Highlight_Config::~Highlight_Config()
{
}

void Highlight_Config::restorePageToDefaults()
{
  if(highlightListView->childCount() != 0) {
    highlightListView->clear();
    emit modified();
  }
}

void Highlight_Config::loadSettings()
{
  Q3PtrList<Highlight> highlightList=Preferences::highlightList();
  highlightListView->clear();
  // fill in the highlight patterns backwards to keep the right sorting order
  for(unsigned int i=highlightList.count();i!=0;i--)
  {
    Highlight* currentHighlight=highlightList.at(i-1);
    new HighlightViewItem(highlightListView,currentHighlight);
  }

  highlightListView->setSelected(highlightListView->firstChild(), true);

  // remember current list for hasChanged()
  m_oldHighlightList=currentHighlightList();
}

bool Highlight_Config::hasChanged()
{
  return(m_oldHighlightList!=currentHighlightList());
}

// Slots:

void Highlight_Config::highlightSelected(Q3ListViewItem* item)
{
  // check if there was a widget selected at all
  if(item)
  {
    // make a highlight item out of the generic qlistviewitem
    HighlightViewItem* highlightItem=static_cast<HighlightViewItem*>(item);

    // check if the checkbox on the item has changed
    if(highlightItem->hasChanged())
    {
      // tell the prefs system it was changed and acknowledge the change to the listview item
      emit modified();
      highlightItem->changeAcknowledged();
    }

    // tell all now emitted signals that we just clicked on a new item, so they should
    // not emit the modified() signal.
    newItemSelected=true;
   patternColor->setColor(highlightItem->getColor());
   patternInput->setText(highlightItem->getPattern());
   soundURL->setURL(highlightItem->getSoundURL().prettyUrl());
   autoTextInput->setText(highlightItem->getAutoText());
    // all signals will now emit the modified() signal again
    newItemSelected=false;
    // remember to enable all edit widgets
  }
  updateButtons();

 }

void Highlight_Config::updateButtons()
{
  bool enabled = highlightListView->selectedItem() != NULL;
  // is the kregexpeditor installed?
  bool installed = !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty();
  // enable or disable edit widgets
  patternLabel->setEnabled(enabled);
  patternInput->setEnabled(enabled);
  patternButton->setEnabled(enabled && installed);
  colorLabel->setEnabled(enabled);
  patternColor->setEnabled(enabled);
  soundURL->setEnabled(enabled);
  soundLabel->setEnabled(enabled);
  soundPlayBtn->setEnabled(enabled);
  autoTextLabel->setEnabled(enabled);
  autoTextInput->setEnabled(enabled);

  if(installed)
  {
      QToolTip::add(patternButton, i18n("Click to run Regular Expression Editor (KRegExpEditor)"));
  }
  else
  {
      QToolTip::add(patternButton, i18n("The Regular Expression Editor (KRegExpEditor) is not installed"));
  }
}

void Highlight_Config::highlightTextChanged(const QString& newPattern)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setPattern(newPattern);
    emit modified();
  }
}

void Highlight_Config::highlightTextEditButtonClicked()
{
  QDialog *editorDialog =
      KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor" );
  if (editorDialog)
  {
        // kdeutils was installed, so the dialog was found.  Fetch the editor interface.
    KRegExpEditorInterface *reEditor =
        static_cast<KRegExpEditorInterface *>(editorDialog->qt_cast( "KRegExpEditorInterface" ) );
    Q_ASSERT( reEditor ); // This should not fail!// now use the editor.
    reEditor->setRegExp(patternInput->text());
    int dlgResult = editorDialog->exec();
    if ( dlgResult == QDialog::Accepted )
    {
      QString re = reEditor->regExp();
      patternInput->setText(re);
      HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());
      if(item) item->setPattern(re);
    }
    delete editorDialog;
  }
}

void Highlight_Config::highlightColorChanged(const QColor& newColor)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setColor(newColor);
    item->repaint();
    emit modified();
  }
}

void Highlight_Config::soundURLChanged(const QString& newURL)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setSoundURL(KUrl(newURL));
    emit modified();
  }
}

void Highlight_Config::autoTextChanged(const QString& newText)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setAutoText(newText);
    emit modified();
  }
}

void Highlight_Config::addHighlight()
{
  Highlight* newHighlight=new Highlight(i18n("New"),false,QColor("#ff0000"),KUrl(),QString());

  HighlightViewItem* item=new HighlightViewItem(highlightListView,newHighlight);
  highlightListView->setSelected(item,true);
  patternInput->setFocus();
  patternInput->selectAll();
  emit modified();
}

void Highlight_Config::removeHighlight()
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->selectedItem());

  if(item)
  {
    delete item;

    item=static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if(item)
      highlightListView->setSelected(item,true);

    emit modified();
  }
  updateButtons();
}

Q3PtrList<Highlight> Highlight_Config::getHighlightList()
{
  Q3PtrList<Highlight> newList;

  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->firstChild());
  while(item)
  {
    newList.append(new Highlight(item->getPattern(),item->getRegExp(),item->getColor(),item->getSoundURL(),item->getAutoText()));
    item=item->itemBelow();
  }

  return newList;
}

QStringList Highlight_Config::currentHighlightList()
{
  QStringList newList;

  HighlightViewItem* item=static_cast<HighlightViewItem*>(highlightListView->firstChild());
  while(item)
  {
    newList.append(item->getPattern()+item->getRegExp()+item->getColor().name()+item->getSoundURL().url()+item->getAutoText());
    item=item->itemBelow();
  }

  return newList;
}

void Highlight_Config::playSound()
{
  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  konvApp->sound()->play(KUrl(soundURL->url()));
}

void Highlight_Config::saveSettings()
{
  KConfig* config = KGlobal::config();

  // Write all highlight entries
  Q3PtrList<Highlight> hiList=getHighlightList();
  int i = 0;
  for(Highlight* hl = hiList.first(); hl; hl = hiList.next())
  {
    config->setGroup(QString("Highlight%1").arg(i));
    config->writeEntry("Pattern", hl->getPattern());
    config->writeEntry("RegExp", hl->getRegExp());
    config->writeEntry("Color", hl->getColor());
    config->writePathEntry("Sound", hl->getSoundURL().prettyUrl());
    config->writeEntry("AutoText", hl->getAutoText());
    i++;
  }

  Preferences::setHighlightList(hiList);

  // Remove unused entries...
  while(config->hasGroup(QString("Highlight%1").arg(i)))
  {
    config->deleteGroup(QString("Highlight%1").arg(i));
    i++;
  }

  // remember current list for hasChanged()
  m_oldHighlightList=currentHighlightList();
}

// #include "./preferences/highlight_preferences.moc"
