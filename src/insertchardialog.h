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

#include <kdialogbase.h>

class KCharSelect;
class QChar;

namespace Konversation
{

    class InsertCharDialog : public KDialogBase
    {
        Q_OBJECT

        public:
            explicit InsertCharDialog(const QString& font = QString(), QWidget *parent = 0, const char *name = 0);
            ~InsertCharDialog();

            QChar chr();

        protected slots:
            virtual void slotOk();

            signals:
            void insertChar(const QChar&);

        private:
            KCharSelect* m_charTable;
    };

}
#endif
