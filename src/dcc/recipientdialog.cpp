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

#include "recipientdialog.h"
#include "preferences.h"

#include <QListView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>


QString DccRecipientDialog::selectedNickname;     // static

DccRecipientDialog::DccRecipientDialog(QWidget* parent, QAbstractListModel* model) :
  KDialog(parent)
{
    // Create the top level widget
    QWidget* page=new QWidget(this);
    setMainWidget(page);
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setModal( true );
    setCaption( i18n("Select Recipient") );
    // Add the layout to the widget
    QVBoxLayout* dialogLayout=new QVBoxLayout(page);
    dialogLayout->setSpacing(spacingHint());
    // Add the nickname list widget
    QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(this);
    sortModel->setSortCaseSensitivity(Preferences::self()->sortCaseInsensitive() ? Qt::CaseInsensitive : Qt::CaseSensitive);
    sortModel->setSourceModel(model);
    sortModel->sort(0, Qt::AscendingOrder);
    QListView* nicknameList = new QListView(page);
    nicknameList->setUniformItemSizes(true);
    nicknameList->setModel(sortModel);

    nicknameInput=new KLineEdit(page);

    dialogLayout->addWidget(nicknameList);
    dialogLayout->addWidget(nicknameInput);

    connect(nicknameList, SIGNAL(clicked(QModelIndex)), this, SLOT(newNicknameSelected(QModelIndex)));
    connect(nicknameList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(newNicknameSelectedQuit(QModelIndex)));

    setButtonGuiItem(KDialog::Ok, KGuiItem(i18n("&OK"), "dialog-ok", i18n("Select nickname and close the window")));
    setButtonGuiItem(KDialog::Cancel, KGuiItem(i18n("&Cancel"), "dialog-cancel", i18n("Close the window without changes")));

    restoreDialogSize(KConfigGroup(KGlobal::config(), "DCCRecipientDialog"));

    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
    connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancel() ) );
}

DccRecipientDialog::~DccRecipientDialog()
{
    KConfigGroup config(KGlobal::config(), "DCCRecipientDialog");
    saveDialogSize(config);
}

QString DccRecipientDialog::getSelectedNickname()
{
    return selectedNickname;
}

void DccRecipientDialog::newNicknameSelected(const QModelIndex& index)
{
    nicknameInput->setText(index.data().toString());
}

void DccRecipientDialog::newNicknameSelectedQuit(const QModelIndex& index)
{
    newNicknameSelected(index);
    selectedNickname = nicknameInput->text();

    delayedDestruct();
}

void DccRecipientDialog::slotCancel()
{
    selectedNickname.clear();
    reject();
}

void DccRecipientDialog::slotOk()
{
    selectedNickname=nicknameInput->text();
    accept();
}

QString DccRecipientDialog::getNickname(QWidget* parent, QAbstractListModel* model)
{
    QPointer<DccRecipientDialog> dlg = new DccRecipientDialog(parent, model);
    dlg->exec();
    const QString selectedNick = dlg->getSelectedNickname();

    delete dlg;
    return selectedNick;
}

#include "recipientdialog.moc"
