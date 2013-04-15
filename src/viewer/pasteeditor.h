// Copyright 2009 Peter Simonsson <peter.simonsson@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef KONVERSATION_PASTEEDITOR_H
#define KONVERSATION_PASTEEDITOR_H

#include "ui_pasteeditor.h"

#include <KDialog>

class QAction;

class PasteEditor : public KDialog, private Ui::PasteEditor
{
    Q_OBJECT
    public:
        explicit PasteEditor(QWidget* parent);
        ~PasteEditor();

        void setText(const QString& text);
        QString text() const;

        static QString edit(QWidget* parent, const QString& text);

    protected slots:
        void addQuotationIndicators();
        void removeNewlines();

    private slots:
        void doInlineAutoreplace();

    private:
        bool m_autoReplaceActionWasEnabled;
        QAction* m_autoReplaceAction;
};

#endif //KONVERSATION_PASTEEDITOR_H
