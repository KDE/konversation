/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef DCCRESUMEDIALOG_H
#define DCCRESUMEDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>

class KUrlRequester;
class QCheckBox;
class QAbstractButton;

namespace Konversation
{
    namespace DCC
    {
        class TransferRecv;

        class ResumeDialog : public QDialog
        {
            Q_OBJECT

            public:
                enum ReceiveAction
                {
                    RA_Rename    = 1,
                    RA_Overwrite = 1 << 1,
                    RA_Resume    = 1 << 2,
                    RA_Cancel    = 1 << 3,
                    RA_OverwriteDefaultPath = 1 << 4
                };

                ~ResumeDialog() override;

                static ReceiveAction ask(TransferRecv* item, const QString& message, int enabledActions, ReceiveAction defaultAction);

            private Q_SLOTS:
                void suggestNewName();
                void setDefaultName();
                void updateDialogButtons();

                void buttonClicked(QAbstractButton*);

            private:
                ResumeDialog(Konversation::DCC::TransferRecv* item, const QString& caption, const QString& message, int enabledActions, QFlags<QDialogButtonBox::StandardButton> enabledButtonCodes, QDialogButtonBox::StandardButton defaultButtonCode);

            private:
                // UI
                KUrlRequester* m_urlreqFileURL;
                QCheckBox* m_overwriteDefaultPathCheckBox;
                QDialogButtonBox* m_buttonBox;

                // data
                TransferRecv* m_item;
                int m_enabledActions;
                ReceiveAction m_selectedAction;

                Q_DISABLE_COPY(ResumeDialog)
        };
    }
}

#endif
