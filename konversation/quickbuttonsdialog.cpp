/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  quickbuttonsdialog.cpp  -  Provides an interface to edit the quick buttons
  begin:     Mon Jun 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qregexp.h>
#include <qlabel.h>

#include <klocale.h>
#include <klistview.h>
#include <kdebug.h>

#include "quickbuttonsdialog.h"

QuickButtonsDialog::QuickButtonsDialog(QStringList buttonList,QSize size):
                    KDialogBase(0,"quickbuttonsdialog",false,i18n("Edit Quick Buttons"),
                                KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                                KDialogBase::Ok,true)
{
  kdDebug() << "QuickButtonsDialog::QuickButtonsDialog()" << endl;

  /* Create the top level widget */
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  /* Add the layout to the widget */
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());
  /* Set up the button list */
  buttonListView=new KListView(page);

  buttonListView->addColumn(i18n("Button Name"));
  buttonListView->addColumn(i18n("Button Action"));

  buttonListView->setAllColumnsShowFocus(true);
  buttonListView->setItemsRenameable(true);
  buttonListView->setRenameable(0,true);
  buttonListView->setRenameable(1,true);
  buttonListView->setSorting(-1,false);
  buttonListView->setDragEnabled(true);
  buttonListView->setAcceptDrops(true);

  /* Insert buttons in reverse order to make them appear sorted correctly */
  int index;
  for(index=8;index!=0;index--)
  {
    QStringList buttonText(QStringList::split(',',buttonList[index-1],true));
    new KListViewItem(buttonListView,buttonText[0],buttonText[1]);
  }

  QLabel* instructions=new QLabel(i18n("You can use the following placeholders:\n"
                                       "%c: Current channel\n"
                                       "%u: List of selected nicknames\n"
                                       "%s<term>%: term used to separate nicknames in %u\n"
                                       "%n: Send command directly to the server instead of your input line"),page);

  dialogLayout->addWidget(buttonListView);
  dialogLayout->addWidget(instructions);

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));

  setInitialSize(size);
}

QuickButtonsDialog::~QuickButtonsDialog()
{
}

void QuickButtonsDialog::slotOk()
{
  slotApply();
  slotCancel();
}

void QuickButtonsDialog::slotApply()
{
  emit applyClicked(getButtonList());
}

void QuickButtonsDialog::slotCancel()
{
  emit cancelClicked(size());
}

QStringList QuickButtonsDialog::getButtonList()
{
  QStringList newList;
  QListViewItem* item=buttonListView->itemAtIndex(0);
  while(item!=0)
  {
    QString title(item->text(0));
    /* Make sure we don't have any "," in the title that would confuse Preferences */
    title.replace(QRegExp(","),"_");
    newList.append(title+","+item->text(1));
    item=item->itemBelow();
  }
  return newList;
}
