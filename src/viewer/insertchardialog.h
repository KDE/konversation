/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONINSERTCHARDIALOG_H
#define KONVERSATIONINSERTCHARDIALOG_H

#include <QDialog>


class KCharSelect;

namespace Konversation
{

    class InsertCharDialog : public QDialog
    {
        Q_OBJECT

        public:
            explicit InsertCharDialog(const QString& font = QString(), QWidget *parent = nullptr);
            ~InsertCharDialog() override;

            void setFont(const QFont &font);
            uint chr() const;

        Q_SIGNALS:
            void insertChar(uint);

        private Q_SLOTS:
            void charSelected();
            void slotAccepted();

        private:
            KCharSelect* m_charTable;

            Q_DISABLE_COPY(InsertCharDialog)
    };

}
#endif
