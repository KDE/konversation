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
#include <QDialogButtonBox>
#include <QPushButton>

#include <KLocalizedString>
#include <KLineEdit>
#include <KSharedConfig>
#include <KWindowConfig>
#include <KIconLoader>

namespace Konversation
{
    namespace DCC
    {
        QString RecipientDialog::selectedNickname;     // static

        RecipientDialog::RecipientDialog(QWidget* parent, QAbstractListModel* model) :
          QDialog(parent)
        {
            setModal( true );
            setWindowTitle( i18n("Select Recipient") );
            // Add the layout to the widget
            QVBoxLayout* dialogLayout = new QVBoxLayout(this);
            // Add the nickname list widget
            QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(this);
            sortModel->setSortCaseSensitivity(Preferences::self()->sortCaseInsensitive() ? Qt::CaseInsensitive : Qt::CaseSensitive);
            sortModel->setSourceModel(model);
            sortModel->sort(0, Qt::AscendingOrder);
            QListView* nicknameList = new QListView(this);
            nicknameList->setUniformItemSizes(true);
            nicknameList->setModel(sortModel);

            nicknameInput = new KLineEdit(this);

            dialogLayout->addWidget(nicknameList);
            dialogLayout->addWidget(nicknameInput);

            connect(nicknameList, &QListView::clicked, this, &RecipientDialog::newNicknameSelected);
            connect(nicknameList, &QListView::doubleClicked, this, &RecipientDialog::newNicknameSelectedQuit);

            QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
            dialogLayout->addWidget(buttonBox);
            QPushButton* button = buttonBox->addButton(QDialogButtonBox::Ok);
            button->setToolTip(i18n("Select nickname and close the window"));
            button->setIcon(SmallIcon("dialog-ok"));
            button->setShortcut(Qt::CTRL | Qt::Key_Return);
            button->setDefault(true);
            button = buttonBox->addButton(QDialogButtonBox::Cancel);
            button->setToolTip(i18n("Close the window without changes"));
            button->setIcon(SmallIcon("dialog-cancel"));

            KWindowConfig::restoreWindowSize(windowHandle(), KConfigGroup(KSharedConfig::openConfig(), "DCCRecipientDialog"));

            connect(buttonBox, &QDialogButtonBox::accepted, this, &RecipientDialog::slotOk);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &RecipientDialog::slotCancel);
        }

        RecipientDialog::~RecipientDialog()
        {
            KConfigGroup config(KSharedConfig::openConfig(), "DCCRecipientDialog");
            KWindowConfig::saveWindowSize(windowHandle(), config);
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

            deleteLater();
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


