//
// C++ Implementation: quickbuttonsconfigcontroller
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <klistview.h>

#include "config/preferences.h"

#include "quickbuttons_preferences.h"
#include "quickbuttonsconfigcontroller.h"

QuickButtonsConfigController::QuickButtonsConfigController(QuickButtons_Config* quickButtonsPage,QObject *parent, const char *name)
 : QObject(parent, name)
{
  // reset flag to defined state (used to block signals when just selecting a new item)
  newItemSelected=false;

  m_quickButtonsPage=quickButtonsPage;
  populateQuickButtonsList();
}

QuickButtonsConfigController::~QuickButtonsConfigController()
{
}

void QuickButtonsConfigController::saveSettings()
{
}

void QuickButtonsConfigController::populateQuickButtonsList()
{
  QStringList buttonList(Preferences::buttonList());
  for(unsigned int index=0;index<buttonList.count();index++)
  {
    QString definition=buttonList[index];
    new KListViewItem(m_quickButtonsPage->buttonListView,definition.section(',',0,0),definition.section(',',1));
  }
}

// slots

void QuickButtonsConfigController::entrySelected(QListViewItem* quickButtonEntry)
{
}

#include "quickbuttonsconfigcontroller.moc"
