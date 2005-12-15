#include <klocale.h>
#include <klistview.h>
#include <qlistview.h>
#include <qlineedit.h>
#include "ignore_preferences.h"
#include "ignorelistviewitem.h"
#include "ignore.h"
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <qcheckbox.h>
#include "preferences.h"

Ignore_Config::Ignore_Config( QWidget* parent, const char* name, WFlags fl )
    : Ignore_ConfigUI( parent, name, fl )
{
    connect(newButton,SIGNAL(clicked()),
        this,SLOT(newIgnore()));
    connect(removeButton,SIGNAL(clicked()),
        this,SLOT(removeIgnore()));
    connect(removeAllButton,SIGNAL(clicked()),
        ignoreListView,SLOT(clear()));
    connect(ignoreListView,SIGNAL(selectionChanged(QListViewItem*)),
        this,SLOT(select(QListViewItem*)));
    connect(chkChannel, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkQuery, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkNotice, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkCTCP, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(chkDCC, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
    connect(txtPattern, SIGNAL(textChanged(const QString &)), this, SLOT(flagCheckboxChanged()));
//    connect(chkException, SIGNAL(clicked()), this, SLOT(flagCheckboxChanged()));
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

QPtrList<Ignore> Ignore_Config::getIgnoreList()
{
    QPtrList<Ignore> newList;

    IgnoreListViewItem* item=static_cast<IgnoreListViewItem*>(ignoreListView->firstChild());
    while(item)
    {
        Ignore* newItem=new Ignore(item->text(0),item->getFlags());
        newList.append(newItem);
        item=item->itemBelow();
    }

    return newList;
}
void Ignore_Config::saveSettings()
{
    Preferences::setIgnoreList(getIgnoreList());
}

void Ignore_Config::updateWidgets()
{
    QPtrList<Ignore> ignoreList=Preferences::ignoreList();
    // Insert Ignore items backwards to get them sorted properly
    Ignore* item=ignoreList.last();
    ignoreListView->clear();
    while(item)
    {
        new IgnoreListViewItem(ignoreListView,item->getName(),item->getFlags());
        item=ignoreList.prev();
    }

    select(NULL);
}


void Ignore_Config::select(QListViewItem* item)
{
    // FIXME: Cast to IgnoreListViewItem, maybe derive from KListView some day
    IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(item);

    chkChannel->setEnabled(selectedItem != NULL);
    chkQuery->setEnabled(selectedItem != NULL);
    chkNotice->setEnabled(selectedItem != NULL);
    chkCTCP->setEnabled(selectedItem != NULL);
    chkDCC->setEnabled(selectedItem != NULL);
//	chkExceptions->setEnabled(selectedItem != NULL);
    txtPattern->setEnabled(selectedItem != NULL);

    if(selectedItem)
    {
	int flags = selectedItem->getFlags();
        chkChannel->setChecked(flags & Ignore::Channel);
        chkQuery->setChecked(flags & Ignore::Query);
        chkNotice->setChecked(flags & Ignore::Notice);
        chkCTCP->setChecked(flags & Ignore::CTCP);
        chkDCC->setChecked(flags & Ignore::DCC);
	txtPattern->setText(selectedItem->getName());

//        chkExceptions->setChecked(flags & Ignore::Exception) ;
    }
}

void Ignore_Config::flagCheckboxChanged()
{
    int flags = 0;
    if(chkChannel->isChecked()) flags |= Ignore::Channel;
    if(chkQuery->isChecked()) flags |= Ignore::Query;
    if(chkNotice->isChecked()) flags |= Ignore::Notice;
    if(chkCTCP->isChecked()) flags |= Ignore::CTCP;
    if(chkDCC->isChecked()) flags |= Ignore::DCC;
    
//    if(chkExceptions->isChecked()) flags |= Ignore::Exceptions;
    IgnoreListViewItem* selectedItem=static_cast<IgnoreListViewItem*>(ignoreListView->selectedItem());
    if(selectedItem) {
        selectedItem->setFlags(flags);
	selectedItem->setName(txtPattern->text());
    }
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
