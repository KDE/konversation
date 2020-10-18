/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef RECIPIENTDIALOG_H
#define RECIPIENTDIALOG_H

#include <QModelIndex>

#include <QDialog>

class KLineEdit;

namespace Konversation
{
    namespace DCC
    {
        class RecipientDialog : public QDialog
        {
            Q_OBJECT

            public:
                RecipientDialog(QWidget* parent, QAbstractListModel* model);
                ~RecipientDialog() override;

                static QString getNickname(QWidget* parent, QAbstractListModel* model);

            private Q_SLOTS:
                void newNicknameSelected(const QModelIndex& index);
                                                          // KDE double click
                void newNicknameSelectedQuit(const QModelIndex& index);

                void slotOk();
                void slotCancel();

            private:
                QString getSelectedNickname() const;

            private:
                static QString selectedNickname;

                KLineEdit* nicknameInput;

                Q_DISABLE_COPY(RecipientDialog)
        };
    }
}

#endif
