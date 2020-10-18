/*
    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONVERSATION_PASTEEDITOR_H
#define KONVERSATION_PASTEEDITOR_H

#include "ui_pasteeditor.h"

#include <QDialog>

class QAction;

class PasteEditor : public QDialog, private Ui::PasteEditor
{
    Q_OBJECT
    public:
        explicit PasteEditor(QWidget* parent);
        ~PasteEditor() override;

        void setText(const QString& text);
        QString text() const;

        static QString edit(QWidget* parent, const QString& text);

    private Q_SLOTS:
        void addQuotationIndicators();
        void removeNewlines();
        void doInlineAutoreplace();

    private:
        bool m_autoReplaceActionWasEnabled;
        QAction* m_autoReplaceAction;

        Q_DISABLE_COPY(PasteEditor)
};

#endif //KONVERSATION_PASTEEDITOR_H
