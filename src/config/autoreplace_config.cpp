/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#include "autoreplace_config.h"
#include "preferences.h"

#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>

#define DIRECTION_OUTPUT 0
#define DIRECTION_INPUT  1
#define DIRECTION_BOTH   2


Autoreplace_Config::Autoreplace_Config(QWidget* parent, const char* name)
 : QWidget(parent)
{
  setObjectName(QString::fromLatin1(name));
  setupUi(this);

  // reset flag to defined state (used to block signals when just selecting a new item)
  m_newItemSelected=false;
  //Check if the regexp editor is installed
// it does not make sense to port / enable this since KRegExpEditor is in a very bad shape. just keep this
// code here because it will probably help at a later point to port it when KRegExpEditor is again usable.
// 2009-02-06, uwolfer
  bool installed = false;//( KServiceTypeTrader::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString(), this )!= 0 );
  regExpEditorButton->setVisible(false);

  if(installed)
  {
      regExpEditorButton->setEnabled(true);
      regExpEditorButton->setStatusTip(i18n("Click to run Regular Expression Editor (KRegExpEditor)"));
  }
  else
  {
      regExpEditorButton->setEnabled(false);
      regExpEditorButton->setStatusTip(i18n("The Regular Expression Editor (KRegExpEditor) is not installed"));
  }
  // populate combobox
  directionCombo->insertItem(DIRECTION_OUTPUT, i18n("Outgoing"));
  directionCombo->insertItem(DIRECTION_INPUT, i18n("Incoming"));
  directionCombo->insertItem(DIRECTION_BOTH, i18n("Both"));

  // populate listview
  loadSettings();

  connect(patternListView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(entrySelected(QTreeWidgetItem*)));

  connect(directionCombo, SIGNAL(activated(int)), this, SLOT(directionChanged(int)));

  connect(patternInput, SIGNAL(textChanged(const QString&)), this, SLOT(patternChanged(const QString&)));
  connect(regExpEditorButton, SIGNAL(clicked()), this, SLOT(showRegExpEditor()));
  connect(replacementInput, SIGNAL(textChanged(const QString&)), this, SLOT(replacementChanged(const QString&)));

  connect(newButton, SIGNAL(clicked()), this, SLOT(addEntry()));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeEntry()));
}

Autoreplace_Config::~Autoreplace_Config()
{
}

void Autoreplace_Config::loadSettings()
{
  setAutoreplaceListView(Preferences::autoreplaceList());

  // remember autoreplace list for hasChanged()
  m_oldAutoreplaceList=Preferences::autoreplaceList();
}

// fill listview with autoreplace definitions
void Autoreplace_Config::setAutoreplaceListView(const QList<QStringList> &autoreplaceList)
{
  // clear listView
  patternListView->clear();
  // go through the list
  for (int index=0;index<autoreplaceList.count();index++)
  {
    // get autoreplace definition
    QStringList definition=autoreplaceList[index];
    // cut definition apart in name and action, and create a new listview item
    QTreeWidgetItem* newItem=new QTreeWidgetItem(patternListView);
    newItem->setFlags(newItem->flags() &~ Qt::ItemIsDropEnabled);
    newItem->setCheckState(0, Qt::Unchecked);
    // Regular expression?
    if (definition.at(0)=="1") newItem->setCheckState(0, Qt::Checked);
    // direction input/output/both
    if (definition.at(1)=="i")
    {
        newItem->setText(1,directionCombo->itemText(DIRECTION_INPUT));
    }
    else if (definition.at(1)=="o")
    {
        newItem->setText(1,directionCombo->itemText(DIRECTION_OUTPUT));
    }
    else if (definition.at(1)=="io")
    {
        newItem->setText(1,directionCombo->itemText(DIRECTION_BOTH));
    }
    // pattern
    newItem->setText(2,definition.at(2));
    // replacement
    newItem->setText(3,definition.at(3));
    // hidden column, so we are independent of the i18n()ed display string
    newItem->setText(4,definition.at(1));
  } // for
  patternListView->setCurrentItem(patternListView->topLevelItem(0));
}

