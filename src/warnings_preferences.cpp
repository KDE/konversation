#include <klocale.h>
#include <klistview.h>
#include <qlistview.h>
#include "warnings_preferences.h"
#include <kapplication.h>
#include <kconfig.h>
#include "konviconfigdialog.h"
#include <kdebug.h>

Warnings_Config::Warnings_Config( QWidget* parent, const char* name, WFlags fl )
    : Warnings_ConfigUI( parent, name, fl )
{
  dialogListView->setSorting(1);
  loadSettings();
  connect(dialogListView, SIGNAL(clicked(QListViewItem *)), this, SIGNAL(modified()));
}

Warnings_Config::~Warnings_Config()
{

}

void Warnings_Config::restorePageToDefaults()
{
  
  QCheckListItem* item=static_cast<QCheckListItem*>(dialogListView->itemAtIndex(0));
  bool changed=false;
  while(item)
  {
    if(!item->isOn()) {
      item->setOn(true);
      changed=true;
    }
    item=static_cast<QCheckListItem*>(item->itemBelow());
  }
  if(changed) {
    emit modified();
  }
}

void Warnings_Config::saveSettings()
{
  KConfig* config = kapp->config();
  config->setGroup("Notification Messages");
  
  QCheckListItem* item=static_cast<QCheckListItem*>(dialogListView->itemAtIndex(0));
  int i=0;
  while(item)
  {
    config->writeEntry(item->text(2),item->isOn());
    kdDebug() << "saving: " << item->text(2) << " as " <<  item->text(1) << endl;
    item=static_cast<QCheckListItem*>(item->itemBelow());
    ++i;
  }
}

void Warnings_Config::loadSettings()
{
  QStringList dialogDefinitions;
  QString flagNames = "Invitation,SaveLogfileNote,ClearLogfileQuestion,CloseQueryAfterIgnore,ResumeTransfer,QuitServerTab,QuitChannelTab,QuitQueryTab,ChannelListNoServerSelected,RemoveDCCReceivedFile,HideMenuBarWarning,ChannelListWarning,LargePaste";
  dialogDefinitions.append(i18n("Automatically join channel on invite"));
  dialogDefinitions.append(i18n("Notice that saving logfiles will save whole file"));
  dialogDefinitions.append(i18n("Question before deleting logfile contents"));
  dialogDefinitions.append(i18n("Question on closing queries after ignoring the nickname"));
  dialogDefinitions.append(i18n("Question on what to do on DCC resume"));
  dialogDefinitions.append(i18n("Close server tab"));
  dialogDefinitions.append(i18n("Close channel tab"));
  dialogDefinitions.append(i18n("Close query tab"));
  dialogDefinitions.append(i18n("The channel list can only be opened from server-aware tabs"));
  dialogDefinitions.append(i18n("Warning on deleting file received on DCC"));
  dialogDefinitions.append(i18n("Warning on hiding the main window menu"));
  dialogDefinitions.append(i18n("Warning on high traffic with channel list"));
  dialogDefinitions.append(i18n("Warning on pasting large portions of text"));
  QCheckListItem *item;
  dialogListView->clear();

  KConfig* config = kapp->config();
  config->setGroup("Notification Messages");
  QString flagName; 
  for(unsigned int i=0; i<dialogDefinitions.count() ;i++)
  {
    item=new QCheckListItem(dialogListView,dialogDefinitions[i],QCheckListItem::CheckBox);
    item->setText(1,dialogDefinitions[i]);
    flagName = flagNames.section(",",i,i);
    item->setText(2,flagName);
    item->setOn(config->readBoolEntry(flagName,true));
    //kdDebug() << "loading: " << item->text(2) << " as " <<  item->text(1) << endl;
  }
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Warnings_Config::languageChange()
{
  loadSettings();
}

#include "warnings_preferences.moc"
