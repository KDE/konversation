/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONINSERTCHARDIALOG_H
#define KONVERSATIONINSERTCHARDIALOG_H

#include <QDialog>


class KCharSelect;
class QChar;

namespace Konversation
{

    class InsertCharDialog : public QDialog
    {
        Q_OBJECT

        public:
            explicit InsertCharDialog(const QString& font = QString(), QWidget *parent = nullptr);
            ~InsertCharDialog();

            void setFont(const QFont &font);
            uint chr() const;

        protected Q_SLOTS:
            void charSelected();
            void slotAccepted();

        Q_SIGNALS:
            void insertChar(uint);

        private:
            KCharSelect* m_charTable;
    };

}
#endif