// save autoreplace entries to configuration
void Autoreplace_Config::saveSettings()
{
  // get configuration object
  KSharedConfigPtr config=KGlobal::config();

  // delete all patterns
  config->deleteGroup("Autoreplace List");
  // create new empty autoreplace group

  KConfigGroup grp = config->group("Autoreplace List");

  // create empty list
  QList<QStringList> newList=currentAutoreplaceList();

  // check if there are any patterns in the list view
  if(newList.count())
  {
    // go through all patterns and save them into the configuration
    QString regexString("Regex");
    QString directString("Direction");
    QString patternString("Pattern");
    QString replaceString("Replace");
    for(int index=0;index<newList.count();index++)
    {
        // write the current entry's pattern and replacement (adds a "#" to preserve blanks at the end of the line)
        QString indexString(QString::number(index));
        QStringList definition = newList[index];
        grp.writeEntry(regexString + indexString,definition.at(0)); //regex status
        grp.writeEntry(directString + indexString,definition.at(1)); //direction
        grp.writeEntry(patternString + indexString,definition.at(2)+'#'); //pattern
        grp.writeEntry(replaceString + indexString,definition.at(3)+'#'); //replace

    } // for
  }
  // if there were no entries at all, write a dummy entry to prevent KConfigXT from "optimizing"
  // the group out, which would in turn make konvi restore the default entries
  else
    grp.writeEntry("Empty List",QString());

  // set internal autoreplace list
  Preferences::setAutoreplaceList(newList);

  // remember autoreplace list for hasChanged()
  m_oldAutoreplaceList=newList;
}

void Autoreplace_Config::restorePageToDefaults()
{
  setAutoreplaceListView(Preferences::defaultAutoreplaceList());
}

QList<QStringList> Autoreplace_Config::currentAutoreplaceList()
{
  // get first item of the autoreplace listview
  QTreeWidgetItem* item=patternListView->topLevelItem(0);
  // create empty list
  QList<QStringList> newList;
  QChar regex;
  // go through all items and save them into the configuration
  while(item)
  {
    regex = (item->checkState(0) == Qt::Checked) ? '1' : '0';
    // remember entry in internal list (col 4 is hidden for input/output)
    newList.append(QStringList() << regex << item->text(4) << item->text(2) << item->text(3));
    // get next item in the listview
    item=patternListView->itemBelow(item);
  } // while

  // return list
  return newList;
}

bool Autoreplace_Config::hasChanged()
{
  return(m_oldAutoreplaceList!=currentAutoreplaceList());
}

// slots

// what to do when the user selects an item
void Autoreplace_Config::entrySelected(QTreeWidgetItem* autoreplaceEntry)
{
  // play it safe, assume disabling all widgets first
  bool enabled=false;

  // check if there really was an item selected
  if(autoreplaceEntry)
  {
    // remember to enable the editing widgets
    enabled=true;

    // tell the editing widgets not to emit modified() on signals now
    m_newItemSelected=true;
    // update editing widget contents
    patternInput->setText(autoreplaceEntry->text(2));
    replacementInput->setText(autoreplaceEntry->text(3));

    // set combobox to selected item
    int itemIndex=0;
    QString direction=autoreplaceEntry->text(4);
    if(direction=="i") itemIndex=DIRECTION_INPUT;
    else if(direction=="o") itemIndex=DIRECTION_OUTPUT;
    else if(direction=="io") itemIndex=DIRECTION_BOTH;
    directionCombo->setCurrentIndex(itemIndex);
    // re-enable modified() signal on text changes in edit widgets
    m_newItemSelected=false;
  }
  // enable or disable editing widgets
  removeButton->setEnabled(enabled);
  directionLabel->setEnabled(enabled);
  directionCombo->setEnabled(enabled);
  patternLabel->setEnabled(enabled);
  patternInput->setEnabled(enabled);
  replacementLabel->setEnabled(enabled);
  replacementInput->setEnabled(enabled);

// see note above about KRegExpEditor
#if 0
  if(!KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty())
  {
    regExpEditorButton->setEnabled(enabled);
  }
#endif
  // make checkboxes work
  emit modified();
}

