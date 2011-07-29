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
/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "recipientdialog.h"
#include "preferences.h"

#include <QListView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include <KLocale>
#include <KLineEdit>

namespace Konversation
{
    namespace DCC
    {
        QString RecipientDialog::selectedNickname;     // static

        RecipientDialog::RecipientDialog(QWidget* parent, QAbstractListModel* model) :
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

            connect( this, SIGNAL(okClicked()), this, SLOT(slotOk()) );
            connect( this, SIGNAL(cancelClicked()), this, SLOT(slotCancel()) );
        }

        RecipientDialog::~RecipientDialog()
        {
            KConfigGroup config(KGlobal::config(), "DCCRecipientDialog");
            saveDialogSize(config);
        }

        QString RecipientDialog::getSelectedNickname()
        {
            return selectedNickname;
        }

        void RecipientDialog::newNicknameSelected(const QModelIndex& index)
        {
            nicknameInput->setText(index.data().toString());
        }

        void RecipientDialog::newNicknameSelectedQuit(const QModelIndex& index)
        {
            newNicknameSelected(index);
            selectedNickname = nicknameInput->text();

            delayedDestruct();
        }

        void RecipientDialog::slotCancel()
        {
            selectedNickname.clear();
            reject();
        }

        void RecipientDialog::slotOk()
        {
            selectedNickname=nicknameInput->text();
            accept();
        }

        QString RecipientDialog::getNickname(QWidget* parent, QAbstractListModel* model)
        {
            QPointer<RecipientDialog> dlg = new RecipientDialog(parent, model);
            dlg->exec();
            const QString selectedNick = dlg->getSelectedNickname();

            delete dlg;
            return selectedNick;
        }

    }
}

#include "recipientdialog.moc"
