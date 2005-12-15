#include <klocale.h>
#include <klistview.h>
#include <qlistview.h>
#include "ignore_preferences.h"
#include "ignorelistviewitem.h"
#include "ignore.h"
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kpushbutton.h>

Ignore_Config::Ignore_Config( QWidget* parent, const char* name, WFlags fl )
    : Ignore_ConfigUI( parent, name, fl )
{
    connect(newButton,SIGNAL(clicked()),
        this,SLOT(newIgnore()));
    connect(removeButton,SIGNAL(clicked()),
        this,SLOT(removeIgnore()));
    connect(removeAllButton,SIGNAL(clicked()),
        ignoreListView,SLOT(removeAll()));
    connect(ignoreListView,SIGNAL(selectionChanged(QListViewItem*)),
        this,SLOT(select(QListViewItem*)));

    updateWidgets();
}

Ignore_Config::~Ignore_Config()
{

}

void Ignore_Config::newIgnore()
{
     new IgnoreListViewItem(ignoreListView,
        "new!new@new.new",
        Ignore::Channel |
        Ignore::Query |
        Ignore::Notice |
        Ignore::CTCP |
        Ignore::DCC);
 
}
void Ignore_Config::removeIgnore()
{
    delete ignoreListView->selectedItem();
}

void Ignore_Config::saveSettings()
{
}

void Ignore_Config::updateWidgets()
{
/*    QPtrList<Ignore> ignoreList=preferences->getIgnoreList();
    // Insert Ignore items backwards to get them sorted properly
    Ignore* item=ignoreList.last();
    ignoreListView->clear();
    while(item)
    {
        new IgnoreListViewItem(ignoreListView,item->getName(),item->getFlags());
        item=ignoreList.prev();
    }*/

}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Ignore_Config::languageChange()
{
  updateWidgets();
}

#include "ignore_preferences.moc"