// what to do when the user changes the direction of an entry
void Autoreplace_Config::directionChanged(int newDirection)
{
  // get possible selected item
  QTreeWidgetItem* item=patternListView->currentItem();

  // sanity check
  if(item)
  {
    // prepare hidden identifier string
    QString id;
    // find the direction strings to set up in the item
    if(newDirection==DIRECTION_INPUT)       id='i';
    else if(newDirection==DIRECTION_OUTPUT) id='o';
    else if(newDirection==DIRECTION_BOTH)   id="io";
    // rename direction
    item->setText(1,directionCombo->itemText(newDirection));
    item->setText(4,id);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// what to do when the user changes the pattern of an entry
void Autoreplace_Config::patternChanged(const QString& newPattern)
{
  // get possible selected item
  QTreeWidgetItem* item=patternListView->currentItem();

  // sanity check
  if(item)
  {
    // rename pattern
    if (newPattern.length()>0)
    {
        item->setText(2,newPattern);
    }
    else
    {
        item->setText(2,i18nc("Fallback content for the \"Find:\" field of an auto-replace rule that gets set when the user leaves the field empty and e.g. switches to another rule in the list. It's identical to the default content of the field after adding a new auto-replace rule.", "New"));
    }
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// what to do when the user changes the replacement of an entry
void Autoreplace_Config::replacementChanged(const QString& newReplacement)
{
  // get possible selected item
  QTreeWidgetItem* item=patternListView->currentItem();

  // sanity check
  if(item)
  {
    // rename item
    item->setText(3,newReplacement);
    // tell the config system that something has changed
    if(!m_newItemSelected) emit modified();
  }
}

// add button pressed
void Autoreplace_Config::addEntry()
{
  // add new item at the bottom of list view
  QTreeWidgetItem* newItem=new QTreeWidgetItem(patternListView);
  newItem->setFlags(newItem->flags() &~ Qt::ItemIsDropEnabled);
  newItem->setCheckState(0, Qt::Unchecked);
  // if successful ...
  if(newItem)
  {
    // set default direction
    newItem->setText(1,directionCombo->itemText(DIRECTION_OUTPUT));
    // set default pattern name
    newItem->setText(2,i18nc("Default content of the \"Find:\" field of a newly-added auto-replace rule.", "New"));
    // set default direction
    newItem->setText(4,"o");
    // select new item and make it the current one
    patternListView->setCurrentItem(newItem);
    // set input focus on item pattern edit
    patternInput->setFocus();
    // select all text to make overwriting easier
    patternInput->selectAll();
    // tell the config system that something has changed
    emit modified();
  }
}

// remove button pressed
void Autoreplace_Config::removeEntry()
{
  // get possible first selected item
  QTreeWidgetItem* item=patternListView->currentItem();

  // sanity check
  if(item)
  {
    // get item below the current one
    QTreeWidgetItem* nextItem=patternListView->itemBelow(item);
    // if there was none, get the one above
    if(!nextItem) nextItem=patternListView->itemAbove(item);

    // remove the item from the list
    delete item;

    // check if we found the next item
    if(nextItem)
    {
      // select the item and make it the current item
      patternListView->setCurrentItem(nextItem);
    }
    else
    {
      // no next item found, this means the list is empty
      entrySelected(0);
    }
    // tell the config system that somethig has changed
    emit modified();
  }
}

void Autoreplace_Config::showRegExpEditor()
{
    QDialog *editorDialog = KServiceTypeTrader::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString(), this );

    if(editorDialog)
    {
        // kdeutils was installed, so the dialog was found.  Fetch the editor interface.
         KRegExpEditorInterface *iface = qobject_cast<KRegExpEditorInterface*>( editorDialog );
        Q_ASSERT(iface); // This should not fail!
        iface->setRegExp(patternInput->text());
        int dlgResult = editorDialog->exec();

        if(dlgResult == QDialog::Accepted)
        {
            QString re = iface->regExp();
            patternInput->setText(re);
        }

        delete editorDialog;
    }
}

#include "autoreplace_config.moc"
