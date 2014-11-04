/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
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
            explicit InsertCharDialog(const QString& font = QString(), QWidget *parent = 0);
            ~InsertCharDialog();

            void setFont(const QFont &font);
            QChar chr();

        protected Q_SLOTS:
            void charSelected();
            void slotAccepted();

        Q_SIGNALS:
            void insertChar(const QChar&);

        private:
            KCharSelect* m_charTable;
    };

}
#endif
