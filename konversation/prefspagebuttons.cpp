/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Provides an interface to edit the quick buttons
  begin:     Mon Jun 9 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qregexp.h>
#include <qlayout.h>
#include <qlabel.h>

#include <klistview.h>
#include <klocale.h>

#include "prefspagebuttons.h"
#include "preferences.h"

PrefsPageButtons::PrefsPageButtons(QFrame* newParent,Preferences* newPreferences) :
                  PrefsPage(newParent,newPreferences)
{
  // Add the layout to the widget
  QVBoxLayout* buttonsLayout=new QVBoxLayout(parentFrame,marginHint(),spacingHint());

  // Set up the button list
  buttonListView=new KListView(parentFrame);

  buttonListView->addColumn(i18n("Button Name"));
  buttonListView->addColumn(i18n("Button Action"));

  buttonListView->setAllColumnsShowFocus(true);
  buttonListView->setItemsRenameable(true);
  buttonListView->setRenameable(0,true);
  buttonListView->setRenameable(1,true);
  buttonListView->setSorting(-1,false);
  buttonListView->setDragEnabled(true);
  buttonListView->setAcceptDrops(true);

  // Insert buttons in reverse order to make them appear sorted correctly
  QStringList buttonList=preferences->getButtonList();
  for(int index=8;index!=0;index--)
  {
    QString buttonText=buttonList[index-1];
    new KListViewItem(buttonListView,buttonText.section(',',0,0),buttonText.section(',',1));
  }

  QLabel* instructions=new QLabel(i18n("You can use the following placeholders:\n"
                                       "%c: Current channel\n"
                                       "%k: Channel password (not functional yet)\n"
                                       "%K: Server password\n"
                                       "%u: List of selected nicknames\n"
                                       "%s<term>%: term used to separate nicknames in %u\n"
                                       "%n: Send command directly to the server instead of your input line"),parentFrame);

  buttonsLayout->addWidget(buttonListView);
  buttonsLayout->addWidget(instructions);
}

PrefsPageButtons::~PrefsPageButtons()
{
}

QStringList PrefsPageButtons::getButtonList()
{
  QStringList newList;
  QListViewItem* item=buttonListView->itemAtIndex(0);
  while(item!=0)
  {
    QString title(item->text(0));
    // Make sure we don't have any "," in the title that would confuse Preferences
    title.replace(QRegExp(","),"_");
    newList.append(title+","+item->text(1));
    item=item->itemBelow();
  }
  return newList;
}

void PrefsPageButtons::applyPreferences()
{
  preferences->setButtonList(getButtonList());
}

#include "prefspagebuttons.moc"
