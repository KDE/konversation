/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
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
            auto* dialogLayout = new QVBoxLayout(this);
            // Add the nickname list widget
            auto *sortModel = new QSortFilterProxyModel(this);
            sortModel->setSortCaseSensitivity(Preferences::self()->sortCaseInsensitive() ? Qt::CaseInsensitive : Qt::CaseSensitive);
            sortModel->setSourceModel(model);
            sortModel->sort(0, Qt::AscendingOrder);
            auto* nicknameList = new QListView(this);
            nicknameList->setUniformItemSizes(true);
            nicknameList->setModel(sortModel);

            nicknameInput = new KLineEdit(this);

            dialogLayout->addWidget(nicknameList);
            dialogLayout->addWidget(nicknameInput);

            connect(nicknameList, &QListView::clicked, this, &RecipientDialog::newNicknameSelected);
            connect(nicknameList, &QListView::doubleClicked, this, &RecipientDialog::newNicknameSelectedQuit);

            auto* buttonBox = new QDialogButtonBox(this);
            dialogLayout->addWidget(buttonBox);
            QPushButton* button = buttonBox->addButton(QDialogButtonBox::Ok);
            button->setToolTip(i18n("Select nickname and close the window"));
            button->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok")));
            button->setShortcut(Qt::CTRL | Qt::Key_Return);
            button->setDefault(true);
            button = buttonBox->addButton(QDialogButtonBox::Cancel);
            button->setToolTip(i18n("Close the window without changes"));
            button->setIcon(QIcon::fromTheme(QStringLiteral("dialog-cancel")));

            KWindowConfig::restoreWindowSize(windowHandle(), KConfigGroup(KSharedConfig::openStateConfig(), QStringLiteral("DCCRecipientDialog")));

            connect(buttonBox, &QDialogButtonBox::accepted, this, &RecipientDialog::slotOk);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &RecipientDialog::slotCancel);
        }

        RecipientDialog::~RecipientDialog()
        {
            KConfigGroup config(KSharedConfig::openStateConfig(), QStringLiteral("DCCRecipientDialog"));
            KWindowConfig::saveWindowSize(windowHandle(), config);
        }

        QString RecipientDialog::getSelectedNickname() const
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

#include "moc_recipientdialog.cpp"
