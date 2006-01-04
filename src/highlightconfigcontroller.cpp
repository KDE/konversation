//
// C++ Implementation: highlightconfigcontroller
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2003, 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qdir.h>
#include <qlabel.h>

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <klistview.h>
#include <klineedit.h>
#include <kcolorcombo.h>
#include <klocale.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>

#include "config/preferences.h"

#include "highlight_preferences.h"
#include "highlightconfigcontroller.h"
#include "highlightviewitem.h"
#include "konversationapplication.h"
#include "konversationsound.h"

HighlightConfigController::HighlightConfigController(Highlight_Config* highlightPage,QObject* parent, const char* name)
 : QObject(parent,name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  m_highlightPage=highlightPage;
  populateHighlightList();

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
        m_highlightPage->soundURL->fileDialog()->setURL( url );
        break;
      }
      ++it;
    }
  }
  // End copy

  connect(m_highlightPage->highlightListView,SIGNAL (selectionChanged(QListViewItem*)),this,SLOT (highlightSelected(QListViewItem*)) );
  connect(m_highlightPage->highlightListView,SIGNAL (clicked(QListViewItem*)),this,SLOT (highlightSelected(QListViewItem*)) );
  
  connect(m_highlightPage->patternInput,SIGNAL (textChanged(const QString&)),this,SLOT (highlightTextChanged(const QString&)) );
  connect(m_highlightPage->patternButton,SIGNAL (clicked()),this,SLOT(highlightTextEditButtonClicked()));
  connect(m_highlightPage->patternColor,SIGNAL (activated(const QColor&)),this,SLOT (highlightColorChanged(const QColor&)) );
  
  connect(m_highlightPage->soundURL, SIGNAL(textChanged(const QString&)), this, SLOT(soundURLChanged(const QString&)));
  connect(m_highlightPage->soundPlayBtn, SIGNAL(clicked()), this, SLOT(playSound()));

  connect(m_highlightPage->autoTextInput,SIGNAL (textChanged(const QString&)),this,SLOT (autoTextChanged(const QString&)) );

  connect(m_highlightPage->newButton,SIGNAL (clicked()),this,SLOT (addHighlight()) );
  connect(m_highlightPage->removeButton,SIGNAL (clicked()),this,SLOT (removeHighlight()) );
}

HighlightConfigController::~HighlightConfigController()
{
}

void HighlightConfigController::populateHighlightList()
{
  QPtrList<Highlight> highlightList=Preferences::highlightList();
  // fill in the highlight patterns backwards to keep the right sorting order
  for(unsigned int i=highlightList.count();i!=0;i--)
  {
    Highlight* currentHighlight=highlightList.at(i-1);
    new HighlightViewItem(m_highlightPage->highlightListView,currentHighlight);
  }
}

// Slots:

void HighlightConfigController::highlightSelected(QListViewItem* item)
{
  if(item)
  {
    HighlightViewItem* highlightItem=static_cast<HighlightViewItem*>(item);

    m_highlightPage->patternLabel->setEnabled(true);
    m_highlightPage->patternInput->setEnabled(true);
    m_highlightPage->patternButton->setEnabled(true);
    m_highlightPage->patternColor->setEnabled(true);
    m_highlightPage->soundURL->setEnabled(true);
    m_highlightPage->soundLabel->setEnabled(true);
    m_highlightPage->soundPlayBtn->setEnabled(true);
    m_highlightPage->autoTextLabel->setEnabled(true);
    m_highlightPage->autoTextInput->setEnabled(true);
    
    // Determine if kdeutils Regular Expression Editor is installed.  If so, enable edit button.
    m_highlightPage->patternButton->setEnabled(!KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty());

    // tell all now emitted signals that we just clicked on a new item, so they should
    // not emit the modified() signal.
    newItemSelected=true;
    m_highlightPage->patternColor->setColor(highlightItem->getColor());
    m_highlightPage->patternInput->setText(highlightItem->getPattern());
    m_highlightPage->soundURL->setURL(highlightItem->getSoundURL().prettyURL());
    m_highlightPage->autoTextInput->setText(highlightItem->getAutoText());
    // all signals will now emit the modified() signal again
    newItemSelected=false;
  }
  else
  {
    m_highlightPage->patternLabel->setEnabled(false);
    m_highlightPage->patternInput->setEnabled(false);
    m_highlightPage->patternButton->setEnabled(false);
    m_highlightPage->patternColor->setEnabled(false);
    m_highlightPage->soundURL->setEnabled(false);
    m_highlightPage->soundLabel->setEnabled(false);
    m_highlightPage->soundPlayBtn->setEnabled(false);
    m_highlightPage->autoTextLabel->setEnabled(false);
    m_highlightPage->autoTextInput->setEnabled(false);
  }
}

