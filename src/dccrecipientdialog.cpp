/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  lets the user choose a nick from the list
  begin:     Sam Dez 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "dccrecipientdialog.h"

#include <qlayout.h>
//Added by qt3to4:
#include <Q3VBoxLayout>

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>


QString DccRecipientDialog::selectedNickname;     // static

DccRecipientDialog::DccRecipientDialog(QWidget* parent, const QStringList &list,const QSize &size) :
  KDialogBase(parent,"dcc_recipient_dialog",true,i18n("Select Recipient"),
	      KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,true)
{
    // Create the top level widget
    QWidget* page=new QWidget(this);
    setMainWidget(page);
    // Add the layout to the widget
    Q3VBoxLayout* dialogLayout=new Q3VBoxLayout(page);
    dialogLayout->setSpacing(spacingHint());
    // Add the nickname list widget
    K3ListBox* nicknameList=new K3ListBox(page,"recipient_list");

    nicknameList->insertStringList(list);
    nicknameList->sort(true);

    nicknameInput=new KLineEdit(page,"nickname_input");

    dialogLayout->addWidget(nicknameList);
    dialogLayout->addWidget(nicknameInput);

    connect(nicknameList,SIGNAL (highlighted(Q3ListBoxItem*)),this,SLOT (newNicknameSelected(Q3ListBoxItem*)) );
    connect(nicknameList,SIGNAL (doubleClicked(Q3ListBoxItem*)),this,SLOT (newNicknameSelectedQuit(Q3ListBoxItem*)) );

    setButtonOK(KGuiItem(i18n("&OK"),"button_ok",i18n("Select nickname and close the window")));
    setButtonCancel(KGuiItem(i18n("&Cancel"),"button_cancel",i18n("Close the window without changes")));

    setInitialSize(size);
    show();
}

DccRecipientDialog::~DccRecipientDialog()
{
}

QString DccRecipientDialog::getSelectedNickname()
{
    return selectedNickname;
}

void DccRecipientDialog::newNicknameSelected(Q3ListBoxItem* item)
{
    nicknameInput->setText(item->text());
}

void DccRecipientDialog::newNicknameSelectedQuit(Q3ListBoxItem* item)
{
    newNicknameSelected(item);
    selectedNickname=nicknameInput->text();

    delayedDestruct();
}

void DccRecipientDialog::slotCancel()
{
    selectedNickname=QString();
    KDialogBase::slotCancel();
}

void DccRecipientDialog::slotOk()
{
    selectedNickname=nicknameInput->text();
    KDialogBase::slotOk();
}

QString DccRecipientDialog::getNickname(QWidget* parent, const QStringList& list)
{
    QSize size;                                   // TODO: get it from Preferences
    DccRecipientDialog dlg(parent,list,size);
    dlg.exec();

    return dlg.getSelectedNickname();
}

#include "dccrecipientdialog.moc"
