/*
    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pasteeditor.h"
#include "preferences.h"
#include "application.h"

#include <QPushButton>
#include <KLocalizedString>
#include <KActionCollection>

#include <QTextCursor>
#include <QTextOption>
#include <QPointer>
#include <QAction>

PasteEditor::PasteEditor(QWidget* parent)
    : QDialog(parent), Ui::PasteEditor(), m_autoReplaceActionWasEnabled(true), m_autoReplaceAction(nullptr)
{
    setWindowTitle(i18n("Edit Multiline Paste"));
    setModal(true);

    setupUi(this);

    m_buttonBox->addButton(i18n("&Send"), QDialogButtonBox::AcceptRole);

    m_textEditor->enableFindReplace(true);

    QTextOption options = m_textEditor->document()->defaultTextOption();
    options.setFlags(options.flags() | QTextOption::ShowTabsAndSpaces);
    m_textEditor->document()->setDefaultTextOption(options);
    m_textEditor->setFocus();

    // Get auto_replace action, check enabled state and make the connection
    m_autoReplaceAction = Application::instance()->getMainWindow()->actionCollection()->action(QStringLiteral("auto_replace"));
    if (m_autoReplaceAction)
    {
        // Store action's original enabled state in ViewContainer
        m_autoReplaceActionWasEnabled = m_autoReplaceAction->isEnabled();
        m_autoReplaceAction->setEnabled(true);
        addAction(m_autoReplaceAction);
        connect(m_autoReplaceAction, &QAction::triggered, this, &PasteEditor::doInlineAutoreplace);
    }

    connect(m_removeNewlinesButton, &QPushButton::clicked, this, &PasteEditor::removeNewlines);
    connect(m_addQuotesButton, &QPushButton::clicked, this, &PasteEditor::addQuotationIndicators);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PasteEditor::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PasteEditor::reject);

    resize(Preferences::self()->multilineEditSize());
}

PasteEditor::~PasteEditor()
{
    Preferences::self()->setMultilineEditSize(size());
    // Restore action's original state in ViewContainer
    if (m_autoReplaceAction)
        m_autoReplaceAction->setEnabled(m_autoReplaceActionWasEnabled);
}

void PasteEditor::addQuotationIndicators()
{
    QTextCursor cursor(m_textEditor->document());
    cursor.beginEditBlock();
    cursor.insertText(QStringLiteral("> "));

    while(cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor))
    {
        cursor.insertText(QStringLiteral("> "));
    }

    cursor.endEditBlock();
}

void PasteEditor::removeNewlines()
{
    QString text(m_textEditor->toPlainText());
    text.remove(QLatin1Char('\r'));
    text.remove(QRegularExpression(QStringLiteral("^\n+|\n+$")));

    int i = 0;

    while (i < text.length())
    {
        if (text[i] == QLatin1Char('\n')) {
            if (text[i - 1].category() == QChar::Separator_Space)
            {
                text.remove(i, 1);
                continue;
            }
            else
                text[i] = QLatin1Char(' ');
        }

        ++i;
    }

    QTextCursor cursor(m_textEditor->document());
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
}

void PasteEditor::setText(const QString& text)
{
    m_textEditor->setPlainText(text);
}

QString PasteEditor::text() const
{
    return m_textEditor->toPlainText();
}

QString PasteEditor::edit(QWidget* parent, const QString& text)
{
    QPointer<PasteEditor> dialog = new PasteEditor(parent);
    dialog->setText(text);

    if(dialog->exec())
    {
        const QString text = dialog->text();
        delete dialog;
        return text;
    }
    delete dialog;
    return QString();
}

void PasteEditor::doInlineAutoreplace()
{
    Application::instance()->doInlineAutoreplace(m_textEditor);
}

#include "moc_pasteeditor.cpp"