void HighlightConfigController::highlightTextChanged(const QString& newPattern)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setPattern(newPattern);
    emit modified();
  }
}

void HighlightConfigController::highlightTextEditButtonClicked()
{
  QDialog *editorDialog =
      KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor" );
  if (editorDialog)
  {
        // kdeutils was installed, so the dialog was found.  Fetch the editor interface.
    KRegExpEditorInterface *reEditor =
        static_cast<KRegExpEditorInterface *>(editorDialog->qt_cast( "KRegExpEditorInterface" ) );
    Q_ASSERT( reEditor ); // This should not fail!// now use the editor.
    reEditor->setRegExp(m_highlightPage->patternInput->text());
    int dlgResult = editorDialog->exec();
    if ( dlgResult == QDialog::Accepted )
    {
      QString re = reEditor->regExp();
      m_highlightPage->patternInput->setText(re);
      HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->selectedItem());
      if(item) item->setPattern(re);
    }
    delete editorDialog;
  }
}

void HighlightConfigController::highlightColorChanged(const QColor& newColor)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setColor(newColor);
    item->repaint();
    emit modified();
  }
}

void HighlightConfigController::soundURLChanged(const QString& newURL)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setSoundURL(KURL(newURL));
    emit modified();
  }
}

void HighlightConfigController::autoTextChanged(const QString& newText)
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->selectedItem());

  if(!newItemSelected && item)
  {
    item->setAutoText(newText);
    emit modified();
  }
}

void HighlightConfigController::addHighlight()
{
  Highlight* newHighlight=new Highlight(i18n("New"),false,QColor("#ff0000"),KURL(),QString::null);

  HighlightViewItem* item=new HighlightViewItem(m_highlightPage->highlightListView,newHighlight);
  m_highlightPage->highlightListView->setSelected(item,true);
  emit modified();
}

void HighlightConfigController::removeHighlight()
{
  HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->selectedItem());

  if(item)
  {
    delete item;

    item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->currentItem());

    if(item)
      m_highlightPage->highlightListView->setSelected(item,true);
    else
    {
      m_highlightPage->patternLabel->setEnabled(false);
      m_highlightPage->patternInput->setEnabled(false);
      m_highlightPage->patternColor->setEnabled(false);
      m_highlightPage->patternButton->setEnabled(false);
      m_highlightPage->soundURL->setEnabled(false);
      m_highlightPage->soundLabel->setEnabled(false);
      m_highlightPage->soundPlayBtn->setEnabled(false);
    }
    emit modified();
  }
}

QPtrList<Highlight> HighlightConfigController::getHighlightList()
{
  QPtrList<Highlight> newList;

  HighlightViewItem* item=static_cast<HighlightViewItem*>(m_highlightPage->highlightListView->firstChild());
  while(item)
  {
    newList.append(new Highlight(item->getPattern(),item->getRegExp(),item->getColor(),item->getSoundURL(),""));
    item=item->itemBelow();
  }

  return newList;
}

void HighlightConfigController::playSound()
{
  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  konvApp->sound()->play(KURL(m_highlightPage->soundURL->url()));
}

void HighlightConfigController::saveSettings()
{
  KConfig* config = kapp->config();

  // Write all highlight entries
  QPtrList<Highlight> hiList=getHighlightList();
  int i = 0;
  for(Highlight* hl = hiList.first(); hl; hl = hiList.next())
  {
    config->setGroup(QString("Highlight%1").arg(i));
    config->writeEntry("Pattern", hl->getPattern());
    config->writeEntry("RegExp", hl->getRegExp());
    config->writeEntry("Color", hl->getColor());
    config->writePathEntry("Sound", hl->getSoundURL().prettyURL());
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
}

#include "highlightconfigcontroller.moc"
